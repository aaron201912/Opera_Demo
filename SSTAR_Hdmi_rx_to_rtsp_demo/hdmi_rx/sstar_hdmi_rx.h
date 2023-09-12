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

#ifndef _SSTAR_HDMI_RX_H_
#define _SSTAR_HDMI_RX_H_

#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_hvp.h"
#include "mi_hvp_datatype.h"
#include "mi_hdmirx.h"
#include "mi_hdmirx_datatype.h"
#include "mi_venc.h"
#include "mi_venc_datatype.h"
#include "common.h"
#include <sstar_scl.h>
#include <sstar_venc.h>

#if defined (__cplusplus)
    extern "C" {
#endif

#define MAX_SYSCMD_LEN 256

extern int bHdmi_Rx_plug_Exit;

int sstar_init_hdmi_rx(buffer_object_t *buf_obj);

int sstar_deinit_hdmi_rx();


#if defined (__cplusplus)
    }
#endif

#endif

