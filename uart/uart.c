#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <string.h>
#include <unistd.h>     /*Unix标准函数定义*/
#include <sys/types.h>  /**/
#include <sys/stat.h>   /**/
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <sys/time.h>
#include <pthread.h>


#define SPEED_CNT   10
#define BUF_SIZE    100

/*用来接收轨道数据*/
char buffer[BUF_SIZE];

int speed_arr[SPEED_CNT] = {
    B2400,B4800,B9600,B19200,B38400,
    B57600,B115200,B230400,B460800,B921600
};

int name_arr[SPEED_CNT] = {
    2400,4800,9600,19200,38400,
    57600,115200,230400,460800,921600
};

/**
*@brief  设置串口通信速率
*@param  opt     类型  struct termios * 终端参数结构体 *
*@param  speed  类型 int  串口速度
*@return  0 success or -1 err
*/
int set_speed(struct termios *opt, int speed)
{
    int i;
    int status;

    for (i= 0; i<SPEED_CNT; i++)
    {
        if (speed == name_arr[i])
        {
            printf("set speed %d  %d\n",name_arr[i], speed_arr[i]);
            /*  设置串口的波特率 */
            cfsetispeed(opt, speed_arr[i]);
            cfsetospeed(opt, speed_arr[i]);

            return 0;
        }
   }

    printf("Cannot find suitable speed\n");
    return -1;
}


/*
*@brief   设置停止位
*@param  opt     类型  struct termios * 终端参数结构体 *
*@param  stopbit 类型const char * stop bit位 1 或者1.5或者2

*/
static void set_stopbit(struct termios *opt, const char *stopbit)
{
    if (0 == strcmp (stopbit, "1"))
    {
        opt->c_cflag &= ~CSTOPB; /* 1 stop bit */
        printf("set 1 stop bit\n");
    }
    else if (0 == strcmp (stopbit, "1.5"))
    {
        opt->c_cflag &= ~CSTOPB; /* 1.5 stop bit */
        printf("set 1.5 stop bit\n");
    }
    else if (0 == strcmp (stopbit, "2"))
    {
        opt->c_cflag |= CSTOPB;  /* 2 stop bits */
        printf("set 2 stop bit\n");
    }
    else
    {
        opt->c_cflag &= ~CSTOPB; /* 1 stop bit */
        printf("default set 2 stop bit\n");
    }

}

/*
*@brief   设置奇偶校验位
*@param  opt     类型  struct termios * 终端参数结构体 *
*@param  parity  类型  char  效验类型 取值为N,E,O,

*/
static void set_parity(struct termios *opt, char parity)
{
    switch (parity)
       {
           case 'e':
           case 'E':                  /* even 偶校验 */
               opt->c_cflag |= PARENB;
               opt->c_cflag &= ~PARODD;
               opt->c_iflag |= (INPCK | ISTRIP);
               printf("even parity check\n");
               break;
           case 'o':
           case 'O':                  /* odd  奇校验 */
               opt->c_cflag |= PARENB;
               opt->c_cflag |= ~PARODD;
               opt->c_iflag |= (INPCK | ISTRIP);
               printf("odd parity check\n");
               break;
           case 'n':
           case 'N':                  /* no parity check */
           default:                   /* no parity check */
               opt->c_cflag &= ~PARENB;
               opt->c_iflag &= ~(INPCK | ISTRIP);
               printf("no parity check\n");
               break;
       }


}
/*
*@brief   设置硬件流控
*@param  opt     类型  struct termios * 终端参数结构体 *
*@param  crtscts  类型  unsigned int  流控类型 取值为0或1

*/

static void set_flowcontrol(struct termios *opt, unsigned int crtscts)
{
    if(crtscts == 0)
    {
        opt->c_cflag &= ~CRTSCTS;
        printf("close hw flowctrl\n");
    }
    else if(crtscts == 1)
    {
        opt->c_cflag |= CRTSCTS;
        printf("open hw flowctrl\n");
    }
}

/*
*@brief   设置数据位
*@param  opt     类型  struct termios * 终端参数结构体 *
*@param  databit  类型  unsigned int  数据位 取值为5,6,7,8
*/
static void set_data_bit(struct termios *opt, unsigned int databit)
{
    opt->c_cflag &= ~CSIZE;
    switch (databit)
    {
        case 5:
            opt->c_cflag |= CS5;
            printf("set data bit 5 bit\n");
            break;
        case 6:
            opt->c_cflag |= CS6;
            printf("set data bit 6 bit\n");
            break;
        case 7:
            opt->c_cflag |= CS7;
            printf("set data bit 7 bit\n");
            break;
        case 8:
        default:
            opt->c_cflag |= CS8;
            printf("set data bit 8 bit\n");
            break;
    }
}


/*
*@brief   设置字符转换
*@param  opt     类型  struct termios * 终端参数结构体 *
*@param  change  类型  unsigned int  是否抓换 取值为0和1
*/

static void set_charchange(struct termios *opt, unsigned int change)
{
    if(change == 0)
    {
        opt->c_iflag &= ~ICRNL;
        opt->c_oflag &= ~OPOST; /*原始输出，不对输出处理*/
    }
    else if(change == 1)
    {
        opt->c_iflag |= ICRNL; /*将输入的CR转化为NL*/
        opt->c_oflag |= OPOST;
    }
}
/*
*@brief   设置串口模式
*@param  opt     类型  struct termios * 终端参数结构体 *
*@param  tyep    类型  unsigned int 串口模式 取值为0和1
*/

static void set_localcontrol(struct termios *opt, unsigned int type)
{
    switch(type)
    {
        case 1:
            /*规范模式, 用户终端时使用*/
            opt->c_lflag |= (ICANON | ECHO | ECHOE);
            break;
        case 0:
        default:
            /*原始模式,串口输入数据三不经过处理的，在串口接收的数据被完整保留*/
            opt->c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
            break;
    }
}


/**
*@brief   设置串口数据位，停止位和效验位
*@param  fd     类型  int  打开的串口文件句柄*
*@param  baudrate     类型  int  波特率*
*@param  databit 类型  int 数据位   取值 为5,6， 7 或者8*
*@param  stopbit 类型  const char *  停止位   取值为 1，1.5或者2*
*@param  parity  类型  char  效验类型 取值为N,E,O
*@param  ctsrts  类型int     是否开启流控 取值为1,0

*@param  vtime vmin  类型  int      若以O_NONBLOCK 方式open，这两个设置没有作用，等同于都为0
                                  若非O_NONBLOCK 方式open，vtime定义要求等待的零到几百毫秒的时间
                                                           vmin 定义要求等待的最小字节数，这个字节可能是0

*@return  0 success or -1 err
*/
//set_port_attr (fd, B115200, 8, "1", 'N', 150, 255);
int set_port_attr (int fd,int baudrate, int databit, const char *stopbit, char parity, int ctsrts,int vtime,int vmin )
{
    struct termios options;
    if (tcgetattr(fd, &options) != 0)
    {
        perror("tcgetattr");
        return -1;
    }
#if 1
    set_speed(&options, baudrate);
    set_data_bit(&options, databit);

    set_stopbit(&options, stopbit);

    set_parity(&options, parity);
    set_flowcontrol(&options, ctsrts);
    set_charchange(&options, 0);
    set_localcontrol(&options, 0);

    options.c_cflag |= CLOCAL | CREAD;
    options.c_cc[VTIME] = vtime; //150
    options.c_cc[VMIN] = vmin; //255
#else
    set_baudrate(&opt, baudrate);
    set_data_bit(&opt, databit);
    //set_flowcontrol(&opt, 0);
    //set_stopbit(&opt, stopbit);

    /* 忽略调制解调器线路状??使用接收??*/
    opt.c_cflag |= CLOCAL | CREAD;

    /* 不使用流??*/
    opt.c_cflag &= ~CRTSCTS;
    /* 8个数据位 */
    opt.c_cflag &= ~CSIZE;
    opt.c_cflag |= CS8;
    /* 无奇偶校验位 */
    opt.c_cflag &= ~PARENB;
    opt.c_iflag &= ~INPCK;//ISTRIP

    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* 1个停止位 */
    opt.c_cflag &= ~CSTOPB;

    opt.c_oflag &= ~OPOST;

    opt.c_iflag &= ~(INLCR | ICRNL | IGNCR | IXON);//IXOFF IXANY
    opt.c_oflag &= ~(ONLCR | OCRNL);

    opt.c_cc[VTIME] = 2;//2

    opt.c_cc[VMIN] = 80;

#endif

    /* Update the options and do it NOW */
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("tcsetattr error");
        return -1;
    }

    /* 清空正读的数据，且不会读出 */
    tcflush(fd,TCIFLUSH);
    tcflush(fd,TCOFLUSH);

    return 0;
}

#define BUF_LEN         (32)
int Send_Len= 16,SendCount = 0;
int RecvCount = 0;
volatile unsigned char g_send_buf[BUF_LEN] = {"abcdefghi12345678"};
volatile unsigned char g_recv_buf[BUF_LEN] = {0};

int uart_send_stop = 0;


void *uart_recv(void *arg)
{
    int i = 0, j = 0;
    int loop_error = 0;
    int recv_count =0;
    int ret = 0;
    int len,fd = -1;
    volatile unsigned char *pbuf = g_recv_buf;
    fd = *((int *)arg);

    while(uart_send_stop == 0 || RecvCount < SendCount )
    {
        ret = read(fd,(void *)pbuf,Send_Len);
        if(ret < 0)
        {
            printf("uart recv len: %d\n", ret);
        }
        if(RecvCount%32 == 0)
        printf("uart recv len: %d RecvCount %d\n", ret,RecvCount);

        memset((void *)pbuf, 0, Send_Len);
        RecvCount += ret;

   }
    printf("uart recv len: %d \n", RecvCount);
recv_stop:
    pthread_exit("uart_recv exit\n");
}

void *uart_send(void *arg)
{
    int fd = -1;
    int fd_random = -1;
    int loop = 10;
    int send_len = 0,send_count=0;
    int ret = -1;

    fd = *((int *)arg);

    fd_random = open("/dev/urandom", O_RDWR|O_NOCTTY);
    if(fd_random < 0)
    {
        printf("open /dev/urandom failed!\n");
        goto send_stop;
    }

    while(send_count < SendCount)
    {
    /*  while(send_len < Send_Len)
        {
            send_len = read(fd_random, (void *)g_send_buf, Send_Len);
        }*/
        system("sync");
        ret = write(fd, (void *)g_send_buf, Send_Len);
        system("sync");
        send_len = 0;
        if(send_count%32 == 0)
            printf("uart send count len:%d current sedlen:%d \n",send_count,ret);
        send_count += ret;
        usleep(1*1000);
    }

    usleep(100*1000);
send_stop:
    uart_send_stop = 1;

    pthread_exit("uart_recv exsit\n");
}


/**

*@breif     main()

*/
int main(int argc, char **argv)
{
    int fd;
    int ret = -1;
    const char *dev = NULL;
    int baudrate;
    int databit;
    char *stopbit=NULL;
    char parity;
    int ctsrts;
    int test_type = 0;


    const char test_string[9] = {"abcdefgh\0"};
    char *p_buffer = NULL;
    int nread = 0;
    int nwrite = 0;
    int  i=0;

    pthread_t th_send, th_recv;



    /* 1、检测传参 */
    if (argc != 8 ) {
        printf("Usage:%s dev_name baudrate databit stopbit parity ctsrts [test type]\n", argv[0]);
        exit(1);
    }
    dev = argv[1];
    baudrate = atoi(argv[2]);
    databit = atoi(argv[3]);
    stopbit = argv[4];
    parity = *argv[5];
    ctsrts = atoi(argv[6]);
    test_type = atoi(argv[7]);


    /* 2、打开串口 */
    fd = open(dev, O_RDWR | O_NOCTTY ); //O_NONBLOCK
    if (-1 == fd) {
        printf("Cannot open %s:%s\n", dev, strerror(errno));
        exit(1);
    }
    printf("==== open %s success ====\n", dev);

#if 1
    /* 3、初始化设备 */

    if (-1 == set_port_attr(fd, baudrate, databit, stopbit,parity,ctsrts,10,0)) {
        printf("Set port_attr Error\n");
        close(fd);
        exit(1);
    }
#endif
    /* 4、开始收发测试 */
   if(test_type == 0 || test_type ==1)
   {
        int count = 0;
        if(test_type == 0)
            count = 128;
        else
            count = 128*1024*50;

        for(i =0 ;i< count;i++)
        {
            if (NULL != test_string) {
                /*开始自发自收测试*/
                /* 写串口 */
                do {
                    nwrite = write(fd, test_string, strlen(test_string));
                } while(nwrite <= 0);
                printf("send %d bytes data.\n", nwrite);

                /* 清空buffer */
                memset(buffer, 0, BUF_SIZE);
                p_buffer = buffer;
                nread = 0;

                /* 读串口 */
                do {
                    ret = read(fd, p_buffer, 64);
                    if (ret > 0) {
                     //   printf("read %d bytes data:%s\n", ret, p_buffer);
                        p_buffer += ret;
                        nread += ret;
                    }
                } while(nread < nwrite);
                printf("all read %d bytes data:%s\n", nread, buffer);
                if( (0 != strcmp(buffer, test_string)))
                {
                      printf( "test type  error!!! send:%s != read:%s",test_string,buffer);
                     close(fd);
                     return -1;
                }

             }
        }
        printf( "test send and recv count successs!!!\n",count);
        close(fd);
        return 0;
    }



    if(test_type == 2 || test_type == 3)
    {
        if(test_type == 2)
            SendCount = 1024;
        else
            SendCount = 1024*50;
        uart_send_stop = 0;
        pthread_create(&th_recv, NULL, uart_recv, (void *)(&fd));
        sleep(1);
        pthread_create(&th_send, NULL, uart_send, (void *)(&fd));



            /* 等待两个线程结束*/
        pthread_join(th_send, NULL);
        pthread_join(th_recv, NULL);
        if(SendCount == RecvCount)
        {
             printf( "test send %d and rec %d  successs!!!\n",SendCount,RecvCount);
             RecvCount=0;
             close(fd);
             return 0;
        }else
        {
             printf( "test send %d and rec %d  not successs!!!\n",SendCount,RecvCount);
             RecvCount=0;
             close(fd);
             return -1;
        }

    }



        /*开始测试*/
        /*循环读取并回写串口内容*/
    if(test_type == 4)
     {
        printf("start read and pass back\n");
        while(1) {
            /* 清空buffer */
            memset(buffer, 0, BUF_SIZE);
            /* 读串口 */
            nread = read(fd, buffer, 64);
            if(nread > 0) {
                printf("read %d bytes.data: %s\n", nread,buffer);
                do {
                    ret = write(fd, buffer, nread);
                } while(ret <= 0);

                printf("write %d bytes.\n", ret);
            }

            //printf("block test.\n");
        }

        close(fd);

        return 0;
     }
}

