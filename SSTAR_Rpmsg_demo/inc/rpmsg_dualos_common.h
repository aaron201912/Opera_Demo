/*
* sstar_rpmsg.h- Sigmastar
*
* Copyright (c) [2019~2020] SigmaStar Technology.
*
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License version 2 for more details.
*
*/

/*
 * @file rpmsg_dualos.h
 *
 * @brief Global header file for sstar rpmsg_dualos
 *
 * @ingroup rpmsg_dualos
 */
#ifndef __RPMSG_DUALOS_COMMON_H__
#define __RPMSG_DUALOS_COMMON_H__

#define SSTAR_MAX_OS_NUM        (2)
#define SSTAR_MAX_SOC_NUM       (1)

#define EPT_TYPE_SIGMASTAR      0x0
#define EPT_TYPE_CUSTOMER       0x1

#define EPT_SOC_DEFAULT         0x0
#define EPT_OS_LINUX            0x0
#define EPT_OS_RTOS             0x1

#define EPT_CLASS_RESERVED      0x0
#define EPT_CLASS_MI            0x1
#define EPT_CLASS_MISC          0x2

#define EPT_TYPE(x)             ((x & 0x1) << 30)
#define EPT_SOC(x)              ((x & 0x7) << 27)
#define EPT_OS(x)               ((x & 0x3) << 25)
#define EPT_CLASS(x)            ((x & 0x1f) << 20)

#define EPT_TYPE_VAL(x)         ((x & 0x40000000) >> 30)
#define EPT_SOC_VAL(x)          ((x & 0x38000000) >> 27)
#define EPT_OS_VAL(x)           ((x & 0x06000000) >> 25)
#define EPT_CLASS_VAL(x)        ((x & 0x01f00000) >> 20)

#define EPT_MI_MODULE_VAL(x)    ((x & 0x000ff000) >> 12)
#define EPT_MI_DEVICE_VAL(x)    ((x & 0x00000f00) >> 8))
#define EPT_MI_CHANNEL_VAL(x)   ((x & 0x0000000f) >> 0))

#define EPT_IS_VALID(x)         (!(x & 0x80000000))

#define EPT_ADDR_PREFIX(t, s, o)    (EPT_TYPE(t) | EPT_SOC(s) | \
                                        EPT_OS(o))

#endif /* __RPMSG_DUALOS_COMMON_H__ */
