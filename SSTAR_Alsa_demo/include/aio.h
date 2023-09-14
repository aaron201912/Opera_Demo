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
#ifndef _AIO_H_
#define _AIO_H_

//=============================================================================
// Include files
//=============================================================================

/* Use the newer ALSA API */
#define ALSA_PCM_NEW_HW_PARAMS_API
/* All of the ALSA library API is defined in this header */
#include <alsa/asoundlib.h>

typedef unsigned long long ss_phys_addr_t;
typedef unsigned long long ss_miu_addr_t;

//-------------------------------------------------------------------------------------------------
//  System Data Type
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Moudle common  type
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Macros
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Defines
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Structures
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Enum
//-------------------------------------------------------------------------------------------------

// ao
typedef enum
{
    E_MODE_STEREO = 0,
#if defined(CONFIG_SIGMASTAR_CHIP_P5)
    E_MODE_DOUBLE_MONO,
    E_MODE_DOUBLE_LEFT,
    E_MODE_DOUBLE_RIGHT,
    E_MODE_EXCHANGE,
    E_MODE_ONLY_LEFT,
    E_MODE_ONLY_RIGHT,
#endif
} CHANNEL_MODE_E;

typedef enum
{
    E_AO_DETACH  = 0,
    E_AO_VIR_DETACH = E_AO_DETACH,
#if defined(CONFIG_SIGMASTAR_CHIP_P5)
    E_AO_SEL_1 = 1,
    E_AO_DMA      = E_AO_SEL_1,
    E_AO_LDMA     = E_AO_SEL_1,
    E_AO_DAC0     = E_AO_SEL_1,

    E_AO_SEL_2      = 2,
    E_AO_DMA_SRC       = E_AO_SEL_2,
    E_AO_LDMA_SRC      = E_AO_SEL_2,
    E_AO_SPDIF_LPCM_TX = E_AO_SEL_2,

    E_AO_SEL_3       = 3,
    E_AO_NLDMA          = E_AO_SEL_3,
    E_AO_SPDIF_NLPCM_TX = E_AO_SEL_3,

    E_AO_SEL_4 = 4,
    E_AO_I2S_TXA,

    E_AO_SEL_5 = 5,
    E_AO_I2S_TXB,

    E_AO_SEL_6 = 6,
    E_AO_ECHO_0,
#endif
} AO_SEL_E;

// ai
typedef enum
{
    E_AI_DETACH = 0,
#if defined(CONFIG_SIGMASTAR_CHIP_P5)
    E_AI_ADC_A,
    E_AI_ADC_B,
    E_AI_DMIC_A_01,
    E_AI_DMIC_A_23,
    E_AI_DMIC_A_45,
    E_AI_DMIC_A_67,
    E_AI_ECHO_01,
    E_AI_HDMI_01,
    E_AI_I2S_RXA_0_1,
    E_AI_I2S_RXA_2_3,
    E_AI_I2S_RXA_4_5,
    E_AI_I2S_RXA_6_7,
    E_AI_I2S_RXB_0_1,
#endif
} AI_MULTICHANNEL_SEL_E;

typedef enum
{
    E_AIO_IF_MIN = 0,
    // for ao
    E_AO_IF_MIN = E_AIO_IF_MIN,
#if defined(CONFIG_SIGMASTAR_CHIP_P5)
    E_AO_IF_VIR_MUX = 0x1,
    E_AO_IF_HDMI_TX = 0x2,
    E_AO_IF_DAC = 0x4,
    E_AO_IF_ECHO = 0x8,
    E_AO_IF_SPDIF_TX = 0x10,
    E_AO_IF_I2S_TX_A = 0x20,
    E_AO_IF_I2S_TX_B = 0x40,
#endif
    E_AO_IF_MAX,
    // for ai
    E_AI_IF_MIN = E_AO_IF_MAX,
#if defined(CONFIG_SIGMASTAR_CHIP_P5)
    E_AI_IF_MCH_01 = 0x10000,
    E_AI_IF_MCH_23 = 0x20000,
    E_AI_IF_MCH_45 = 0x40000,
    E_AI_IF_MCH_67 = 0x80000,
    E_AI_IF_MCH_89 = 0x100000,
    E_AI_IF_MCH_AB = 0x200000,
    E_AI_IF_MCH_CD = 0x400000,
    E_AI_IF_MCH_EF = 0x800000,
    E_AI_IF_MCH_VIR = 0x1000000,
#endif
    E_AI_IF_MAX,
    E_AIO_IF_MAX = E_AI_IF_MAX,
} AIO_IF_E;

//=============================================================================
// Extern definition
//=============================================================================

//=============================================================================
// Macro definition
//=============================================================================

//=============================================================================
// Data type definition
//=============================================================================

#if defined(CONFIG_SIGMASTAR_CHIP_P5)

#define CARD_NUM (2)
#define DEVICE_NUM (1)

#define DEFAULT_PERIOD_SIZE  (1024)
#define DEFAULT_PERIOD_COUNT (4)

#define DPGA_VOL_MIN (0)
#define DPGA_VOL_MAX (1023)

// ao mixer control name
#define CHANNEL_MODE "CHANNEL_MODE_PLAYBACK"

#define DAC_L_VOL      "DAC_0 Playback Volume"
#define DAC_R_VOL      "DAC_1 Playback Volume"
#define I2S_TXA_L_VOL  "I2S_TXA_0 Playback Volume"
#define I2S_TXA_R_VOL  "I2S_TXA_1 Playback Volume"
#define I2S_TXB_L_VOL  "I2S_TXB_0 Playback Volume"
#define I2S_TXB_R_VOL  "I2S_TXB_1 Playback Volume"
#define SPDIF_TX_L_VOL "SPDIF_TX_0 Playback Volume"
#define SPDIF_TX_R_VOL "SPDIF_TX_1 Playback Volume"

#define AO_VIR   "AO_VIR_MUX_SEL"
#define HDMI_TX  "HDMI_TX_SEL"
#define DAC      "DAC_SEL"
#define ECHO_TX  "ECHO_SEL"
#define SPDIF_TX "SPDIF_TX_SEL"
#define I2S_TXA  "I2S_TXA_SEL"
#define I2S_TXB  "I2S_TXB_SEL"

// ai mixer control name
#define ADC_A_L_VOL "ADC_A_0 Capture Volume"
#define ADC_A_R_VOL "ADC_A_1 Capture Volume"

#define ADC_B_L_VOL "ADC_B_0 Capture Volume"
#define ADC_B_R_VOL "ADC_B_1 Capture Volume"

#define DMIC_0_VOL "DMIC_0 Capture Volume"
#define DMIC_1_VOL "DMIC_1 Capture Volume"
#define DMIC_2_VOL "DMIC_2 Capture Volume"
#define DMIC_3_VOL "DMIC_3 Capture Volume"
#define DMIC_4_VOL "DMIC_4 Capture Volume"
#define DMIC_5_VOL "DMIC_5 Capture Volume"
#define DMIC_6_VOL "DMIC_6 Capture Volume"
#define DMIC_7_VOL "DMIC_7 Capture Volume"

#define I2S_RXA_0_VOL "I2S_RXA_0 Capture Volume"
#define I2S_RXA_1_VOL "I2S_RXA_1 Capture Volume"
#define I2S_RXA_2_VOL "I2S_RXA_2 Capture Volume"
#define I2S_RXA_3_VOL "I2S_RXA_3 Capture Volume"
#define I2S_RXA_4_VOL "I2S_RXA_4 Capture Volume"
#define I2S_RXA_5_VOL "I2S_RXA_5 Capture Volume"
#define I2S_RXA_6_VOL "I2S_RXA_6 Capture Volume"
#define I2S_RXA_7_VOL "I2S_RXA_7 Capture Volume"

#define I2S_RXB_0_VOL "I2S_RXB_0 Capture Volume"
#define I2S_RXB_1_VOL "I2S_RXB_1 Capture Volume"

#define ECHO_RX_VOL "ECHO Capture Volume"

#define AI_MCH_01  "AI_MCH_01_SEL"
#define AI_MCH_23  "AI_MCH_23_SEL"
#define AI_MCH_45  "AI_MCH_45_SEL"
#define AI_MCH_67  "AI_MCH_67_SEL"
#define AI_MCH_89  "AI_MCH_89_SEL"
#define AI_MCH_AB  "AI_MCH_AB_SEL"
#define AI_MCH_CD  "AI_MCH_CD_SEL"
#define AI_MCH_EF  "AI_MCH_EF_SEL"
#define AI_MCH_VIR "AI_MCH_VIR_SEL"

#endif

//=============================================================================
// Variable definition
//=============================================================================

//=============================================================================
// Global function definition
//=============================================================================

//-------------------------------------------------------------------------------------------------
//  global function  prototypes
//-------------------------------------------------------------------------------------------------

static int aio_control_contents_set_by_name(unsigned int card_id, char *name, char *value);

#endif /* _AIO_H_ */
