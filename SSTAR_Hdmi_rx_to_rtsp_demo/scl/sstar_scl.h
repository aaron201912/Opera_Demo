#ifndef _SSTAR_SCL_H_
#define _SSTAR_SCL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mi_common_datatype.h"
#include "mi_scl.h"
#include "mi_scl_datatype.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "common.h"

typedef struct 
{
    MI_SCL_DEV scl_dev_id;
    MI_SCL_CHANNEL scl_chn_id;
    MI_SCL_PORT scl_outport;
    MI_SCL_HWSclId_e scl_hw_outport_mask;
    MI_SYS_Rotate_e scl_rotate;
    int scl_out_width;
    int scl_out_height;
    MI_SYS_ChnPort_t scl_src_chn_port;
    MI_U32 scl_src_module_inited;
    MI_U32 scl_src_fps;
    MI_U32 scl_dst_fps;
    MI_SYS_BindType_e scl_bind_type;
}sstar_scl_info_t;


/**************************************************
* Init scl
* @param            \b IN: scl info
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
MI_S32 sstar_scl_init(sstar_scl_info_t *scl_info);

/**************************************************
* Deinit scl
* @param            \b IN: buffer object
* @return           \b OUT: 0:   success
                           -1:   fail
**************************************************/
MI_S32 sstar_scl_deinit(sstar_scl_info_t *scl_info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SSTAR_SCL_H_ */