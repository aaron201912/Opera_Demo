#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdint.h>
#include <pthread.h>
#include "sstar_rpmsg.h"
#include "rpmsg_dualos_common.h"
#define RPMSG_COUNT 3

struct rpmsg_info
{
    int eptFd;
    int channel;
    int src;
    int dst;
    pthread_t recv_thread;
    int exit;
};

unsigned long get_current_tick(void)
{
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);

    return spec.tv_sec * 1000 + spec.tv_nsec / 1000 / 1000;
}

void set_gpio_param(int EndPoint, const char *data, int Gpio_Index, int OutLevel)
{
    char message[128] = {0};
    char Description[128] = {0};
    char param_lenth[2] = {0};
    char param[10] = {0};
    int len = 0;
    memset(Description, 0, sizeof(Description));
    strncpy(Description, data + 2, strlen(data) - 2);
    printf("Get GPIO Description:[%s]\n", Description);

    message[0] = '0';
    message[1] = '0';
    len += 2;

    sprintf(param, "%d", Gpio_Index);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", OutLevel);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    message[len] = '\0';
    write(EndPoint, message, len + 1);
    printf("GPIO param set\n");

}

void set_adc_param(int EndPoint, const char *data, int ChanSel, int trimode, int trimethod,int conmode_sel,int dmaenable, int enable)
{
    char message[128] = {0};
    char Description[128] = {0};
    char param_lenth[2] = {0};
    char param[10] = {0};
    int len = 0;
    memset(Description, 0, sizeof(Description));
    strncpy(Description, data + 2, strlen(data) - 2);
    printf("Get GPIO Description:[%s]\n", Description);

    message[0] = '0';
    message[1] = '1';
    len += 2;

    sprintf(param, "%d", ChanSel);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", trimode);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", trimethod);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", conmode_sel);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", dmaenable);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", enable);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    message[len] = '\0';
    write(EndPoint, message, len + 1);
    printf("ADC param set\n");

}

void set_pwm_out_param(int EndPoint, const char *data, int PWMIndex, int Freq, int Shift, int Duty, int NotInvert, int enable)
{
    char message[128] = {0};
    char Description[128] = {0};
    char param_lenth[2] = {0};
    char param[10] = {0};
    int len = 0;
    memset(Description, 0, sizeof(Description));
    strncpy(Description, data + 2, strlen(data) - 2);
    printf("Get GPIO Description:[%s]\n", Description);

    message[0] = '0';
    message[1] = '2';
    len += 2;

    sprintf(param, "%d", PWMIndex);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", Freq);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", Shift);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", Duty);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", NotInvert);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", enable);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    message[len] = '\0';
    write(EndPoint, message, len + 1);
    printf("PWM param set\n");


}

void set_pwm_in_param(int EndPoint, const char *data, int PWMId, int CapEnable, int RstMode, int DetMode, int EdgeSel, int TimerDiv, int PulDiv)
{
    char message[128] = {0};
    char Description[128] = {0};
    char param_lenth[2] = {0};
    char param[10] = {0};
    int len = 0;
    memset(Description, 0, sizeof(Description));
    strncpy(Description, data + 2, strlen(data) - 2);
    printf("Get GPIO Description:[%s]\n", Description);

    message[0] = '0';
    message[1] = '3';
    len += 2;

    sprintf(param, "%d", PWMId);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", CapEnable);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", RstMode);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", DetMode);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", EdgeSel);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", TimerDiv);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    sprintf(param, "%d", PulDiv);
    sprintf(param_lenth, "%d", strlen(param));
    strncpy(message + len, param_lenth, strlen(param_lenth));//set gpio index lenth
    len += strlen(param_lenth);
    strncpy(message + len, param, strlen(param));//set gpio index
    len += strlen(param);

    message[len] = '\0';
    write(EndPoint, message, len + 1);
    printf("PWM param set\n");

}


/****************************************************************************
message:
    [MESSAGE_TYPE][BSP_TYPE][Description/DATA]
    [Description]: [BSP_TYPE] select 0/1/2
    [DATA]: [BSP_TYPE] select 3/4/5
    [MESSAGE_TYPE]
        0: message
        1: command
    [BSP_TYPE]
        0: GPIO
        1: ADC
        2: PWM
        3: GPIO data
        4: ADC data
        5: PWM data
command:
    [MESSAGE_TYPE][BSP_TYPE][BSP_COMMAND]
    [BSP_COMMAND]:
        [PARAM_LENTH1][PARAM1][PARAM_LENTH2][PARAM2]...
    [MESSAGE_TYPE]
        0: message
        1: command
    [BSP_TYPE]
        0: GPIO
        1: ADC
        2: PWM out
        3: PWM in
        4: ADC data
        5: PWM data
****************************************************************************/
void parse_message(int EndPoint, const char *data)
{
    char *type[2] = {"0","1"};//0: cmd;1: msg
    char *Bsp_type[6] ={"0","1","2","3","4","5"}; //0: GPIO;1: ADC;2: PWM out;3: PWM in;4: ADC data;5: PWM data
    char Description[128] = {0};
    //printf("message parse\n");
    if(strncmp(data, type[0], 1) == 0)
    {
        if(strncmp(data + 1, Bsp_type[0], 1) == 0){
            //strncpy(message, data + 1,strlen(data));
            printf("GPIO param parse\n");
        }else if(strncmp(data + 1, Bsp_type[1], 1) == 0){
            //strncpy(message, data + 1,strlen(data));
            printf("ADC param parse\n");
        }else if(strncmp(data + 1, Bsp_type[2], 1) == 0){
            //strncpy(message, data + 1,strlen(data));
            printf("PWM param parse\n");
        }else{
            printf("undefined param!\n");
        }
        //printf("Get MCU cmd %s\n", message);
    }
    else if(strncmp(data, type[1], 1) == 0)
    {
        if(strncmp(data + 1, Bsp_type[0], 1) == 0){

            set_gpio_param(EndPoint, data, 122, 1);

        }
        else if(strncmp(data + 1, Bsp_type[1], 1) == 0){

            set_adc_param(EndPoint, data, 1, 14, 1, 1, 1, 1);

        }
        else if(strncmp(data + 1, Bsp_type[2], 1) == 0){

            set_pwm_out_param(EndPoint, data, 7, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 8, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 9, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 10, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 11, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 12, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 13, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 14, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 15, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 16, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 17, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 18, 10000, 0, 50, 0, 1);
            set_pwm_out_param(EndPoint, data, 19, 10000, 0, 50, 0, 1);
        }
        else if(strncmp(data + 1, Bsp_type[3], 1) == 0)
        {
            set_pwm_in_param(EndPoint, data, 0, 1, 1, 1, 0, 3, 2);
            set_pwm_in_param(EndPoint, data, 1, 1, 1, 1, 0, 3, 2);
            set_pwm_in_param(EndPoint, data, 3, 1, 1, 1, 0, 3, 2);
            set_pwm_in_param(EndPoint, data, 4, 1, 1, 1, 0, 3, 2);
            set_pwm_in_param(EndPoint, data, 5, 1, 1, 1, 0, 3, 2);

        }
        else if(strncmp(data + 1, Bsp_type[4], 1) == 0)
        {
            memset(Description, 0, sizeof(Description));
            strncpy(Description, data + 2, strlen(data) - 2);
            printf("Get ADCdata: value[%s]\n", Description);
        }
        else if(strncmp(data + 1, Bsp_type[5], 1) == 0)
        {
            memset(Description, 0, sizeof(Description));
            strncpy(Description, data + 2, strlen(data) - 2);
            printf("Get PWMdata: [%s]\n", Description);
        }
        else{
            printf("undefined message!\n");
        }
    }
    else if(strncmp(data, "start", 5) == 0)
    {
        printf("MCU reday\n");
    }
    else
    {
        printf("message type err\n");
    }
}

void* rpmsg_recv(void* param)
{
    //select Mcu message
    int s32Ret = 0;
    int ret = 0;
    fd_set read_fds;
    char data[256];
    struct timeval TimeoutVal;
    struct rpmsg_info *info = (struct rpmsg_info *)param;
    TimeoutVal.tv_sec = 2;
    TimeoutVal.tv_usec = 0;
    while(info->exit)
    {      
        FD_ZERO(&read_fds);
        FD_SET(info->eptFd, &read_fds);
        //printf("select rpmsg dev!\n");
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        s32Ret = select(info->eptFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if(s32Ret < 0)
        {
            printf("rpmsg select failed\n");
            usleep(10 * 1000);
            continue;
        }
        else if(s32Ret == 0)
        {
            usleep(10 * 1000);
            continue;
        }
        else
        {
            if(FD_ISSET(info->eptFd, &read_fds)){
                printf("Mcu reday send message\n");
                memset(data, 0, sizeof(data));
                ret = read(info->eptFd, data, sizeof(data));
                if (ret > 0)
                {
                    printf("Linux [ept src:%d;rpmsg channel:%d] read:%s\n", info->src, info->channel, data);
                    if(info->dst == 2)
                        parse_message(info->eptFd, data);
                    else
                    {
                        memset(data, 0, sizeof(data));
                        snprintf(data, sizeof(data), "rpmsg [channel:%d;Linux ept:%d] Linux alive", info->channel, info->src);
                        write(info->eptFd, data, strlen(data) + 1);
                        usleep(10 * 1000);
                    }
                }
            }
        }
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
    printf("rpmsg channel%d recv thread exit!\n", info->channel);
    return (void*)0;
}

struct rpmsg_info recv[RPMSG_COUNT];

int main()
{
        struct ss_rpmsg_endpoint_info info[RPMSG_COUNT];
        char buffer[6] = "start\0";
        char stop_buffer[5] = "stop\0";
        char data[512];
        int ret,lenth;
        char devPath[256];
        int fd;
        unsigned int index = 0x0;
        int Mcu_enable[RPMSG_COUNT] = {0, 0};
        unsigned long last, now;
        unsigned int count;
        int i = 0;
        fd = open("/dev/rpmsg_ctrl0", O_RDWR);
        if (fd < 0)
        {
            perror("open:");
            return 0;
        }
        printf("open rpmsg_ctrl0 success!\n");
        for(i = 0; i < RPMSG_COUNT; i++)
        {
            memset(&info[i], 0, sizeof(struct ss_rpmsg_endpoint_info));
            memset(&recv[i], 0, sizeof(struct rpmsg_info));
            info[i].src = EPT_ADDR_MACRO(EPT_TYPE_CUSTOMER, 1 + 2 * i);
            recv[i].src = 1 + 2 * i;
            info[i].dst = EPT_ADDR_MACRO(EPT_TYPE_CUSTOMER, 2 + 2 * i);
            recv[i].dst = 2 + 2 * i;
            snprintf(info[i].name, sizeof(info[i].name), "demo%d", i);
            info[i].mode = RPMSG_MODE_RISCV;
            info[i].target_id = 0;

            if (ioctl(fd, SS_RPMSG_CREATE_EPT_IOCTL, &info[i]) < 0)
            {
                perror("ioctl:");
                close(fd);
                return 0;
            }

            sleep(2);
            snprintf(devPath, sizeof(devPath), "/dev/rpmsg%d", info[i].id);
            recv[i].eptFd = open(devPath, O_RDWR);
            printf("open /dev/rpmsg%d success!\n", info[i].id);
            if (recv[i].eptFd < 0)
            {
                fprintf(stderr, "Failed to open endpoint!\n");
                return 0;
            }
            recv[i].channel = info[i].id;
        }
        last = now = get_current_tick();
        for(i = 0; i < RPMSG_COUNT; i++)
        {
            if(Mcu_enable[i] == 0)
            {   
                while (1)
                {
                    snprintf(data, sizeof(data), "rpmsg [channel:%d;Linux ept:%d] send: %s", recv[i].channel, recv[i].src, buffer);
                    ret = write(recv[i].eptFd, data, strlen(data) + 1);
                    if(ret > 0)
                    {
                        printf("rpmsg [channel:%d;linux ept:%d] write start message to Mcu success!\n", recv[i].channel, recv[i].src);
                        Mcu_enable[i] = 1;
                        break;
                    }
                }
            }
        }
        for(i = 0; i < RPMSG_COUNT; i++)
        {
            recv[i].exit = 1;                
            pthread_create(&recv[i].recv_thread, NULL, rpmsg_recv, (void*)&recv[i]);
        }
        getchar();
        for(i = 0; i < RPMSG_COUNT; i++)
        {    
            recv[i].exit = 0;
            if(i == (RPMSG_COUNT - 1))
            {
                ret = write(recv[i].eptFd, stop_buffer, strlen(stop_buffer) + 1);
                if(ret > 0)
                {
                    printf("rpmsg [channel:%d;linux ept:%d] write stop message to Mcu success!\n", recv[i].channel, recv[i].src);
                }
            }
        }
        for(i = 0; i < RPMSG_COUNT; i++)
        {
            if(recv[i].recv_thread)
            {
                //printf("rpmsg channel%d recv thread join and exit[%d]!\n", recv[i].channel, recv[i].exit);
                pthread_join(recv[i].recv_thread, NULL);
            }
            close(recv[i].eptFd);
            printf("close /dev/rpmsg%d success!\n", recv[i].channel);
        }
        close(fd);
        printf("close rpmsg_ctrl0 success!\n");      
        return 0;

}
