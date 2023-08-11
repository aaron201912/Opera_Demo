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
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/time.h>
#include <unistd.h>
#include <sys/mman.h>
#if 0
#include <openssl/aes.h>

#include <openssl/evp.h>

#include <openssl/rsa.h>
#endif
using namespace std;
using std::cout;
using std::endl;
using std::ostringstream;
using std::vector;
using std::string;

#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"



#define  LABEL_IMAGE_FUNC_INFO(fmt, args...)           do {printf("[Info ] [%-4d] [%10s] ", __LINE__, __func__); printf(fmt, ##args);} while(0)

#define alignment_up(a,b)  (((a)+(b-1))&(~(b-1)))



struct PreProcessedData {
    char *pImagePath;
    int intResizeH;
    int intResizeW;
    int intResizeC;
    bool bNorm;
    float fmeanB;
    float fmeanG;
    float fmeanR;
    float std;
    bool bRGB;
    unsigned char * pdata;

} ;


#define LABEL_CLASS_COUNT (1200)
#define LABEL_NAME_MAX_SIZE (60)
MI_S32  IPUCreateDevice(char *pFirmwarePath,MI_U32 u32VarBufSize)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_IPU_DevAttr_t stDevAttr;
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;

    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, pFirmwarePath, 0);
    return s32Ret;
}


static int H2SerializedReadFunc_1(void *dst_buf,int offset, int size, char *ctx)
{
// read data from buf
    return 0;
}

static int H2SerializedReadFunc_2(void *dst_buf,int offset, int size, char *ctx)
{
// read data from buf
    std::cout<<"read from call back function"<<std::endl;
    memcpy(dst_buf,ctx+offset,size);
    return 0;

}

MI_S32 IPUCreateChannel(MI_U32 *s32Channel, char *pModelImage)
{


    MI_S32 s32Ret ;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 2;
    stChnAttr.u32OutputBufDepth = 2;
    return MI_IPU_CreateCHN(s32Channel, &stChnAttr, NULL, pModelImage);
}




MI_S32 IPUCreateChannel_FromMemory(MI_U32 *s32Channel, char *pModelImage)
{

    MI_S32 s32Ret ;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 2;
    stChnAttr.u32OutputBufDepth = 2;

    return MI_IPU_CreateCHN(s32Channel, &stChnAttr, H2SerializedReadFunc_2, pModelImage);
}



MI_S32 IPUCreateChannel_FromEncryptFile(MI_U32 *s32Channel, char *pModelImage)
{

    MI_S32 s32Ret ;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 2;
    stChnAttr.u32OutputBufDepth = 2;

    return MI_IPU_CreateCHN(s32Channel, &stChnAttr, H2SerializedReadFunc_2, pModelImage);
}



MI_S32 IPUDestroyChannel(MI_U32 s32Channel)
{
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_IPU_DestroyCHN(s32Channel);
    return s32Ret;

}

void GetImage(   PreProcessedData *pstPreProcessedData)
{
    string filename=(string)(pstPreProcessedData->pImagePath);
    cv::Mat sample;
    cv::Mat img = cv::imread(filename, -1);
    if (img.empty()) {
      std::cout << " error!  image don't exist!" << std::endl;
      exit(1);
    }


    int num_channels_  = pstPreProcessedData->intResizeC;
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

    cv::Mat sample_norm = sample_float;
    if (pstPreProcessedData->bRGB)
    {
        cv::cvtColor(sample_float, sample_norm, cv::COLOR_BGR2RGB);
    }


    cv::Mat sample_resized;
    cv::Size inputSize = cv::Size(pstPreProcessedData->intResizeW, pstPreProcessedData->intResizeH);
    if (sample.size() != inputSize)
    {
		cout << "input size should be :" << pstPreProcessedData->intResizeC << " " << pstPreProcessedData->intResizeH << " " << pstPreProcessedData->intResizeW << endl;
		cout << "now input size is :" << img.channels() << " " << img.rows<<" " << img.cols << endl;
		cout << "img is going to resize!" << endl;
		cv::resize(sample_norm, sample_resized, inputSize);
	}
    else
	{
      sample_resized = sample_norm;
    }

    float *pfSrc = (float *)sample_resized.data;
    int imageSize = pstPreProcessedData->intResizeC*pstPreProcessedData->intResizeW*pstPreProcessedData->intResizeH;

    for(int i=0;i<imageSize;i++)
    {
        *(pstPreProcessedData->pdata+i) = (unsigned char)(round(*(pfSrc + i)));
    }


}


static MI_BOOL GetTopN(float aData[], int dataSize, int aResult[], int TopN)
{
    int i, j, k;
    float data = 0;
    MI_BOOL bSkip = FALSE;

    for (i=0; i < TopN; i++)
    {
        data = -0.1f;
        for (j = 0; j < dataSize; j++)
        {
            if (aData[j] > data)
            {
                bSkip = FALSE;
                for (k = 0; k < i; k++)
                {
                    if (aResult[k] == j)
                    {
                        bSkip = TRUE;
                    }
                }

                if (bSkip == FALSE)
                {
                    aResult[i] = j;
                    data = aData[j];
                }
            }
        }
    }

    return TRUE;
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
    char * pLabelPath =argv[3];
    char * pRGB = argv[4];
    char * pfps = NULL;
    char * ptime = NULL;
    int fps = -1;
    int duration = -1;
    if (argc == 7)
    {
        pfps = argv[5];
        ptime = argv[6];
        fps = atoi(pfps);
        duration = atoi(ptime);


    }
    MI_BOOL bRGB = FALSE;

    if(strncmp(pRGB,"RGB",sizeof("RGB"))!=0 && strncmp(pRGB,"BGR",sizeof("BGR"))!=0 && strncmp(pRGB,"RAWDATA",sizeof("RAWDATA"))!=0)
    {

        std::cout << "model intput_format error" <<std::endl;
        return -1;

    }

    static char label[LABEL_CLASS_COUNT][LABEL_NAME_MAX_SIZE];
    MI_U32 u32ChannelID = 0;
    MI_S32 s32Ret;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector;
    MI_IPU_TensorVector_t OutputTensorVector;
    MI_IPU_OfflineModelStaticInfo_t OfflineModelInfo;
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
            break;
        }
    }

    LabelFile.close();

    MI_SYS_Init(0);

    //1.create device
    cout<<"get variable size from memory__"<<std::endl;
    char *pmem = NULL;
    int fd = 0;
    struct stat sb;
    fd = open(pModelImgPath, O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }
    memset(&sb, 0, sizeof(sb));
    if (fstat(fd, &sb) < 0)
    {
        perror("fstat");
        return -1;
    }
    pmem = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (pmem == NULL)
    {
        perror("mmap");
        return -1;
    }
    if (MI_SUCCESS != MI_IPU_GetOfflineModeStaticInfo(H2SerializedReadFunc_2, pmem, &OfflineModelInfo))
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
    /*case 0 create module from path*/
#if 0
        if(MI_SUCCESS !=IPUCreateChannel(u32ChannelID,pModelImgPath))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }
#endif

#if 1
    /*case1 create channel from memory*/
    cout<<"create channel from memory__"<<std::endl;
    if(MI_SUCCESS !=IPUCreateChannel_FromMemory(&u32ChannelID,pmem))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }
#endif

#if 0
   /*
     case 3 encrypt and decrypt the img file , is not ready
   */


    cout<<"decrypt sgs image"<<std::endl;
    char *pmem = NULL;
    int fd = 0;
    struct stat sb;
    fd = open(pModelImgPath, O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }
    memset(&sb, 0, sizeof(sb));
    if (fstat(fd, &sb) < 0)
    {
        perror("fstat");
        return -1;
    }
    cout<<"the img is 16 byte alignment ?"<<(sb.st_size&15)<<std::endl;
    pmem = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (pmem == NULL)
    {
        perror("mmap");
        return -1;
    }
    int alignment_size = alignment_up(sb.st_size,16);
    unsigned char * encrypt_buffer = new unsigned char[alignment_size];
    unsigned char * decrypt_buffer = new unsigned char[alignment_size];
    memset(decrypt_buffer,0,alignment_size);
    memset(encrypt_buffer,0,alignment_size);
    static unsigned char dahua_keyval[16] ;
    strncpy((char *)dahua_keyval,"1234567890987654",16);
    AES_KEY  aesEncryptKey;
    AES_KEY  aesDecryptKey;
    AES_set_encrypt_key(dahua_keyval, 128, &aesEncryptKey);
    AES_set_encrypt_key(dahua_keyval, 128, &aesDecryptKey);
     cout<<"line:"<<__LINE__<<std::endl;
     cout<<"alignment_size:"<<alignment_size<<std::endl;
    //AES_BLOCK_SIZE
    for(int i=0;i<1;i++)
    {
        AES_ecb_encrypt((unsigned char *)pmem + AES_BLOCK_SIZE * i, encrypt_buffer + AES_BLOCK_SIZE * i, &aesEncryptKey, AES_ENCRYPT);
    }


    for(int i=0;i<1;i++)
    {
         AES_ecb_encrypt(encrypt_buffer  + AES_BLOCK_SIZE * i, decrypt_buffer + AES_BLOCK_SIZE * i, &aesDecryptKey, AES_DECRYPT);
    }


  cout<<"AES_BLOCK_SIZE"<<AES_BLOCK_SIZE<<std::endl;
  cout<<"pmem:"<<*(int *)pmem<<std::endl;
  cout<<"encrypt:"<<*(int *)encrypt_buffer<<std::endl;
  cout<<"decrypt_buffer:"<<*(int *)decrypt_buffer<<std::endl;


    if(MI_SUCCESS !=IPUCreateChannel_FromMemory(u32ChannelID,(char *)decrypt_buffer))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }
    cout<<"line:"<<__LINE__<<std::endl;
    //delete encrypt_buffer;
    //delete decrypt_buffer;
#endif



    //3.get input/output tensor
    s32Ret = MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);
    if (s32Ret == MI_SUCCESS) {
        for (int i = 0; i < desc.u32InputTensorCount; i++) {
            cout<<"input tensor["<<i<<"] name :"<<desc.astMI_InputTensorDescs[i].name<<endl;
        }
        for (int i = 0; i < desc.u32OutputTensorCount; i++) {
            cout<<"output tensor["<<i<<"] name :"<<desc.astMI_OutputTensorDescs[i].name<<endl;
        }
    }


    unsigned char *pu8ImageData = NULL;
    const char* dump_input_bin = getenv("DUMP_INPUT_BIN");
    MI_IPU_GetInputTensors( u32ChannelID, &InputTensorVector);
    int datasize = 0;
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
        int intResizeH = desc.astMI_InputTensorDescs[0].u32TensorShape[1];
        int intResizeW = desc.astMI_InputTensorDescs[0].u32TensorShape[2];
        int intResizeC = desc.astMI_InputTensorDescs[0].u32TensorShape[3];
        pu8ImageData = new unsigned char[intResizeH*intResizeW*intResizeC];

        PreProcessedData stProcessedData;
        stProcessedData.intResizeC = intResizeC;
        stProcessedData.intResizeH = intResizeH;
        stProcessedData.intResizeW = intResizeW;
        stProcessedData.pdata = pu8ImageData;
        stProcessedData.pImagePath = pImagePath;
        if(strncmp(pRGB,"RGB",sizeof("RGB"))==0)
        {
            bRGB = TRUE;
        }
        stProcessedData.bRGB = bRGB;
        GetImage(&stProcessedData);

        datasize=intResizeH*intResizeW*intResizeC;

    }

     memcpy(InputTensorVector.astArrayTensors[0].ptTensorData[0],pu8ImageData,datasize);
     MI_SYS_FlushInvCache(InputTensorVector.astArrayTensors[0].ptTensorData[0], datasize);

     MI_IPU_GetOutputTensors( u32ChannelID, &OutputTensorVector);
     if(dump_input_bin)
     {
         FILE* stream_input = fopen("inputtoinvoke.bin","w");
         int input_size = fwrite(InputTensorVector.astArrayTensors[0].ptTensorData[0],sizeof(unsigned char),datasize,stream_input);
         fclose(stream_input);

     }


    //4.invoke
    int times = 32;
    if(fps!=-1)
    {
     times =duration*fps;
    }

    printf("the times is %d \n",times);

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    for (int i=0;i<times;i++ )
    {
        struct timespec ts_start_1;
        clock_gettime(CLOCK_MONOTONIC, &ts_start_1);
        if(MI_SUCCESS!=MI_IPU_Invoke(u32ChannelID, &InputTensorVector, &OutputTensorVector))
        {
            cout<<"IPU invoke failed!!"<<endl;
            delete pu8ImageData;
            IPUDestroyChannel(u32ChannelID);
            MI_IPU_DestroyDevice();
            return -1;
        }

        struct timespec ts_end_1;
        clock_gettime(CLOCK_MONOTONIC, &ts_end_1);
        int elasped_time_1 = (ts_end_1.tv_sec-ts_start_1.tv_sec)*1000000+(ts_end_1.tv_nsec-ts_start_1.tv_nsec)/1000;
        float durationInus = 0.0;
        if(fps!=-1)
       {
        durationInus = 1000000.0/fps;
       }

        if ((elasped_time_1<durationInus)&&(fps!=-1))
        {
            usleep((int)(durationInus-elasped_time_1));
        }


    }

    clock_gettime(CLOCK_MONOTONIC, &ts_end);

    int elasped_time = (ts_end.tv_sec-ts_start.tv_sec)*1000000+(ts_end.tv_nsec-ts_start.tv_nsec)/1000;
    cout<<"fps:"<<1000.0/(float(elasped_time)/1000/times)<<std::endl;


    // show result of classify
    IPU_PrintOutputXOR(desc, OutputTensorVector);

    int s32TopN[5];
    memset(s32TopN,0,sizeof(s32TopN));
    int iDimCount = desc.astMI_OutputTensorDescs[0].u32TensorDim;
    int s32ClassCount  = 1;
    for(int i=0;i<iDimCount;i++ )
    {
      s32ClassCount *= desc.astMI_OutputTensorDescs[0].u32TensorShape[i];
    }
    float *pfData = (float *)OutputTensorVector.astArrayTensors[0].ptTensorData[0];

    cout<<"the class Count :"<<s32ClassCount<<std::endl;
    cout<<std::endl;
    cout<<std::endl;
    GetTopN(pfData, s32ClassCount, s32TopN, 5);


    for(int i=0;i<5;i++)
    {
      cout<<"order: "<<i+1<<" index: "<<s32TopN[i]<<" "<<pfData[s32TopN[i]]<<" "<<label[s32TopN[i]]<<endl;
    }


    //5. put intput tensor

    MI_IPU_PutInputTensors(u32ChannelID,&InputTensorVector);
    MI_IPU_PutOutputTensors(u32ChannelID,&OutputTensorVector);


    //6.destroy channel/device

   delete pu8ImageData;
   IPUDestroyChannel(u32ChannelID);
   MI_IPU_DestroyDevice();

    return 0;

}
