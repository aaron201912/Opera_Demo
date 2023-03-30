#ifndef __SHOW_IPU_LOG_HEADER__
#define __SHOW_IPU_LOG_HEADER__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define IPU_LOG "/sys/dla/ipu_log"
#define IPU_LOG_OFF "ipu log is off"

#define IPU0 "[ipu0]"
#define IPU1 "[ipu1]"

#define CTRL "ctrl"
#define CORECTRL "corectrl"
#define ADDR "addr="
#define TOTAL_SIZE "total_size="
#define USED_SIZE "used_size="

#define MAX_IPU_CORE_NUM 2
#define CTRL_MODE 1
#define CORECTRL_MODE 2

struct single_log_info {
    uint64_t addr;
    uint32_t total_size;
    uint32_t used_size;
    uint32_t cfg;
};

struct ipu_log_info {
    struct single_log_info ctrl[MAX_IPU_CORE_NUM];
    struct single_log_info corectrl[MAX_IPU_CORE_NUM];
};

int parse_log_info(struct ipu_log_info *p_ipu_log_info, char *buf);
int save_ipu_log(struct ipu_log_info *p_ipu_log_info, const char *path, const char* strModel);

#endif  //__SHOW_IPU_LOG_HEADER__