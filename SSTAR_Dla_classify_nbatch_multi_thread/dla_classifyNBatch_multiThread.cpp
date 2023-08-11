/* Copyright (c) 2021-2022 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
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
#include <pthread.h>
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

static int SerializedReadFunc_1(void *dst_buf,int offset, int size, char *ctx)
{
// read data from buf
    return 0;
}

static int SerializedReadFunc_2(void *dst_buf,int offset, int size, char *ctx)
{
// read data from buf
    std::cout<<"read from call back function"<<std::endl;
    memcpy(dst_buf,ctx+offset,size);
    return 0;

}

MI_S32  IPUCreateDevice(char *pFirmwarePath,MI_U32 u32VarBufSize)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_IPU_DevAttr_t stDevAttr;
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;

    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, pFirmwarePath, 0);
    return s32Ret;
}

MI_S32  IPUCreateDevice_FromMemory(char *pFirmware,MI_U32 u32VarBufSize)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_IPU_DevAttr_t stDevAttr;
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;

    s32Ret = MI_IPU_CreateDevice(&stDevAttr, SerializedReadFunc_2, pFirmware, 0);
    return s32Ret;
}


MI_S32 IPUCreateChannel(MI_U32 *u32Channel, char *pModelImage)
{
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 0;
    stChnAttr.u32OutputBufDepth = 0;
    return MI_IPU_CreateCHN(u32Channel, &stChnAttr, NULL, pModelImage);
}




MI_S32 IPUCreateChannel_FromMemory(MI_U32 *u32Channel, char *pModelImage)
{
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 0;
    stChnAttr.u32OutputBufDepth = 0;

    return MI_IPU_CreateCHN(u32Channel, &stChnAttr, SerializedReadFunc_2, pModelImage);
}


MI_S32 IPUCreateChannel_FromUserMMAMemory(MI_U32 *u32Channel, MI_PHY u64ModelPA)
{
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 0;
    stChnAttr.u32OutputBufDepth = 0;

    return MI_IPU_CreateCHNWithUserMem(u32Channel, &stChnAttr, u64ModelPA);
}


MI_S32 IPUCreateChannel_FromEncryptFile(MI_U32 *u32Channel, char *pModelImage)
{
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 0;
    stChnAttr.u32OutputBufDepth = 0;

    return MI_IPU_CreateCHN(u32Channel, &stChnAttr, SerializedReadFunc_2, pModelImage);
}



MI_S32 IPUDestroyChannel(MI_U32 u32Channel)
{
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_IPU_DestroyCHN(u32Channel);
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

MI_S32 IPU_TensorMalloc(MI_IPU_Tensor_t* pTensor, MI_U32 BufSize)
{
    MI_S32 s32ret = 0;
    MI_PHY phyAddr = 0;
    void* pVirAddr = NULL;
    if(BufSize==0)
    {
        return -1;
    }
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

MI_S32 IPU_TensorFree(MI_IPU_Tensor_t* pTensor, MI_U32 BufSize)
{
    MI_S32 s32ret = 0;
    s32ret = MI_SYS_Munmap(pTensor->ptTensorData[0], BufSize);
    s32ret = MI_SYS_MMA_Free(0, pTensor->phyTensorAddr[0]);
    return s32ret;
}

MI_S32 IPU_VariableBufferMalloc(MI_PHY        *phyBufferAddr, MI_U32 BufSize)
{
    MI_S32 s32ret = 0;
    MI_PHY phyAddr = 0;

    if(BufSize==0)
    {
        return MI_SUCCESS;
    }
    s32ret = MI_SYS_MMA_Alloc(0, NULL, BufSize, &phyAddr);
    if (s32ret != MI_SUCCESS)
    {
        std::cerr << "Alloc buffer failed!" << std::endl;
        return s32ret;
    }

    *phyBufferAddr = phyAddr;
    return s32ret;
}

MI_S32 IPU_VariableBufferFree(MI_PHY       phyBufferAddr, MI_U32 BufSize)
{
    MI_S32 s32ret = 0;
    if(BufSize != 0)
    {
        s32ret = MI_SYS_MMA_Free(0, phyBufferAddr);
    }

    return s32ret;
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

static void IPU_PrintOutputXOR(MI_IPU_SubNet_InputOutputDesc_t desc, MI_IPU_BatchInvokeParam_t stInvokeParam, MI_BOOL bBatchOneBuf)
{
    MI_U32 u32InputNum = desc.u32InputTensorCount;
    MI_U32 u32OutputNum = desc.u32OutputTensorCount;
    MI_U32 u32BatchNum = stInvokeParam.u32BatchN;

    if(bBatchOneBuf)
    {
        volatile MI_U32 u32XORValue = 0;
        MI_U8 *pu8XORValue[4]= {(MI_U8 *)&u32XORValue,(MI_U8 *)&u32XORValue+1,(MI_U8 *)&u32XORValue+2,(MI_U8 *)&u32XORValue+3 };
        MI_U32 u32Count = 0;

        for (MI_U32 idxOutputNum = 0; idxOutputNum < desc.u32OutputTensorCount; idxOutputNum++)
        {
            for (MI_U32 idxBatchNum = 0; idxBatchNum < stInvokeParam.u32BatchN; idxBatchNum++)
            {
                MI_U8 u8Data = 0;
                MI_U8 *pu8Data = (MI_U8 *)stInvokeParam.astArrayTensors[idxOutputNum + u32InputNum*u32BatchNum].ptTensorData[0] + idxBatchNum * IPU_CalcTensorSize(desc.astMI_OutputTensorDescs[idxOutputNum]);
                for(int i = 0; i < IPU_CalcTensorSize(desc.astMI_OutputTensorDescs[idxOutputNum]); i++)
                {
                    u8Data = *(pu8Data + i);
                    *pu8XORValue[u32Count%4] ^= u8Data;
                    u32Count++;
                }
            }
        }
        printf("All outputs XOR = 0x%08x\n", u32XORValue);
    }
    else
    {
        for (MI_U32 idxBatchNum = 0; idxBatchNum < stInvokeParam.u32BatchN; idxBatchNum++)
        {
            volatile MI_U32 u32XORValue = 0;
            MI_U8 *pu8XORValue[4]= {(MI_U8 *)&u32XORValue,(MI_U8 *)&u32XORValue+1,(MI_U8 *)&u32XORValue+2,(MI_U8 *)&u32XORValue+3 };
            MI_U32 u32Count = 0;

            for (MI_U32 idxOutputNum = 0; idxOutputNum < desc.u32OutputTensorCount; idxOutputNum++)
            {
                MI_U8 u8Data = 0;
                MI_U8 *pu8Data = (MI_U8 *)stInvokeParam.astArrayTensors[idxBatchNum * desc.u32OutputTensorCount + idxOutputNum + u32InputNum*u32BatchNum].ptTensorData[0];
                for(int i = 0; i < IPU_CalcTensorSize(desc.astMI_OutputTensorDescs[idxOutputNum]); i++)
                {
                    u8Data = *(pu8Data + i);
                    *pu8XORValue[u32Count%4] ^= u8Data;
                    u32Count++;
                }
            }

            printf("All outputs XOR Batch[%d] = 0x%08x\n", idxBatchNum, u32XORValue);
        }
    }
}

static void IPU_Free_Outputtensor_Memory(MI_IPU_SubNet_InputOutputDesc_t desc, MI_IPU_BatchInvokeParam_t stInvokeParam,MI_U32 totalInputNumber)
{
    for (MI_U32 idxBatchNum = 0; idxBatchNum < stInvokeParam.u32BatchN; idxBatchNum++)
    {
        for (MI_U32 idxOutputNum = 0; idxOutputNum < desc.u32OutputTensorCount; idxOutputNum++)
        {
            IPU_TensorFree(&stInvokeParam.astArrayTensors[idxBatchNum * desc.u32OutputTensorCount + idxOutputNum + totalInputNumber], desc.astMI_OutputTensorDescs[idxOutputNum].s32AlignedBufSize);
        }
    }
}

static void IPU_Free_Inputtensor_Memory(MI_IPU_SubNet_InputOutputDesc_t desc, MI_IPU_BatchInvokeParam_t stInvokeParam)
{
    for (MI_U32 idxBatchNum = 0; idxBatchNum < stInvokeParam.u32BatchN; idxBatchNum++)
    {
        for (MI_U32 idxInputNum = 0; idxInputNum < desc.u32InputTensorCount; idxInputNum++)
        {
            IPU_TensorFree(&stInvokeParam.astArrayTensors[idxBatchNum * desc.u32InputTensorCount + idxInputNum], desc.astMI_InputTensorDescs[idxInputNum].s32AlignedBufSize);
        }
    }
}

static char * pImagePath1;
static char * pImagePath2;
static char * pRGB;
MI_BOOL bRGB = FALSE;
static int fps = -1;
static int duration = -1;
static char label[LABEL_CLASS_COUNT][LABEL_NAME_MAX_SIZE];
static MI_U32 u32ChannelID = 0;
static MI_IPU_OfflineModelStaticInfo_t OfflineModelInfo;

MI_S32 IPU_MallocBufferAndInvoke(char * pImagePath, MI_U32 u32IpuIndex)
{
    MI_IPU_BatchInvokeParam_t stInvokeParam;
    MI_IPU_RuntimeInfo_t stRuntimeInfo;
    MI_S32 s32Ret;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_PHY phyVariableBuffer = 0;

    //7. Get input/output tensor description (number and size)
    s32Ret = MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);
    if (s32Ret == MI_SUCCESS) {
        for (MI_U32 i = 0; i < desc.u32InputTensorCount; i++) {
            cout<<"input tensor["<<i<<"] name :"<<desc.astMI_InputTensorDescs[i].name<<endl;
        }
        for (MI_U32 i = 0; i < desc.u32OutputTensorCount; i++) {
            cout<<"output tensor["<<i<<"] name :"<<desc.astMI_OutputTensorDescs[i].name<<endl;
        }
    }

    //8. convert process for image inputs
    unsigned char *pu8ImageData = NULL;
    const char* dump_input_bin = getenv("DUMP_INPUT_BIN");

    int intResizeH = desc.astMI_InputTensorDescs[0].u32TensorShape[1];
    int intResizeW = desc.astMI_InputTensorDescs[0].u32TensorShape[2];
    int intResizeC = desc.astMI_InputTensorDescs[0].u32TensorShape[3];

    int datasize;
    if(strncmp(pRGB,"RAWDATA",sizeof("RAWDATA"))==0)
    {
        FILE* stream;
        stream = fopen(pImagePath,"r");
        fseek(stream, 0, SEEK_END);
        int length = ftell(stream);
        cout << "length==" << length<<endl;
        rewind(stream);

        pu8ImageData = new unsigned char[length];

        datasize = fread(pu8ImageData,sizeof(unsigned char),length,stream);
        cout << "size==" << datasize <<endl;
        fclose(stream);

    }
    else
    {

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

        datasize = intResizeH*intResizeW*intResizeC;

    }


    memset(&stInvokeParam, 0, sizeof(MI_IPU_BatchInvokeParam_t));
    memset(&stRuntimeInfo, 0, sizeof(MI_IPU_RuntimeInfo_t));

    //9. specify batch number
    stInvokeParam.u32BatchN = 3; // for example batch is 3

    //10. Alloc in/out tensors memory according to tensor num and size and batch number.
    //layout example: u32BatchN = 3; u32InputTensorCount = 2; u32OutputTensorCount = 2;
    // stInvokeParam.astArrayTensors[0]  for batch 0 input 0
    // stInvokeParam.astArrayTensors[1]  for batch 0 input 1
    // stInvokeParam.astArrayTensors[2]  for batch 1 input 0
    // stInvokeParam.astArrayTensors[3]  for batch 1 input 1
    // stInvokeParam.astArrayTensors[4]  for batch 2 input 0
    // stInvokeParam.astArrayTensors[5]  for batch 2 input 1
    // stInvokeParam.astArrayTensors[6]  for batch 0 output 0
    // stInvokeParam.astArrayTensors[7]  for batch 0 output 1
    // stInvokeParam.astArrayTensors[8]  for batch 1 output 0
    // stInvokeParam.astArrayTensors[9]  for batch 1 output 1
    // stInvokeParam.astArrayTensors[10] for batch 2 output 0
    // stInvokeParam.astArrayTensors[11] for batch 2 output 1
    MI_U32 totalInputNumber=0;
    for (MI_U32 idxBatchNum = 0; idxBatchNum < stInvokeParam.u32BatchN; idxBatchNum++)
    {
        for (MI_U32 idxInputNum = 0; idxInputNum < desc.u32InputTensorCount; idxInputNum++)
        {
            if (MI_SUCCESS != IPU_TensorMalloc(&stInvokeParam.astArrayTensors[idxBatchNum * desc.u32InputTensorCount + idxInputNum], desc.astMI_InputTensorDescs[idxInputNum].s32AlignedBufSize))
            {
                delete pu8ImageData;
                return -1;
            }
            totalInputNumber++;
            memcpy(stInvokeParam.astArrayTensors[idxBatchNum * desc.u32InputTensorCount + idxInputNum].ptTensorData[0],pu8ImageData,intResizeH*intResizeW*intResizeC);
            MI_SYS_FlushInvCache(stInvokeParam.astArrayTensors[idxBatchNum * desc.u32InputTensorCount + idxInputNum].ptTensorData[0], intResizeH*intResizeW*intResizeC);
        }
    }


    if(dump_input_bin)
    {
        std::string filename = "invoke_"+std::string(pImagePath)+".bin";
        FILE* stream_input = fopen(filename.c_str(),"w");
        int input_size = fwrite(stInvokeParam.astArrayTensors[0].ptTensorData[0],sizeof(unsigned char),datasize,stream_input);
        fclose(stream_input);

    }

    for (MI_U32 idxBatchNum = 0; idxBatchNum < stInvokeParam.u32BatchN; idxBatchNum++)
    {
        for (MI_U32 idxOutputNum = 0; idxOutputNum < desc.u32OutputTensorCount; idxOutputNum++)
        {
            if (MI_SUCCESS != IPU_TensorMalloc(&stInvokeParam.astArrayTensors[idxBatchNum * desc.u32OutputTensorCount + idxOutputNum + totalInputNumber], desc.astMI_OutputTensorDescs[idxOutputNum].s32AlignedBufSize))
            {
                IPU_Free_Inputtensor_Memory(desc, stInvokeParam);
                delete pu8ImageData;
                return -1;
            }
        }
    }

    //11. Alloc variable tensor memory
    // Attention! stInvokeParam only needs phyAddr for variable tensor. To avoid unexpected issues, don't remap to virtaddr if not necessary.
    if (MI_SUCCESS != IPU_VariableBufferMalloc(&phyVariableBuffer, OfflineModelInfo.u32VariableBufferSize))
    {
        IPU_Free_Outputtensor_Memory(desc, stInvokeParam,totalInputNumber);
        IPU_Free_Inputtensor_Memory(desc, stInvokeParam);
        delete pu8ImageData;
        return -1;
    }

    stInvokeParam.u32VarBufSize = OfflineModelInfo.u32VariableBufferSize;
    stInvokeParam.u64VarBufPhyAddr = phyVariableBuffer;
    //when using external user MMA buffer for variable tensor buffer, need to force IPU core
    //Two IPU cores using same variable buffer at same time is interdicted
    //  0 or (IPU_DEV_0|IPU_DEV_1): scheduled by ipu
    //  IPU_DEV_0: bind to ipu0
    //  IPU_DEV_1: bind to ipu1
    stInvokeParam.u32IpuAffinity = u32IpuIndex; //bind to specified IPU core

    //12. Invoke (forward model)
    struct  timespec    ts_start;
    struct  timespec    ts_end;

    int times = 1;
    if(fps!=-1)
    {
     times =duration*fps;
    }

    printf("the times is %d \n",times);
    clock_gettime(CLOCK_MONOTONIC, &ts_start);
    for (int i=0;i<times;i++ )
    {
        struct  timespec    ts_start_1;
        clock_gettime(CLOCK_MONOTONIC, &ts_start_1);

        if(MI_SUCCESS!=MI_IPU_Invoke2(u32ChannelID, &stInvokeParam, &stRuntimeInfo))
        {
            cout<<"IPU invoke failed!!"<<endl;

            IPU_VariableBufferFree(phyVariableBuffer, OfflineModelInfo.u32VariableBufferSize);
            IPU_Free_Outputtensor_Memory(desc, stInvokeParam,totalInputNumber);
            IPU_Free_Inputtensor_Memory(desc, stInvokeParam);

            delete pu8ImageData;
            return -1;
        }

        struct  timespec    ts_end_1;
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


    //13. Show classify result of first output of first batch
    IPU_PrintOutputXOR(desc, stInvokeParam, FALSE);
    int s32TopN[5];
    memset(s32TopN,0,sizeof(s32TopN));
    int iDimCount = desc.astMI_OutputTensorDescs[0].u32TensorDim;
    int s32ClassCount  = 1;
    for(int i=0;i<iDimCount;i++ )
    {
      s32ClassCount *= desc.astMI_OutputTensorDescs[0].u32TensorShape[i];
    }

    float *pfData = (float *)stInvokeParam.astArrayTensors[totalInputNumber].ptTensorData[0];

    cout<<"the class Count :"<<s32ClassCount<<std::endl;
    cout<<std::endl;
    cout<<std::endl;
    GetTopN(pfData, s32ClassCount, s32TopN, 5);


    for(int i=0;i<5;i++)
    {
      cout<<"order: "<<i+1<<" index: "<<s32TopN[i]<<" "<<pfData[s32TopN[i]]<<" "<<label[s32TopN[i]]<<endl;
    }

    //14. free variable buffer
    IPU_VariableBufferFree(phyVariableBuffer, OfflineModelInfo.u32VariableBufferSize);

    //15. free intput/output
    IPU_Free_Outputtensor_Memory(desc, stInvokeParam,totalInputNumber);
    IPU_Free_Inputtensor_Memory(desc, stInvokeParam);

    delete pu8ImageData;
    return 1;
}

void * IPU_InvokeThread1(void *arg)
{
    IPU_MallocBufferAndInvoke(pImagePath1,IPU_DEV_0);
    return NULL;
}

void * IPU_InvokeThread2(void *arg)
{
    IPU_MallocBufferAndInvoke(pImagePath2,IPU_DEV_1);
    return NULL;
}

int main(int argc,char *argv[])
{
    if ( argc < 6 )
    {
        std::cout << "USAGE: " << argv[0] <<": <xxxsgsimg.img>" \
        << "<picture1> <picture2> " << "<labels> "<< "<model intput_format:RGB or BGR>"<<std::endl;
        exit(0);
    } else {
         std::cout<<"model_img:"<<argv[1]<<std::endl;
         std::cout<<"picture:"<<argv[2]<<std::endl;
		 std::cout<<"picture:"<<argv[3]<<std::endl;
         std::cout<<"labels:"<<argv[4]<<std::endl;
         std::cout<<"model input_format:"<<argv[5]<<std::endl;
    }

    MI_IPU_BatchInvokeParam_t stInvokeParam;
    MI_IPU_RuntimeInfo_t stRuntimeInfo;
    MI_S32 ret;
    MI_PHY u64ModelPA;
    void *pModelVA;
    int fd, size;
    char * pFirmwarePath = NULL;
    char * pModelImgPath = argv[1];
    pImagePath1= argv[2];
    pImagePath2= argv[3];
    char * pLabelPath =argv[4];
    pRGB = argv[5];
    char * pfps = NULL;
    char * ptime = NULL;

    if (argc == 8)
    {
        pfps = argv[6];
        ptime = argv[7];
        fps = atoi(pfps);
        duration = atoi(ptime);
    }
    MI_S32 s32Ret;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_PHY phyVariableBuffer = 0;
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

    //1. Init system
    MI_SYS_Init(0);

    //2. Create device (no need for firmware memory)
    if(MI_SUCCESS !=IPUCreateDevice(0,0))
    {
        cout<<"create ipu device failed!"<<std::endl;
        return -1;

    }

    //3. alloc model MMA memory
    fd = open(pModelImgPath, O_RDONLY);
    if (fd < 0) {
        printf("fail to open %s\n", pModelImgPath);
        return -1;
    }
    size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    ret = MI_SYS_MMA_Alloc(0, NULL, size, &u64ModelPA);
    if (ret != MI_SUCCESS) {
        printf("fail to allocate buffer for model\n");
        return -1;
    }

    //4. map model MMA memory VA
    ret = MI_SYS_Mmap(u64ModelPA, size, &pModelVA, TRUE);
    if (ret != MI_SUCCESS)
    {
        printf("fail to mmap model\n");
        MI_SYS_MMA_Free(0, u64ModelPA);
        return -1;
    }
    int len = 0, read_size;
    while (len < size) {
        read_size = read(fd, (MI_U8 *)pModelVA+len, size-len);
        if (read_size < 0) {
            printf("read error in %s\n", pModelImgPath);
            close(fd);
            return read_size;
        }
        len += read_size;
    }
    MI_SYS_FlushInvCache(pModelVA, size);

    //5. Create channel from MMA PA model memory
    cout<<"create channel from memory__"<<std::endl;
    if(MI_SUCCESS!=IPUCreateChannel_FromUserMMAMemory(&u32ChannelID,u64ModelPA))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }

    //6. Get model variable tensor size.
    if (MI_SUCCESS != MI_IPU_GetOfflineModeStaticInfo(SerializedReadFunc_2, (char *)pModelVA, &OfflineModelInfo))
    {
        cout<<"get model variable buffer size failed!"<<std::endl;
        return -1;
    }

    //create two threads to invoke two different input pictures in loop on different IPU cores
    pthread_t tid1;
    pthread_create(&tid1,NULL,IPU_InvokeThread1,NULL);
    pthread_t tid2;
    pthread_create(&tid2,NULL,IPU_InvokeThread2,NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    //16.destroy channel/device/sys exit
    IPUDestroyChannel(u32ChannelID);
    MI_SYS_MMA_Free(0, u64ModelPA);
    MI_IPU_DestroyDevice();
    MI_SYS_Exit(0);
    return 0;

}
