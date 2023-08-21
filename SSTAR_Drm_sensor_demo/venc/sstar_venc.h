#ifndef _SSTAR_VENC_H_
#define _SSTAR_VENC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mi_common_datatype.h"
#include "mi_venc.h"
#include "mi_venc_datatype.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "common.h"

typedef struct 
{
    MI_VENC_DEV venc_dev_id;
    MI_VENC_CHN venc_chn_id;
    MI_S32 venc_outport;
    int venc_out_width;
    int venc_out_height;
    MI_SYS_ChnPort_t venc_src_chn_port;
    MI_U32 venc_src_fps;
    MI_U32 venc_dst_fps;
    MI_SYS_BindType_e venc_bind_type;
}sstar_venc_info_t;


/**************************************************
* Init venc
* @param            \b IN: venc info
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
MI_S32 sstar_venc_init(sstar_venc_info_t *venc_info);

/**************************************************
* Deinit venc
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
MI_S32 sstar_venc_deinit(sstar_venc_info_t *venc_info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SSTAR_VENC_H_ */