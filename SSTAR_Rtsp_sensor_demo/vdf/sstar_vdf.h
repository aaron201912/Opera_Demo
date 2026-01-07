#ifndef _SSTAR_VDF_H_
#define _SSTAR_VDF_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_vdf.h"
#include "mi_vdf_datatype.h"
#include "common.h"

int sstar_vdf_init(MI_U32 width, MI_U32 height);
int sstar_vdf_get_result(MI_VDF_Result_t *pstVdfResult);
int sstar_vdf_put_result(MI_VDF_Result_t *pstVdfResult);
int sstar_vdf_deinit();
int sstar_vdf_bind_scl(MI_SYS_ChnPort_t scl_src_chn_port);
int sstar_vdf_unbind_scl(MI_SYS_ChnPort_t scl_src_chn_port);
int sstar_vdf_get_input_buff(MI_SYS_BUF_HANDLE* bufHandle, MI_U32 width,  MI_U32 height, MI_SYS_BufInfo_t* stBufInfo);
int sstar_vdf_put_input_buff(MI_SYS_BUF_HANDLE bufHandle, MI_SYS_BufInfo_t* stBufInfo);
void * sstar_vdf_feed_buff_thread(void * param);
void * sstar_vdf_get_result_thread(void * param);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SSTAR_VDF_H_ */