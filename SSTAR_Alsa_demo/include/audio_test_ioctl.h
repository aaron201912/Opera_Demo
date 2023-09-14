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

#ifndef _AUDIO_TEST_IOCTL_H_
#define _AUDIO_TEST_IOCTL_H_

#define AUDIO_IOCTL_MAGIC   'S'

#define AUDRV_PCM_TEST          _IOW(AUDIO_IOCTL_MAGIC, 0x0, struct AUD_PcmCfg_s)
#define AUDRV_SET_DPGA_GAIN     _IOW(AUDIO_IOCTL_MAGIC, 0x1, struct AUD_GainIfCfg_s)
#define AUDRV_SET_ADC_GAIN      _IOW(AUDIO_IOCTL_MAGIC, 0x2, struct AUD_GainIfCfg_s)
#define AUDRV_SET_DMIC_GAIN     _IOW(AUDIO_IOCTL_MAGIC, 0x3, struct AUD_GainCfg_s)
#define AUDRV_BENCH_AIO_START   _IO(AUDIO_IOCTL_MAGIC, 0x4)
#define AUDRV_BENCH_AIO_STOP    _IO(AUDIO_IOCTL_MAGIC, 0x5)
#define AUDRV_BENCH_READ        _IOW(AUDIO_IOCTL_MAGIC, 0x6, struct AUD_BenchRead_s)
#define AUDRV_DMA_SINEGEN       _IOW(AUDIO_IOCTL_MAGIC, 0x7, struct AUD_SineGen_Cfg_s)
#define AUDRV_AO_ATTACH         _IOW(AUDIO_IOCTL_MAGIC, 0x8, struct AUD_AO_Attach_s)
#define AUDRV_AO_DETACH         _IOW(AUDIO_IOCTL_MAGIC, 0x9, struct AUD_AO_Attach_s)
#define AUDRV_DMA_PAUSE         _IO(AUDIO_IOCTL_MAGIC, 0xa)
#define AUDRV_DMA_RESUME        _IO(AUDIO_IOCTL_MAGIC, 0xb)
#define AUDRV_DMA_STOP          _IO(AUDIO_IOCTL_MAGIC, 0xc)
#define AUDRV_DMA_START         _IO(AUDIO_IOCTL_MAGIC, 0xd)
#define AUDRV_INTERFACE_MUTE    _IOW(AUDIO_IOCTL_MAGIC, 0xe, struct AUD_InterfaceMuteCfg_s)
#define AUDRV_BENCH_AIO_ATTACH  _IOW(AUDIO_IOCTL_MAGIC, 0xf, struct AUD_PcmCfg_s)
#define AUDRV_AI_ATTACH         _IOW(AUDIO_IOCTL_MAGIC, 0x10, struct AUD_AI_Attach_s)
#define AUDRV_DPGA_MUTE         _IOW(AUDIO_IOCTL_MAGIC, 0x11, struct AUD_DpgaMuteCfg_s)
#define AUDRV_CHANNEL_MODE      _IOW(AUDIO_IOCTL_MAGIC, 0x12, struct AUD_ChanModeCfg_s)
#define AUDRV_MIXER_SETTING     _IOW(AUDIO_IOCTL_MAGIC, 0x13, struct AUD_MixerCfg_s)

#define IOCTL_AUDIO_MAXNR 0x1f


#endif /* _AUDIO_TEST_IOCTL_H_ */
