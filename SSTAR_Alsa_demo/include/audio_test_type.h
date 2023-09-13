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

#ifndef _AUDIO_TEST_TYPE_H_
#define _AUDIO_TEST_TYPE_H_

#define AUD_AIO_BIT0                           0x00000001
#define AUD_AIO_BIT1                           0x00000002
#define AUD_AIO_BIT2                           0x00000004
#define AUD_AIO_BIT3                           0x00000008
#define AUD_AIO_BIT4                           0x00000010
#define AUD_AIO_BIT5                           0x00000020
#define AUD_AIO_BIT6                           0x00000040
#define AUD_AIO_BIT7                           0x00000080
#define AUD_AIO_BIT8                           0x00000100
#define AUD_AIO_BIT9                           0x00000200
#define AUD_AIO_BIT10                          0x00000400
#define AUD_AIO_BIT11                          0x00000800
#define AUD_AIO_BIT12                          0x00001000
#define AUD_AIO_BIT13                          0x00002000
#define AUD_AIO_BIT14                          0x00004000
#define AUD_AIO_BIT15                          0x00008000
#define AUD_AIO_BIT16                          0x00010000
#define AUD_AIO_BIT17                          0x00020000
#define AUD_AIO_BIT18                          0x00040000
#define AUD_AIO_BIT19                          0x00080000
#define AUD_AIO_BIT20                          0x00100000
#define AUD_AIO_BIT21                          0x00200000
#define AUD_AIO_BIT22                          0x00400000
#define AUD_AIO_BIT23                          0x00800000
#define AUD_AIO_BIT24                          0x01000000
#define AUD_AIO_BIT25                          0x02000000
#define AUD_AIO_BIT26                          0x04000000
#define AUD_AIO_BIT27                          0x08000000
#define AUD_AIO_BIT28                          0x10000000
#define AUD_AIO_BIT29                          0x20000000
#define AUD_AIO_BIT30                          0x40000000
#define AUD_AIO_BIT31                          0x80000000

#define AUD_AIO_MAX_AO_INTERFACE               6 // ADC_AB + ADC_CD + ECHO_A + HDMI_A +I2S_A + I2S_B

typedef struct AUD_I2sCfg_s
{
    int nTdmMode;
    int nMsMode;
    int nBitWidth;
    int nFmt;
    int u16Channels;
    int e4WireMode;
    int eMck;
    int nRate;
}_AUD_I2sCfg_s;

typedef enum
{
    AUD_AI_DMA_CH_SLOT_0_1,
    AUD_AI_DMA_CH_SLOT_2_3,
    AUD_AI_DMA_CH_SLOT_4_5,
    AUD_AI_DMA_CH_SLOT_6_7,
    AUD_AI_DMA_CH_SLOT_TOTAL,

} _AUD_AI_DmChSlot_e;

typedef enum
{
    AUD_AO_DMA_CH_SLOT_0,
    AUD_AO_DMA_CH_SLOT_1,
    AUD_AO_DMA_CH_SLOT_TOTAL,

} _AUD_AO_DmaChSlot_e;

typedef enum
{
    AUD_AI_DMA_A,
    AUD_AI_DMA_B,
    AUD_AI_DMA_C,
    AUD_AI_DMA_D,
    AUD_AI_DMA_E,
    AUD_AI_DMA_DIRECT_A,
    AUD_AI_DMA_DIRECT_B,
    AUD_AI_DMA_TOTAL,

} _AUD_AI_Dma_e;

typedef enum
{
    AUD_AI_IF_NONE = 0,
    AUD_AI_IF_ADC_A_0_B_0,
    AUD_AI_IF_ADC_C_0_D_0,
    AUD_AI_IF_DMIC_A_0_1,
    AUD_AI_IF_DMIC_A_2_3,
    AUD_AI_IF_HDMI_A_0_1,
    AUD_AI_IF_ECHO_A_0_1,
    AUD_AI_IF_I2S_RX_A_0_1,
    AUD_AI_IF_I2S_RX_A_2_3,
    AUD_AI_IF_I2S_RX_A_4_5,
    AUD_AI_IF_I2S_RX_A_6_7,
    AUD_AI_IF_I2S_RX_A_8_9,
    AUD_AI_IF_I2S_RX_A_10_11,
    AUD_AI_IF_I2S_RX_A_12_13,
    AUD_AI_IF_I2S_RX_A_14_15,
    AUD_AI_IF_I2S_RX_B_0_1,
    AUD_AI_IF_I2S_RX_B_2_3,
    AUD_AI_IF_I2S_RX_B_4_5,
    AUD_AI_IF_I2S_RX_B_6_7,
    AUD_AI_IF_I2S_RX_B_8_9,
    AUD_AI_IF_I2S_RX_B_10_11,
    AUD_AI_IF_I2S_RX_B_12_13,
    AUD_AI_IF_I2S_RX_B_14_15,
    AUD_AI_IF_I2S_RX_C_0_1,
    AUD_AI_IF_I2S_RX_C_2_3,
    AUD_AI_IF_I2S_RX_C_4_5,
    AUD_AI_IF_I2S_RX_C_6_7,
    AUD_AI_IF_I2S_RX_C_8_9,
    AUD_AI_IF_I2S_RX_C_10_11,
    AUD_AI_IF_I2S_RX_C_12_13,
    AUD_AI_IF_I2S_RX_C_14_15,
    AUD_AI_IF_I2S_RX_D_0_1,
    AUD_AI_IF_I2S_RX_D_2_3,
    AUD_AI_IF_I2S_RX_D_4_5,
    AUD_AI_IF_I2S_RX_D_6_7,
    AUD_AI_IF_I2S_RX_D_8_9,
    AUD_AI_IF_I2S_RX_D_10_11,
    AUD_AI_IF_I2S_RX_D_12_13,
    AUD_AI_IF_I2S_RX_D_14_15,

} _AUD_AI_IF_e;

typedef enum
{
    AUD_AO_IF_NONE              = 0,
    AUD_AO_IF_DAC_A_0           = AUD_AIO_BIT0,
    AUD_AO_IF_DAC_B_0           = AUD_AIO_BIT1,
    AUD_AO_IF_DAC_C_0           = AUD_AIO_BIT2,
    AUD_AO_IF_DAC_D_0           = AUD_AIO_BIT3,
    AUD_AO_IF_HDMI_A_0          = AUD_AIO_BIT4,
    AUD_AO_IF_HDMI_A_1          = AUD_AIO_BIT5,
    AUD_AO_IF_ECHO_A_0          = AUD_AIO_BIT6,
    AUD_AO_IF_ECHO_A_1          = AUD_AIO_BIT7,
    AUD_AO_IF_I2S_TX_A_0        = AUD_AIO_BIT8,
    AUD_AO_IF_I2S_TX_A_1        = AUD_AIO_BIT9,
    AUD_AO_IF_I2S_TX_B_0        = AUD_AIO_BIT10,
    AUD_AO_IF_I2S_TX_B_1        = AUD_AIO_BIT11,

} _AUD_AO_IF_e;

typedef enum
{
    AUD_AO_DMA_A,
    AUD_AO_DMA_B,
    AUD_AO_DMA_C,
    AUD_AO_DMA_D,
    AUD_AO_DMA_E,
    AUD_AO_DMA_DIRECT_A,
    AUD_AO_DMA_DIRECT_B,
    AUD_AO_DMA_TOTAL,

} _AUD_AO_Dma_e;

typedef enum
{
    AUD_SINE_GEN_AI_DMA_A,
    AUD_SINE_GEN_AI_DMA_B,
    AUD_SINE_GEN_AI_DMA_C,
    AUD_SINE_GEN_AI_DMA_D,
    AUD_SINE_GEN_AO_DMA_A,
    AUD_SINE_GEN_AO_DMA_B,
    AUD_SINE_GEN_AO_DMA_C,
    AUD_SINE_GEN_AO_DMA_D,
    AUD_SINE_GEN_AI_IF_DMIC_A,
    AUD_SINE_GEN_TOTAL,

} _AUD_SineGen_e;

typedef enum
{
    AUD_SINE_GEN_FREQ_HZ_250,
    AUD_SINE_GEN_FREQ_HZ_500,
    AUD_SINE_GEN_FREQ_HZ_1000,
    AUD_SINE_GEN_FREQ_HZ_1500,
    AUD_SINE_GEN_FREQ_HZ_2000,
    AUD_SINE_GEN_FREQ_HZ_3000,
    AUD_SINE_GEN_FREQ_HZ_4000,
    AUD_SINE_GEN_FREQ_HZ_6000,
    AUD_SINE_GEN_FREQ_HZ_8000,
    AUD_SINE_GEN_FREQ_HZ_12000,
    AUD_SINE_GEN_FREQ_HZ_16000,
    AUD_SINE_GEN_FREQ_HZ_TOTAL,

} _AUD_SineGen_Freq_e;

typedef enum
{
    AUD_AO_CH_MODE_STEREO = 0,
    AUD_AO_CH_MODE_DOUBLE_MONO,
    AUD_AO_CH_MODE_DOUBLE_LEFT,
    AUD_AO_CH_MODE_DOUBLE_RIGHT,
    AUD_AO_CH_MODE_EXCHANGE,
    AUD_AO_CH_MODE_ONLY_LEFT,
    AUD_AO_CH_MODE_ONLY_RIGHT,

} _AUD_AO_ChMode_e;

typedef struct AUD_SineGen_Cfg_s
{
    _AUD_SineGen_Freq_e enFreq; // For 48KHz sample rate, other sample rate must calulate by ratio, eg: 16KHz -> (freq * 16 / 48)
    _AUD_SineGen_e enSineGen;
    char s8Gain;               // 0: 0dB(maximum), 1: -6dB, ... (-6dB per step)
    char bEn;
} AUD_SineGen_Cfg_s;

typedef struct AUD_AI_Attach_s
{
    _AUD_AI_Dma_e enAiDma;
    _AUD_AI_IF_e aenAiIf[AUD_AI_DMA_CH_SLOT_TOTAL];
    _AUD_AI_IF_e aenAiIfFor16ChI2s[AUD_AI_DMA_CH_SLOT_TOTAL];

} AUD_AI_Attach_t;

typedef struct AUD_AO_Attach_s
{
    _AUD_AO_Dma_e enAoDma;
    unsigned char nAoDmaCh;
    _AUD_AO_IF_e enAoIf[AUD_AIO_MAX_AO_INTERFACE];
    char nAoAttachNum;

} AUD_AO_Attach_t;

typedef struct AUD_TestGainCfg_s
{
    char aiIf;
    int nIfGain[2];
    int nDpgaGain[2];
    char nGainFading;
    char nGainEn;
}_AUD_TestGainCfg_s;

typedef struct AUD_PcmCfg_s
{
    int nRate;
    int nBitWidth;
    int n16Channels;
    int nInterleaved;
    int n32PeriodSize; //bytes
    int n32StartThres;
    int nTimeMs;
    int nI2sConfig;
    int n8IsOnlyEvenCh;
    int nWrSize;
    int nChMode;
    _AUD_I2sCfg_s sI2s;
    AUD_AI_Attach_t aiAttach;
    AUD_AO_Attach_t aoAttachL;
    AUD_AO_Attach_t aoAttachR;
    _AUD_TestGainCfg_s sGainCfg;
    int nSpecialDumpChannels;
    char aoUseFile;
    char file_name[100];
}_AUDPcmCfg_t;

typedef struct AUD_GainCfg_s
{
    int s16Gain;
    char s8Ch;
}_AUD_GainCfg_s;

typedef struct AUD_GainIfCfg_s
{
    char aiIf;
    char nLeftGain;
    char nRightGain;
}_AUD_GainIfCfg_s;

// Bench
typedef struct AUD_BenchRead_s
{
    void *aBufAddr;
    unsigned int nLen;
}_AUD_BenchRead_s;

typedef struct AUD_InterfaceMuteCfg_s
{
    _AUD_AI_IF_e aiIf;
    _AUD_AO_IF_e aoIf;
    char sEnL;
    char sEnR;
}_AUD_InterfaceMuteCfg_s;

typedef struct AUD_DpgaMuteCfg_s
{
    char isMute;
    char s8Ch;
}_AUD_DpgaMuteCfg_s;

typedef struct AUD_ChanModeCfg_s
{
    char nChanMode;
}_AUD_ChanModeCfg_s;

typedef struct AUD_MixerCfg_s
{
    int nMixerNumber;
    int nMixerRate;
}_AUD_MixerCfg_s;


#endif /* _AUDIO_TEST_TYPE_H_ */
