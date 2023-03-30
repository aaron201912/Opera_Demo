#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
//#define DEV_NAME    "/dev/ttyS1"    ///< 串口设备
//static const char *device = "/dev/ttyS1"; 
//static uint32_t baudrate = 115200;
static bool Test_Status = false;
static int uartfd = 0;
static int _g_success_num;
static int _g_error_num;
pthread_t UARTThreadID;

#define DATA_SIZE (1024 * 1)
#define CRC_DATA_BYTE 2   //校验码长度为2字节
#define UART_DEBUG

int setOpt_cus(int uartfd, int nSpeed, int nBits, int nParity, int nStop)
{
    struct termios tio;

    // 保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息
    if (tcgetattr(uartfd, &tio) != 0)
    {
        perror("SetupSerial 1");
        return -1;
    }

	/* Turn off echo, canonical mode, extended processing, signals */
	tio.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	/* Turn off break sig, cr->nl, parity off, 8 bit strip, flow control */
	tio.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	/* Clear size, turn off parity bit */
	tio.c_cflag &= ~(CSIZE | PARENB);

	/* Set size to 8 bits */
	tio.c_cflag |= CS8;

	/* Turn output processing off */
	tio.c_oflag &= ~(OPOST);

	/* Set time and bytes to read at once */
	tio.c_cc[VTIME] = 0;
	tio.c_cc[VMIN] = 0;

    // 设置校验位
    switch (nParity)
    {
        case 'o':
        case 'O':                     //奇校验
            tio.c_cflag |= PARENB;
            tio.c_cflag |= PARODD;
            tio.c_iflag |= (INPCK | ISTRIP);
            break;
        case 'e':
        case 'E':                     //偶校验
            tio.c_iflag |= (INPCK | ISTRIP);
            tio.c_cflag |= PARENB;
            tio.c_cflag &= ~PARODD;
            break;
        case 'n':
        case 'N':                    //无校验
            tio.c_cflag &= ~PARENB;
            break;
        default:
            fprintf(stderr, "Unsupported parity\n");
            return -1;
    }
    #if 1
    // 设置波特率 2400/4800/9600/19200/38400/57600/115200/230400
    switch (nSpeed)
    {
        case 2400:
            cfsetispeed(&tio, B2400);
            cfsetospeed(&tio, B2400);
            break;
        case 4800:
            cfsetispeed(&tio, B4800);
            cfsetospeed(&tio, B4800);
            break;
        case 9600:
            cfsetispeed(&tio, B9600);
            cfsetospeed(&tio, B9600);
            break;
        case 19200:
            cfsetispeed(&tio, B19200);
            cfsetospeed(&tio, B19200);
            break;
        case 38400:
            cfsetispeed(&tio, B38400);
            cfsetospeed(&tio, B38400);
            break;
        case 57600:
            cfsetispeed(&tio, B57600);
            cfsetospeed(&tio, B57600);
            break;
        case 115200:
            cfsetispeed(&tio, B115200);
            cfsetospeed(&tio, B115200);
            break;
        case 230400:
            cfsetispeed(&tio, B230400);
            cfsetospeed(&tio, B230400);
            break;
        case 921600:
            cfsetispeed(&tio, B921600);
            cfsetospeed(&tio, B921600);
            break;
        default:
            printf("\tSorry, Unsupported baud rate, set default 9600!\n\n");
            cfsetispeed(&tio, B9600);
            cfsetospeed(&tio, B9600);
            break;
    }
    #endif
    // 设置read读取最小字节数和超时时间


	if(tcsetattr(uartfd,TCSAFLUSH,&tio) < 0)
	{
        perror("SetupSerial 3");
        return -1;
	}


}

int setOpt(int uartfd, int nSpeed, int nBits, int nParity, int nStop)
{
    struct termios newtio, oldtio;

    // 保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息
    if (tcgetattr(uartfd, &oldtio) != 0)
    {
        perror("SetupSerial 1");
        return -1;
    }

    bzero(&newtio, sizeof(newtio));   //新termios参数清零
    newtio.c_cflag |= CLOCAL | CREAD; //CLOCAL--忽略 modem 控制线,本地连线, 不具数据机控制功能, CREAD--使能接收标志
    // 设置数据位数
    newtio.c_cflag &= ~CSIZE; //清数据位标志
    switch (nBits)
    {
    case 7:
        newtio.c_cflag |= CS7;
        break;
    case 8:
        newtio.c_cflag |= CS8;
        break;
    default:
        fprintf(stderr, "Unsupported data size\n");
        return -1;
    }
    // 设置校验位
    switch (nParity)
    {
    case 'o':
    case 'O': //奇校验
        newtio.c_cflag |= PARENB;
        newtio.c_cflag |= PARODD;
        newtio.c_iflag |= (INPCK | ISTRIP);
        break;
    case 'e':
    case 'E': //偶校验
        newtio.c_iflag |= (INPCK | ISTRIP);
        newtio.c_cflag |= PARENB;
        newtio.c_cflag &= ~PARODD;
        break;
    case 'n':
    case 'N': //无校验
        newtio.c_cflag &= ~PARENB;
        break;
    default:
        fprintf(stderr, "Unsupported parity\n");
        return -1;
    }
    // 设置停止位
    switch (nStop)
    {
    case 1:
        newtio.c_cflag &= ~CSTOPB;
        break;
    case 2:
        newtio.c_cflag |= CSTOPB;
        break;
    default:
        fprintf(stderr, "Unsupported stop bits\n");
        return -1;
    }
    // 设置波特率 2400/4800/9600/19200/38400/57600/115200/230400
    switch (nSpeed)
    {
    case 2400:
        cfsetispeed(&newtio, B2400);
        cfsetospeed(&newtio, B2400);
        break;
    case 4800:
        cfsetispeed(&newtio, B4800);
        cfsetospeed(&newtio, B4800);
        break;
    case 9600:
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    case 19200:
        cfsetispeed(&newtio, B19200);
        cfsetospeed(&newtio, B19200);
        break;
    case 38400:
        cfsetispeed(&newtio, B38400);
        cfsetospeed(&newtio, B38400);
        break;
    case 57600:
        cfsetispeed(&newtio, B57600);
        cfsetospeed(&newtio, B57600);
        break;
    case 115200:
        cfsetispeed(&newtio, B115200);
        cfsetospeed(&newtio, B115200);
        break;
    case 230400:
        cfsetispeed(&newtio, B230400);
        cfsetospeed(&newtio, B230400);
        break;
    default:
        printf("\tSorry, Unsupported baud rate, set default 9600!\n\n");
        cfsetispeed(&newtio, B9600);
        cfsetospeed(&newtio, B9600);
        break;
    }
    // 设置read读取最小字节数和超时时间
    newtio.c_cc[VTIME] = 1; // 读取一个字符等待1*(1/10)s
    newtio.c_cc[VMIN] = 1;  // 读取字符的最少个数为1

    newtio.c_cflag &= ~(ICRNL | IXON);
    tcflush(uartfd, TCIFLUSH);                    //清空缓冲区
    if (tcsetattr(uartfd, TCSANOW, &newtio) != 0) //激活新设置
    {
        perror("SetupSerial 3");
        return -1;
    }
    printf("Serial set done!\n");
    return 0;
}


#if 1
int cnt_num = 0;

int uart_read(int uartfd, char *rcv_buf, int read_size, int timeout)
{
    int len, fs_sel;
    fd_set fs_read;
    float time_use=0;
    int msWait=10000;//5ms
    struct timeval start;
    struct timeval end;
    struct timeval time;

    gettimeofday(&start,NULL); //gettimeofday(&start,&tz);结果一样
    //printf("start.tv_sec:%d\n",start.tv_sec);
    //printf("start.tv_usec:%d\n",start.tv_usec);

    cnt_num = 0;

    while(1)
    {
        gettimeofday(&end,NULL);
        //printf("start.tv_sec:%d\n",end.tv_sec);
        //printf("start.tv_usec:%d\n",end.tv_usec);
        time_use=(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);//微秒
        if(time_use > msWait*1000)
        {
            return cnt_num;
        }

        time.tv_sec = timeout / 1000;              //set the rcv wait time
        time.tv_usec = timeout % 1000 * 1000;    //100000us = 0.1s

        FD_ZERO(&fs_read);        //每次循环都要清空集合，否则不能检测描述符变化
        FD_SET(uartfd, &fs_read);    //添加描述符

        // 超时等待读变化，>0：就绪描述字的正数目， -1：出错， 0 ：超时
        fs_sel = select(uartfd + 1, &fs_read, NULL, NULL, &time);
        //printf("fs_sel = %d\n", fs_sel);
        if(fs_sel)
        {
            len = read(uartfd, rcv_buf+cnt_num, read_size - cnt_num);
            if(len == -1)
            {
                return -1;
            }
            else
            {
                //printf("read len=%d %02X %02X %02X  \n",len,rcv_buf[cnt_num],rcv_buf[cnt_num - 1],rcv_buf[cnt_num - 2]);
                cnt_num += len;
            }
            if(cnt_num >= read_size)
            {
                //printf("cnt_num=%d read_size=%d\n",cnt_num, read_size);
                return cnt_num;
            }
        }
        else
        {
            //printf("read timeout 10s\n");
            return cnt_num;
        }
    }
}

#else
int uart_read(int uartfd, char *rcv_buf, int lenth, int timeout)
{
    int len, fs_sel;
    fd_set fs_read;
    struct timeval time;

    time.tv_sec = timeout / 1000;         //set the rcv wait time
    time.tv_usec = timeout % 1000 * 1000; //100000us = 0.1s

    FD_ZERO(&fs_read);    //每次循环都要清空集合，否则不能检测描述符变化
    FD_SET(uartfd, &fs_read); //添加描述符

    // 超时等待读变化，>0：就绪描述字的正数目， -1：出错， 0 ：超时
    fs_sel = select(uartfd + 1, &fs_read, NULL, NULL, &time);
    //    printf("fs_sel = %d\n", fs_sel);
    if (fs_sel)
    {
        len = read(uartfd, rcv_buf, lenth);
        return len;
    }
    else
    {
        printf("read timeout 10s\n");
        return -1;
    }
}
#endif

int check_sum(char *Start_Byte,int Num_Bytes)
{
	int Checksum = 0;
	while (Num_Bytes--)
	{	// 计算CRC
		Checksum += *Start_Byte++;
	}
	return Checksum;
}




static void print_usage(const char *prog)
{
	printf("Usage: %s [-D -S -H]\n", prog);
	puts("  -D --device   device to use (default /dev/ttyS1)\n"
	     "  -S --speed    baudrate: 2400 4800 9600 19200 115200 576000 500000 460800 230400\n"
	     "  -h --help     eg: ./uart_demo -D /dev/ttyS1 -B 38400 \n"
		 "q/Q  quit");
	exit(1);
}

static void parse_opts(int argc, char *argv[]) 
{ 
	int option_index=0;
	while (1) { 
			static const struct option lopts[] = { 
					{ "device",  required_argument, NULL, 'D' }, 
					{ "baudrate",   required_argument, NULL, 'B' }, 
					{ "help",   no_argument, NULL, 'H' },
					{ 0, 0, 0, 0 }, 
			}; 
			int c; 

			c = getopt_long(argc, argv, "D:B:H", lopts, &option_index); 

			if (c == -1) 
					break; 

			switch (c) { 
			case 'D': 
					//device = optarg; 
					break; 
			case 'B': 
					//baudrate = atoi(optarg); 
					break; 
			case 'H': 
					print_usage(argv[0]);  
					break; 
			default: 
					print_usage(argv[0]); 
					break; 
			} 
	} 
} 
int serial_send(int fd,char *str,unsigned int len)
{
    int ret;
#if 0
    if(len > strlen(str))
    len = strlen(str);
#endif

    //printf("str: %s \n", str);
//    printf("len: %d \n", len);

    ret = write(fd,str,len);

//    printf("ret: %d \n", ret);

    if(ret < 0)
    {
        perror("serial send err:");
        return -1;
    }

    return ret;

}

static void *RTX_Stream(void *args)
{
    char rcv_buf[DATA_SIZE];
    int len;
    int remain_len;
    int refix_remain_len;
    int data_size = 0;
    int start_byte = 0;
    int check_sum_value = 0;
    int need_read_size = 0;
    char *tmp_p = NULL;
    int i = 0;
    int j = 0;
	Test_Status = true;
    while(Test_Status)    //循环读取数据
    {
        memset(rcv_buf,0x0,sizeof(rcv_buf));
        tmp_p = rcv_buf;
        need_read_size = DATA_SIZE - 1;
        refix_remain_len = 0;
        len = uart_read(uartfd, rcv_buf, need_read_size, 1000);
		int ret = serial_send(uartfd,rcv_buf,sizeof(rcv_buf));
		printf("send %d bytes!\n",ret);
        remain_len = len;
		
        if(len > 0)
        {
            printf("----------------------------------------------------- read len =%d -----------------------------------------------------\n",len);
            for(i= 0;i < remain_len; )
            {
                if(tmp_p[i] == 0xaa && tmp_p[i+1] == 00)//实际认为头字节为0xAA 0x00
                {
                    start_byte = i;
                    data_size = tmp_p[i+1] << 8 | tmp_p[i+2];//frame len
                    if(data_size > 255 || data_size <= 3)
                    {
                        printf("data_size error, size=%d header:%02X len:%02X %02X\n",data_size,tmp_p[i],tmp_p[i+1],tmp_p[i+2]);
                        i++;
                        _g_error_num ++;
                        continue;
                    }
                    if(remain_len < data_size + CRC_DATA_BYTE)//最后一帧数据不足
                    {
                        need_read_size = data_size + CRC_DATA_BYTE - remain_len + refix_remain_len;
                        if(need_read_size > 0 && (remain_len - refix_remain_len > 0))
                        {

                            if(tmp_p + start_byte - rcv_buf + remain_len >= sizeof(rcv_buf) - 1 )
                            {
                                start_byte -= refix_remain_len;
                                remain_len = sizeof(rcv_buf) - 1 - ((tmp_p + start_byte) - rcv_buf);
                            }
                            memcpy(rcv_buf, &tmp_p[start_byte], remain_len);
                            tmp_p = rcv_buf;
                            printf("\n");
                            printf("Last frame is exceed,need reset buf now,still_need_size=%d  data_size=%d remain_len=%d refix_remain_len=%d start_byte=%d\n",need_read_size,data_size,remain_len,refix_remain_len,start_byte);
                        }
                        len = uart_read(uartfd, &rcv_buf[remain_len], DATA_SIZE - remain_len - 1, 200);//2 byte crc value is not include in len
						int ret = serial_send(uartfd,&rcv_buf[remain_len],len);
            			printf("499 send %d bytes!\n\n \n",ret);
						remain_len += len;
                        printf("----------------------------------------------------- read2 len=%d -----------------------------------------------------\n",len);
                        if(remain_len >= data_size + CRC_DATA_BYTE )
                        {

                            check_sum_value = tmp_p[data_size]<<8 | tmp_p[data_size+1];
                            if(check_sum_value == check_sum(tmp_p, data_size))
                            {
                                _g_success_num ++;
                                remain_len -= (data_size + CRC_DATA_BYTE);
                                tmp_p += (data_size + CRC_DATA_BYTE);
                                printf("*********************** DATA SUCCESS2:check_sum=%02X error_num=%d success_num=%d ***********************\n",check_sum_value,_g_error_num,_g_success_num);
                            }
                            else
                            {
                            #ifdef UART_DEBUG
                                for(j= 0;j < data_size + CRC_DATA_BYTE; j++)
                                {
                                    printf("%02X ",tmp_p[j+refix_remain_len]);
                                    if((j+1)%30 == 0)
                                    {
                                        printf("\n");
                                    }
                                }
                                printf("\n");
                            #endif
                                _g_error_num ++;
                                remain_len -= data_size;
                                tmp_p += data_size;
                                //printf("check_sum error1 check_sum_value==%02X ori_check_sum_value=%02X remain_len=%d _g_error_num=%d\n",check_sum(tmp_p, data_size), check_sum_value,remain_len,_g_error_num);
                                printf(">>>>>>>>>>>>>>>>>>>>>>> DATA  ERROR1  check_sum=%02X error_num=%d success_num=%d <<<<<<<<<<<<<<<<<<<<<<<\n\n",check_sum(tmp_p, data_size),_g_error_num,_g_success_num);
                            }
                        }
                        else if(remain_len > 0)
                        {
                            printf("**************************Missing DATA*************************\n");
#ifdef UART_DEBUG
                            for(j= 0;j < data_size + CRC_DATA_BYTE; j++)
                            {
                                printf("%02X ",tmp_p[j]);
                                if((j+1)%30 == 0)
                                {
                                    printf("\n");
                                }
                            }
                            printf("\n");
#endif
                            //printf("Error,maybe missing data, frame_size=%02X need_read_size=%02X actual_read=%02X \n",data_size + CRC_DATA_BYTE, need_read_size, (start_byte + len));
                            printf(">>>>>>>>>>>>>>>>>>>>>>> DATA  Missing check_sum=%02X error_num=%d success_num=%d <<<<<<<<<<<<<<<<<<<<<<<\n\n",check_sum(tmp_p, data_size),_g_error_num,_g_success_num);

                            _g_error_num ++;
                            remain_len -= data_size;
                            tmp_p += data_size;
                        }
                        else //len < 0
                        {
                            printf("second read len 0\n");
                            break;
                        }
                    }
                    else
                    {
                        check_sum_value = tmp_p[start_byte+data_size]<<8 | tmp_p[start_byte+data_size+1];
                        if(check_sum_value == check_sum(tmp_p + start_byte, data_size))
                        {
                            _g_success_num ++;
                            remain_len -= (data_size + CRC_DATA_BYTE);
                            printf("*********************** DATA SUCCESS1:check_sum=%02X error_num=%d success_num=%d ***********************\n",check_sum_value,_g_error_num,_g_success_num);
                            tmp_p += (data_size + CRC_DATA_BYTE);

                        }
                        else
                        {
#ifdef UART_DEBUG
                            printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ERROR DATA <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
                            for(j= 0;j < data_size + CRC_DATA_BYTE; j++)
                            {
                                printf("%02X ",tmp_p[start_byte+j]);
                                if((j+1)%30 == 0)
                                {
                                    printf("\n");
                                }
                            }
                            printf("\n");
#endif
                            _g_error_num ++;
                            remain_len -= data_size;
                            refix_remain_len += 2;
                            //printf("check_sum2  error  check_sum_value==%02X ori_check_sum_value=%02X __g_error_num=%d_g_success_num=%d\n",check_sum(tmp_p, data_size),check_sum_value,_g_error_num,_g_success_num);
                            printf(">>>>>>>>>>>>>>>>>>>>>>> DATA  ERROR2  check_sum=%02X error_num=%d success_num=%d <<<<<<<<<<<<<<<<<<<<<<<\n\n",check_sum(tmp_p, data_size),_g_error_num,_g_success_num);
                            tmp_p += data_size;
                        }
                    }
                }
                else
                {
                    i++;
                }

            }
            //uart_write(uartfd, rcv_buf, len);
        }
	}
}

int uart_init(char *device, int baudrate)
{
    // 打开串口设备
	//O_RDWR ： 可读可写
	//O_NOCTTY ：该参数不会使打开的文件成为该进程的控制终端。如果没有指定这个标志，那么任何一个 输入都将会影响用户的进程。
	//O_NDELAY ：这个程序不关心DCD信号线所处的状态,端口的另一端是否激活或者停止。如果用户不指定了这个标志，则进程将会一直处在睡眠状态，直到DCD信号线被激活。

    //uartfd = open(DEV_NAME, O_RDWR | O_NOCTTY | O_NDELAY);
	
	#ifdef CHIP_IS_SSD2386
	system("/customer/riu_w 0x103c 0x6d 0x1110");
	#endif
	
	uartfd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if(uartfd < 0)
    {
        perror(device);
        return -1;
    }
    // 设置串口阻塞， 0：阻塞， FNDELAY：非阻塞
    if (fcntl(uartfd, F_SETFL, 0) < 0) //阻塞
        printf("fcntl failed!\n");


    if (isatty(uartfd) == 0)
    {
        printf("standard input is not a terminal device\n");
        close(uartfd);
        return -1;
    }
    else
    {
        printf("is a tty success!\n");
    }
    printf("uartfd-open=%d\n", uartfd);

    if (setOpt_cus(uartfd, baudrate, 8, 'N', 1)== -1)    //设置8位数据位、1位停止位、无校验
    {
        fprintf(stderr, "Set opt Error\n");
        close(uartfd);
        exit(1);
    }
	printf("device = %s, baudrate =%d\n",device, baudrate);
    tcflush(uartfd, TCIOFLUSH);    //清掉串口缓存

    Test_Status = true;
    pthread_create(&UARTThreadID, NULL, RTX_Stream, NULL);

}
int uart_Get_Errnum()
{
	return _g_error_num;
}

int uart_Get_Successnum()
{
	return _g_success_num;
}

int Uart_Test_Status()
{
	printf("\n\n\n\n\n>>>>>>>>>>>>>>>>>>>>>>>error_num=%d success_num=%d <<<<<<<<<<<<<<<<<<<<<<<\n\n",_g_error_num,_g_success_num);
	return 0;
}
int uart_deinit(void)
{
	#ifdef CHIP_IS_SSD2386
	system("/customer/riu_w 0x103c 0x6d 0x1010");
	#endif
	
	Test_Status = false;
	pthread_join(UARTThreadID, NULL);
	close(uartfd);
	return 0;
}
#if 0
int main (int argc, char *argv[])
{

	parse_opts(argc, argv);
	uart_init();
	char ch;
	int is_enter = 0;
	while((ch=getchar())!='q')
	{
		sleep(1);
	}
	uart_deinit();
	Uart_Test_Status();
}
#endif
