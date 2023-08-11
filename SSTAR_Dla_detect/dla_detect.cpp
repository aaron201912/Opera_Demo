/* SigmaStar trade secret */
/* Copyright (c) [2019~2020] SigmaStar Technology.
All rights reserved.

Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
SigmaStar and be kept in strict confidence
(SigmaStar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of SigmaStar Confidential
Information is unlawful and strictly prohibited. SigmaStar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>

#include <string.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
//#include <vector>

#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

#if 0
using std::cout;
using std::endl;
using std::ostringstream;
using std::vector;
using std::string;
using std::ios;
#endif

#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"



#define  DETECT_IMAGE_FUNC_INFO(fmt, args...)           do {printf("[Info ] [%-4d] [%10s] ", __LINE__, __func__); printf(fmt, ##args);} while(0)



#define LABEL_CLASS_COUNT (100)
#define LABEL_NAME_MAX_SIZE (60)

struct PreProcessedData {
    char *pImagePath;
    int iResizeH;
    int iResizeW;
    int iResizeC;
    bool bRGB;
    unsigned char * pdata;

} ;


struct DetectionBBoxInfo {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float score;
    int   classID;

};



MI_S32  IPUCreateDevice(char *pFirmwarePath,MI_U32 u32VarBufSize)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_IPU_DevAttr_t stDevAttr;
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;

    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, pFirmwarePath, 0);
    return s32Ret;
}



MI_S32 IPUCreateChannel(MI_U32 *s32Channel, char *pModelImage)
{


    MI_S32 s32Ret ;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 0;
    stChnAttr.u32OutputBufDepth = 0;
    return MI_IPU_CreateCHN(s32Channel, &stChnAttr, NULL, pModelImage);
}

MI_S32 IPUDestroyChannel(MI_U32 s32Channel)
{
    return MI_IPU_DestroyCHN(s32Channel);
}

MI_S32 IPU_Malloc(MI_IPU_Tensor_t* pTensor, MI_U32 BufSize)
{
    MI_S32 s32ret = 0;
    MI_PHY phyAddr = 0;
    void* pVirAddr = NULL;
    s32ret = MI_SYS_MMA_Alloc(0, NULL, BufSize, &phyAddr);
    if (s32ret != MI_SUCCESS)
    {
        std::cerr << "Alloc buffer failed!" << std::endl;
        return s32ret;
    }
    s32ret = MI_SYS_Mmap(phyAddr, BufSize, &pVirAddr, TRUE);
    if (s32ret != MI_SUCCESS)
    {
        std::cerr << "Mmap buffer failed!" << std::endl;
        MI_SYS_MMA_Free(0, phyAddr);
        return s32ret;
    }
    pTensor->phyTensorAddr[0] = phyAddr;
    pTensor->ptTensorData[0] = pVirAddr;
    return s32ret;
}

MI_S32 IPU_Free(MI_IPU_Tensor_t* pTensor, MI_U32 BufSize)
{
    MI_S32 s32ret = 0;
    s32ret = MI_SYS_Munmap(pTensor->ptTensorData[0], BufSize);
    s32ret = MI_SYS_MMA_Free(0, pTensor->phyTensorAddr[0]);
    return s32ret;
}

void GetImage(PreProcessedData *pstPreProcessedData)
{
    string filename=(string)(pstPreProcessedData->pImagePath);
    cv::Mat sample;
    cv::Mat img = cv::imread(filename, -1);
    if (img.empty()) {
      std::cout << " error!  image don't exist!" << std::endl;
      exit(1);
    }


    int num_channels_  = pstPreProcessedData->iResizeC;
    if (img.channels() == 3 && num_channels_ == 1)
    {
        cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
    }
    else if (img.channels() == 4 && num_channels_ == 1)
    {
        cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
    }
    else if (img.channels() == 4 && num_channels_ == 3)
    {
        cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
    }
    else if (img.channels() == 1 && num_channels_ == 3)
    {
        cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
    }
    else
    {
        sample = img;
    }



    cv::Mat sample_float;
    if (num_channels_ == 3)
      sample.convertTo(sample_float, CV_32FC3);
    else
      sample.convertTo(sample_float, CV_32FC1);



    cv::Mat sample_norm =sample_float ;
    if (pstPreProcessedData->bRGB)
    {
        cv::cvtColor(sample_float, sample_norm, cv::COLOR_BGR2RGB);
    }

    cv::Mat sample_resized;
    cv::Size inputSize = cv::Size(pstPreProcessedData->iResizeW, pstPreProcessedData->iResizeH);
    if (sample.size() != inputSize)
    {
		cout << "input size should be :" << pstPreProcessedData->iResizeC << " " << pstPreProcessedData->iResizeH << " " << pstPreProcessedData->iResizeW << endl;
		cout << "now input size is :" << img.channels() << " " << img.rows<<" " << img.cols << endl;
		cout << "img is going to resize!" << endl;
		cv::resize(sample_norm, sample_resized, inputSize);
	}
    else
	{
      sample_resized = sample_norm;
    }

    float *pfSrc = (float *)sample_resized.data;
    int imageSize = pstPreProcessedData->iResizeC*pstPreProcessedData->iResizeW*pstPreProcessedData->iResizeH;

    for(int i=0;i<imageSize;i++)
    {
        *(pstPreProcessedData->pdata+i) = (unsigned char)(round(*(pfSrc + i)));
    }


}


#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))


void ShowFloatOutPutTensor(float *pfBBox,float *pfClass, float *pfscore, float *pfDetect)
{
    // show bbox
    int s32DetectCount = round(*pfDetect);
    cout<<"BBox:"<<std::endl;
    cout.flags(ios::left);
    for(int i=0;i<s32DetectCount;i++)
    {
       for(int j=0;j<4;j++)
       {
            cout<<setw(15)<<*(pfBBox+(i*4)+j);
       }
       if (i!=0)
       {
            cout<<std::endl;
       }
    }

    //show class
    cout<<"Class:"<<std::endl;
    for(int  i=0;i<s32DetectCount;i++)
    {
       cout<<setw(15)<<round(*(pfClass+i));
    }
    cout<<std::endl;

    //show score
    cout<<"score:"<<std::endl;
    for(int  i=0;i<s32DetectCount;i++)
    {
       cout<<setw(15)<<*(pfscore+i);
    }
    cout<<std::endl;

    //show deteccout
    cout<<"DetectCount"<<std::endl;
    cout<<s32DetectCount<<std::endl;


}

 void WriteVisualizeBBox(string strImageName,
                   const vector<DetectionBBoxInfo > detections,
                   const float threshold, const vector<cv::Scalar>& colors,
                   const map<int, string>& classToDisName)
  {
  // Retrieve detections.
  cv::Mat image = cv::imread(strImageName, -1);
  map< int, vector<DetectionBBoxInfo> > detectionsInImage;

 for (unsigned int j = 0; j < detections.size(); j++) {
   DetectionBBoxInfo bbox;
   const int label = detections[j].classID;
   const float score = detections[j].score;
   if (score < threshold) {
     continue;
   }


   bbox.xmin =  detections[j].xmin*image.cols;
   bbox.xmin = bbox.xmin < 0 ? 0:bbox.xmin ;

   bbox.ymin =  detections[j].ymin*image.rows;
   bbox.ymin = bbox.ymin < 0 ? 0:bbox.ymin ;

   bbox.xmax =  detections[j].xmax*image.cols;
   bbox.xmax = bbox.xmax > image.cols?image.cols:bbox.xmax;

   bbox.ymax =  detections[j].ymax*image.rows;
   bbox.ymax = bbox.ymax > image.rows ? image.rows:bbox.ymax ;

   bbox.score = score;
   bbox.classID = label;
   detectionsInImage[label].push_back(bbox);
 }


  int fontface = cv::FONT_HERSHEY_SIMPLEX;
  double scale = 0.5;
  int thickness = 1;
  int baseline = 0;
  char buffer[50];

    // Show FPS.
//    snprintf(buffer, sizeof(buffer), "FPS: %.2f", fps);
//    cv::Size text = cv::getTextSize(buffer, fontface, scale, thickness,
//                                    &baseline);
//    cv::rectangle(image, cv::Point(0, 0),
//                  cv::Point(text.width, text.height + baseline),
//                  CV_RGB(255, 255, 255), CV_FILLED);
//    cv::putText(image, buffer, cv::Point(0, text.height + baseline / 2.),
//                fontface, scale, CV_RGB(0, 0, 0), thickness, 8);
    // Draw bboxes.
    std::string name = strImageName;

    unsigned int pos = strImageName.rfind("/");
    if (pos > 0 && pos < strImageName.size()) {
      name = name.substr(pos + 1);
    }

    std::string strOutImageName = name;
    strOutImageName = "out_"+strOutImageName;

    pos = name.rfind(".");
    if (pos > 0 && pos < name.size()) {
      name = name.substr(0, pos);
    }

    name = name + ".txt";
    std::ofstream file(name);
    for (map<int, vector<DetectionBBoxInfo> >::iterator it =
         detectionsInImage.begin(); it != detectionsInImage.end(); ++it) {
      int label = it->first;
      string label_name = "Unknown";
      if (classToDisName.find(label) != classToDisName.end()) {
        label_name = classToDisName.find(label)->second;
      }
      const cv::Scalar& color = colors[label];
      const vector<DetectionBBoxInfo>& bboxes = it->second;
      for (unsigned int j = 0; j < bboxes.size(); ++j) {
        cv::Point top_left_pt(bboxes[j].xmin, bboxes[j].ymin);
        cv::Point bottom_right_pt(bboxes[j].xmax, bboxes[j].ymax);
        cv::rectangle(image, top_left_pt, bottom_right_pt, color, 1);
        cv::Point bottom_left_pt(bboxes[j].xmin, bboxes[j].ymax);
        snprintf(buffer, sizeof(buffer), "%s: %.2f", label_name.c_str(),
                 bboxes[j].score);
        cv::Size text = cv::getTextSize(buffer, fontface, scale, thickness,
                                        &baseline);
        cv::rectangle(
            image, top_left_pt + cv::Point(0, 0),
            top_left_pt + cv::Point(text.width, -text.height - baseline),
            color, 1);
        cv::putText(image, buffer, top_left_pt - cv::Point(0, baseline),
                    fontface, scale, CV_RGB(0,255, 0), thickness, 8);
        file << label_name << " " << bboxes[j].score << " "
            << bboxes[j].xmin / image.cols << " "
            << bboxes[j].ymin / image.rows << " "
            << bboxes[j].xmax / image.cols
            << " " << bboxes[j].ymax / image.rows << std::endl;
      }
    }
    file.close();
    cv::imwrite(strOutImageName.c_str(), image);

}

std::vector<DetectionBBoxInfo >  GetDetections(float *pfBBox,float *pfClass, float *pfScore, float *pfDetect)
{
    // show bbox
    int s32DetectCount = round(*pfDetect);
    std::vector<DetectionBBoxInfo > detections(s32DetectCount);
    for(int i=0;i<s32DetectCount;i++)
    {
        DetectionBBoxInfo  detection;
        memset(&detection,0,sizeof(DetectionBBoxInfo));
        //box coordinate
        detection.ymin =  *(pfBBox+(i*4)+0);
        detection.xmin =  *(pfBBox+(i*4)+1);
        detection.ymax =  *(pfBBox+(i*4)+2);
        detection.xmax =  *(pfBBox+(i*4)+3);


        //box class
        detection.classID = round(*(pfClass+i));


        //score
        detection.score = *(pfScore+i);
        detections.push_back(detection);

    }

    return detections;

}

int  GetLabels(char *pLabelPath, char label[][LABEL_NAME_MAX_SIZE])
{
    ifstream LabelFile;
    LabelFile.open(pLabelPath);
    int n=0;
    while(1)
    {
        LabelFile.getline(&label[n][0],60);
        if(LabelFile.eof())
            break;
        n++;
        if(n>=LABEL_CLASS_COUNT)
        {
            cout<<"the labels have line:"<<n<<" ,it supass the available label array"<<std::endl;
        }
    }

    LabelFile.close();
    return n;

}
cv::Scalar HSV2RGB(const float h, const float s, const float v) {
  const int h_i = static_cast<int>(h * 6);
  const float f = h * 6 - h_i;
  const float p = v * (1 - s);
  const float q = v * (1 - f*s);
  const float t = v * (1 - (1 - f) * s);
  float r, g, b;
  switch (h_i) {
    case 0:
      r = v; g = t; b = p;
      break;
    case 1:
      r = q; g = v; b = p;
      break;
    case 2:
      r = p; g = v; b = t;
      break;
    case 3:
      r = p; g = q; b = v;
      break;
    case 4:
      r = t; g = p; b = v;
      break;
    case 5:
      r = v; g = p; b = q;
      break;
    default:
      r = 1; g = 1; b = 1;
      break;
  }
  return cv::Scalar(r * 255, g * 255, b * 255);
}
vector<cv::Scalar> GetColors(const int n)
{
  vector<cv::Scalar> colors;
  cv::RNG rng(12345);
  const float golden_ratio_conjugate = 0.618033988749895;
  const float s = 0.3;
  const float v = 0.99;
  for (int i = 0; i < n; ++i) {
    const float h = std::fmod(rng.uniform(0.f, 1.f) + golden_ratio_conjugate,
                              1.f);
    colors.push_back(HSV2RGB(h, s, v));
  }
  return colors;
}

MI_U32 _MI_IPU_GetTensorUnitDataSize(MI_IPU_ELEMENT_FORMAT eElmFormat)
{
    switch (eElmFormat) {
        case MI_IPU_FORMAT_INT16:
            return sizeof(short);
        case MI_IPU_FORMAT_INT32:
            return sizeof(int);
        case MI_IPU_FORMAT_INT8:
            return sizeof(char);
        case MI_IPU_FORMAT_FP32:
            return sizeof(float);
        case MI_IPU_FORMAT_UNKNOWN:
        default:
            return 1;
    }
}

MI_U32 IPU_CalcTensorSize(MI_IPU_TensorDesc_t stTensorDescs)
{
    MI_U32 u32Size = 1;
    MI_U32 u32UnitSize = 1;

    u32UnitSize = _MI_IPU_GetTensorUnitDataSize(stTensorDescs.eElmFormat);

    for (int i = 0; i < stTensorDescs.u32TensorDim; i++)
    {
        u32Size *= stTensorDescs.u32TensorShape[i];
    }
    u32Size *= u32UnitSize;

    return u32Size;
}

static void IPU_PrintOutputXOR(MI_IPU_SubNet_InputOutputDesc_t desc, MI_IPU_TensorVector_t OutputTensorVector)
{
    MI_U32 u32InputNum = desc.u32InputTensorCount;
    MI_U32 u32OutputNum = desc.u32OutputTensorCount;

    volatile MI_U32 u32XORValue = 0;
    MI_U8 *pu8XORValue[4]= {(MI_U8 *)&u32XORValue,(MI_U8 *)&u32XORValue+1,(MI_U8 *)&u32XORValue+2,(MI_U8 *)&u32XORValue+3 };
    MI_U32 u32Count = 0;

    for (MI_U32 idxOutputNum = 0; idxOutputNum < desc.u32OutputTensorCount; idxOutputNum++)
    {
        MI_U8 u8Data = 0;
        MI_U8 *pu8Data = (MI_U8 *)OutputTensorVector.astArrayTensors[idxOutputNum].ptTensorData[0];
        for(int i = 0; i < IPU_CalcTensorSize(desc.astMI_OutputTensorDescs[idxOutputNum]); i++)
        {
            u8Data = *(pu8Data + i);
            *pu8XORValue[u32Count%4] ^= u8Data;
            u32Count++;
        }
    }
    printf("All outputs XOR = 0x%08x\n", u32XORValue);
}


int main(int argc,char *argv[])
{

    if ( argc < 5 )
    {
        std::cout << "USAGE: " << argv[0] <<": <xxxsgsimg.img> " \
        << "<picture> " << "<labels> "<< "<model intput_format:RGB or BGR>"<<std::endl;
        exit(0);
    } else {
         std::cout<<"model_img:"<<argv[1]<<std::endl;
         std::cout<<"picture:"<<argv[2]<<std::endl;
         std::cout<<"labels:"<<argv[3]<<std::endl;
         std::cout<<"model input_format:"<<argv[4]<<std::endl;
    }


    char * pFirmwarePath = NULL;
    char * pModelImgPath = argv[1];
    char * pImagePath= argv[2];
    char * pLabelPath = argv[3];
    MI_U32 u32ChannelID = 0;
    char * pRGB = argv[4];
    MI_BOOL bRGB = FALSE;

    if(strncmp(pRGB,"RGB",sizeof("RGB"))!=0 && strncmp(pRGB,"BGR",sizeof("BGR"))!=0 && strncmp(pRGB,"RAWDATA",sizeof("RAWDATA"))!=0)
    {

        std::cout << "model intput_format error" <<std::endl;
        return -1;

    }

    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector;
    MI_IPU_TensorVector_t OutputTensorVector;
    MI_IPU_OfflineModelStaticInfo_t OfflineModelInfo;
    static char labels[LABEL_CLASS_COUNT][LABEL_NAME_MAX_SIZE];
    int labelCount = GetLabels(pLabelPath, labels);


    MI_SYS_Init(0);

    //1.create device
    if(MI_SUCCESS != MI_IPU_GetOfflineModeStaticInfo(NULL, pModelImgPath, &OfflineModelInfo))
    {
        cout<<"get model variable buffer size failed!"<<std::endl;
        return -1;
    }
    if(MI_SUCCESS !=IPUCreateDevice(pFirmwarePath,OfflineModelInfo.u32VariableBufferSize))
    {
        cout<<"create ipu device failed!"<<std::endl;
        return -1;
    }



    //2.create channel
    if(MI_SUCCESS!=IPUCreateChannel(&u32ChannelID,pModelImgPath))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }


    //3.get input/output tensor

    MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);
    if (desc.u32OutputTensorCount != 4)
    {
        std::cerr << "Num of output != 4, can't for detect!" << std::endl;
        IPUDestroyChannel(u32ChannelID);
        MI_IPU_DestroyDevice();
        return -1;
    }

    InputTensorVector.u32TensorCount = desc.u32InputTensorCount;
    if (MI_SUCCESS != IPU_Malloc(&InputTensorVector.astArrayTensors[0], desc.astMI_InputTensorDescs[0].s32AlignedBufSize))
    {
        IPUDestroyChannel(u32ChannelID);
        MI_IPU_DestroyDevice();
        return -1;
    }

    OutputTensorVector.u32TensorCount = desc.u32OutputTensorCount;
    for (MI_S32 idx = 0; idx < desc.u32OutputTensorCount; idx++)
    {
        if (MI_SUCCESS != IPU_Malloc(&OutputTensorVector.astArrayTensors[idx], desc.astMI_OutputTensorDescs[idx].s32AlignedBufSize))
        {
            IPUDestroyChannel(u32ChannelID);
            MI_IPU_DestroyDevice();
            return -1;
        }
    }

    unsigned char *pu8ImageData = NULL;
    const char* dump_input_bin = getenv("DUMP_INPUT_BIN");
    int datasize;
    if(strncmp(pRGB,"RAWDATA",sizeof("RAWDATA"))==0)
        {
            FILE* stream;
            stream = fopen(pImagePath,"r");
            fseek(stream, 0, SEEK_END);
            int length = ftell(stream);
            cout << "length==" << length<<endl;
            rewind(stream);

            if(length != desc.astMI_InputTensorDescs[0].s32AlignedBufSize)
            {
                cout<<"please check input bin size"<<endl;
                exit(0);
            }

            pu8ImageData = new unsigned char[length];

            datasize = fread(pu8ImageData,sizeof(unsigned char),length,stream);
            cout << "size==" << datasize <<endl;
            fclose(stream);

        }
    else
        {
            int iResizeH = desc.astMI_InputTensorDescs[0].u32TensorShape[1];
            int iResizeW = desc.astMI_InputTensorDescs[0].u32TensorShape[2];
            int iResizeC = desc.astMI_InputTensorDescs[0].u32TensorShape[3];
            pu8ImageData = new unsigned char[iResizeH*iResizeW*iResizeC];

            PreProcessedData stProcessedData;
            stProcessedData.iResizeC = iResizeC;
            stProcessedData.iResizeH = iResizeH;
            stProcessedData.iResizeW = iResizeW;
            stProcessedData.pdata = pu8ImageData;
            stProcessedData.pImagePath = pImagePath;
            if(strncmp(pRGB,"RGB",sizeof("RGB"))==0)
            {
              bRGB = TRUE;
            }
            stProcessedData.bRGB = bRGB;
            GetImage(&stProcessedData);

            datasize = iResizeH*iResizeW*iResizeC;

        }

    memcpy(InputTensorVector.astArrayTensors[0].ptTensorData[0],pu8ImageData,datasize);
    MI_SYS_FlushInvCache(InputTensorVector.astArrayTensors[0].ptTensorData[0], datasize);

    if(dump_input_bin)
    {
        FILE* stream_input = fopen("inputtoinvoke.bin","w");
        int input_size = fwrite(InputTensorVector.astArrayTensors[0].ptTensorData[0],sizeof(unsigned char),datasize,stream_input);
        fclose(stream_input);

    }


    //4.invoke
#if 0
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
#endif
    int times = 32;
    for (int i=0;i<times;i++ )
    {
        if(MI_SUCCESS!=MI_IPU_Invoke(u32ChannelID, &InputTensorVector, &OutputTensorVector))
        {
            cout<<"IPU invoke failed!!"<<endl;
            delete pu8ImageData;
            IPUDestroyChannel(u32ChannelID);
            MI_IPU_DestroyDevice();
            return -1;
        }
    }
#if 0
    clock_gettime(CLOCK_MONOTONIC, &ts_end);

    int elasped_time = (ts_end.tv_sec-ts_start.tv_sec)*1000000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000;
    cout<<"fps:"<<1000.0/(float(elasped_time)/1000/times)<<std::endl;
#endif

    // show result of detect
    IPU_PrintOutputXOR(desc, OutputTensorVector);

    if (desc.astMI_OutputTensorDescs[0].eElmFormat == MI_IPU_FORMAT_FP32 &&
        desc.astMI_OutputTensorDescs[1].eElmFormat == MI_IPU_FORMAT_FP32 &&
        desc.astMI_OutputTensorDescs[2].eElmFormat == MI_IPU_FORMAT_FP32 &&
        desc.astMI_OutputTensorDescs[3].eElmFormat == MI_IPU_FORMAT_FP32)
    {
        float *pfBBox = (float *)OutputTensorVector.astArrayTensors[0].ptTensorData[0];
        float *pfClass = (float *)OutputTensorVector.astArrayTensors[1].ptTensorData[0];
        float *pfScore = (float *)OutputTensorVector.astArrayTensors[2].ptTensorData[0];
        float *pfDetect = (float *)OutputTensorVector.astArrayTensors[3].ptTensorData[0];

        ShowFloatOutPutTensor( pfBBox, pfClass, pfScore, pfDetect);

        std::vector<DetectionBBoxInfo >  detections  = GetDetections(pfBBox,pfClass,  pfScore,  pfDetect);
        map<int, string> labelToDisName ;
        for (int i=0; i<labelCount;i++)
        {
            labelToDisName[i] = string(&labels[i][0]);
        }
        vector<cv::Scalar> colors = GetColors(labelToDisName.size());
        if(strncmp(pRGB,"RAWDATA",sizeof("RAWDATA"))!=0)
        {
            WriteVisualizeBBox(pImagePath,  detections, 0.5,colors, labelToDisName);
        }
    }
    else
    {
        cerr << "Output datatype isn't `float32`, pls check input_config.ini" << endl;
    }

    //5. put intput tensor

    IPU_Free(&InputTensorVector.astArrayTensors[0], desc.astMI_InputTensorDescs[0].s32AlignedBufSize);
    for (MI_S32 idx = 0; idx < desc.u32OutputTensorCount; idx++)
    {
        IPU_Free(&OutputTensorVector.astArrayTensors[idx], desc.astMI_OutputTensorDescs[idx].s32AlignedBufSize);
    }

    //6.destroy channel/device

    delete pu8ImageData;
    IPUDestroyChannel(u32ChannelID);
    MI_IPU_DestroyDevice();

    return 0;

}
