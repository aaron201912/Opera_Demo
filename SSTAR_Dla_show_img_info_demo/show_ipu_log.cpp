#include "show_ipu_log.h"
#include "mi_sys.h"

//[ipu0] ctrl addr=400341000 total_size=8388608 used_size=0 corectrl addr=401341000 total_size=8388608 used_size=0 ctrl=ffffff cctrl=1fff
//[ipu1] ctrl addr=400b41000 total_size=8388608 used_size=0 corectrl addr=401b41000 total_size=8388608 used_size=0 ctrl=ffffff cctrl=1fff


int parse_log_info(struct ipu_log_info *p_ipu_log_info, char *buf)
{
    int i = 0, mode = 0, core = -1;
    size_t index;
    uint64_t tmp;
    char cmd[64];

    while(buf[i]) {
        index = 0;
        for (; buf[i] != ' ' && buf[i] != '\n' && index < sizeof(cmd)-1; i++) {
            cmd[index++] = buf[i];
        }
        i++;
        cmd[index] = 0;
        if (!cmd[0]) {
            continue;
        }

        if (!strncmp(cmd, IPU0, strlen(IPU0))) {
            core = 0;
        } else if (!strncmp(cmd, IPU1, strlen(IPU1))) {
            core = 1;
        } else if (!strcmp(cmd, CTRL)) {
            mode = CTRL_MODE;
        } else if (!strcmp(cmd, CORECTRL)) {
            mode = CORECTRL_MODE;
        } else if (!strncmp(cmd, ADDR, strlen(ADDR))) {
            tmp = strtoull(cmd+strlen(ADDR), NULL, 16);
            if (tmp == ~0ULL) {
                printf("invalid argument: %s\n", buf);
                return -1;
            }
            if (core < 0) {
                printf("invalid argument: %s\n", buf);
                return -1;
            }
            if (mode == CTRL_MODE) {
                p_ipu_log_info->ctrl[core].addr = tmp;
            } else if (mode == CORECTRL_MODE) {
                p_ipu_log_info->corectrl[core].addr = tmp;
            } else {
                printf("invalid argument: %s\n", buf);
                return -1;
            }
        } else if (!strncmp(cmd, TOTAL_SIZE, strlen(TOTAL_SIZE))) {
            if (core < 0) {
                printf("invalid argument: %s\n", buf);
                return -1;
            }
            if (mode == CTRL_MODE) {
                p_ipu_log_info->ctrl[core].total_size = atoi(cmd+strlen(TOTAL_SIZE));
            } else if (mode == CORECTRL_MODE) {
                p_ipu_log_info->corectrl[core].total_size = atoi(cmd+strlen(TOTAL_SIZE));
            } else {
                printf("invalid argument: %s\n", buf);
                return -1;
            }
        } else if (!strncmp(cmd, USED_SIZE, strlen(USED_SIZE))) {
            if (core < 0) {
                printf("invalid argument: %s\n", buf);
                return -1;
            }
            if (mode == CTRL_MODE) {
                p_ipu_log_info->ctrl[core].used_size = atoi(cmd+strlen(USED_SIZE));
            } else if (mode == CORECTRL_MODE) {
                p_ipu_log_info->corectrl[core].used_size = atoi(cmd+strlen(USED_SIZE));
            } else {
                printf("invalid argument: %s\n", buf);
                return -1;
            }
        }
    }

    return 0;
}

int save_ipu_log(struct ipu_log_info *p_ipu_log_info, const char *path, const char* strModel)
{
    int i, ret, fd;
    char file[1024];
    void *p_log;

    MI_SYS_Init(0);

    for (i = 0; i < MAX_IPU_CORE_NUM; i++) {
        if (p_ipu_log_info->ctrl[i].addr && p_ipu_log_info->ctrl[i].used_size) {
            snprintf(file, sizeof(file), "%s/%s_log_core%d.bin", path, strModel, i);
            fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
            if (fd < 0) {
                printf("fail to create %s, error=%d\n", file, fd);
                return -1;
            }
            ret = MI_SYS_Mmap(p_ipu_log_info->ctrl[i].addr, p_ipu_log_info->ctrl[i].used_size, &p_log, TRUE);
            if (ret != MI_SUCCESS) {
                printf("fail to map ctrl%d log memory\n", i);
                close(fd);
                unlink(file);
                return -1;
            }

            ret = write(fd, p_log, p_ipu_log_info->ctrl[i].used_size);
            if (ret <= 0) {
                printf("fail to read ctrl%d log data\n", i);
                close(fd);
                unlink(file);
                return -1;
            }
            close(fd);
            MI_SYS_Munmap(p_log, p_ipu_log_info->ctrl[i].used_size);
            printf("save ctrl%d log to %s\n", i, file);
        }

        if (p_ipu_log_info->corectrl[i].addr && p_ipu_log_info->corectrl[i].used_size) {
            snprintf(file, sizeof(file), "%s/%s_log_corectrl%d.bin", path, strModel, i);
            fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
            if (fd < 0) {
                printf("fail to create %s, error=%d\n", file, fd);
                return -1;
            }
            ret = MI_SYS_Mmap(p_ipu_log_info->corectrl[i].addr, p_ipu_log_info->corectrl[i].used_size, &p_log, TRUE);
            if (ret != MI_SUCCESS) {
                printf("fail to map corectrl%d log memory\n", i);
                close(fd);
                unlink(file);
                return -1;
            }

            ret = write(fd, p_log, p_ipu_log_info->corectrl[i].used_size);
            if (ret <= 0) {
                printf("fail to read corectrl%d log data\n", i);
                close(fd);
                unlink(file);
                return -1;
            }
            close(fd);
            MI_SYS_Munmap(p_log, p_ipu_log_info->corectrl[i].used_size);
            printf("save corectrl%d log to %s\n", i, file);
        }
    }

    return 0;
}
