#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	fd_set readfds;
    struct timeval timeout;
    int fd_log, fd_save;
	int ret;
	ssize_t bytes_read, bytes_written;
	
	char *buffer = (char *)malloc(8 * 1024); //日志节点最多可以缓存8KB数据
	
    fd_log = open("/proc/dualos/log", O_RDONLY);
    if (fd_log == -1) {
        perror("open fd_log error\n");
        return -1;
    }
	
	fd_save = open("/tmp/save_log_file.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd_save == -1) {
        perror("open fd_save error\n");
        return -1;
    }

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(fd_log, &readfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 300 * 1000;

        ret = select(fd_log + 1, &readfds, NULL, NULL, &timeout);
        if (ret == -1) 
		{
            perror("select error\n");
            return -1;
        } 
		else if (ret == 0) 
		{
            //printf("select timeout\n");
        } 
		else 
		{
			lseek(fd_log, 0, SEEK_SET);        
            bytes_read = read(fd_log, buffer, 8 * 1024);
			if(bytes_read == -1)
			{
				perror("read error\n");
				return -1;
			}
			bytes_written = write(fd_save, buffer, bytes_read);
			if(bytes_written == -1) 
			{
				perror("write error");
				return -1;
			}
        }
    }

    // 关闭文件节点
    close(fd_log);
	close(fd_save);
	
    return 0;
}