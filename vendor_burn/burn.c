#include <sys/ioctl.h>
#include <stdio.h>
#include <stdbool.h>
#include <mtd/mtd-user.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/types.h>
#include <errno.h>
#include "burn.h"


static char g_vendor_mtdname[24] = {0};
static int g_vendor_blockid = 0;
static struct mtd_info_user g_mtd;

/*
 * @brief get the name of vendor partition
 * @param none
 * @return 0: success
 *        -1: fail
 */
int get_vendor_partition_name()
{
    FILE *fp = NULL;
    char line[128];
    char tmp_mtdsize[24];
    char tmp_mtdname[24];
    char tmp_erasesize[24];
    char tmp_name[24];
    int end = 0;

    if (NULL == (fp = fopen("/proc/mtd", "r")))
    {
        printf("Read /proc/mtd failed\n");
        return -1;
    }

    // get vendor_storage mtdname
    while(fgets(line, sizeof(line), fp))
    {
        if (strstr(line, "mtd") || strstr(line, "mtdblock"))
        {
            sscanf(line, "%s %s %s %s", tmp_mtdname, tmp_mtdsize, tmp_erasesize, tmp_name);
            //printf("[3irobot] %s %s %s %s\n", tmp_mtdname, tmp_mtdsize, tmp_erasesize, tmp_name);

            //find vendor_storage partition
            if(!strcmp(tmp_name,"\"vendor_storage\""))
            {
                printf("find vendor_storage partition\n");
                end = strlen(tmp_mtdname) - 1;
                while (tmp_mtdname[end] != 'd' && tmp_mtdname[end] != 'k')
                {
                    end--;
                }
                printf("%c\n",tmp_mtdname[end+1]);
                sprintf(g_vendor_mtdname,"/dev/mtd%c",tmp_mtdname[end+1]);
                printf("mtdname: %s\n",g_vendor_mtdname);
                fclose(fp);
                fp = NULL;
                return 0;
            }
        }
    }

    fclose(fp);
    fp = NULL;
    return -1;
}

/*
 * @brief get the id of vendor block
 * @param none
 * @return 0: success
 *        -1: fail
 */
int get_vendor_block_id()
{
    int fd;
    int block_cnt = 0;
    unsigned int start_addr = 0;

    fd = open (g_vendor_mtdname, O_SYNC | O_RDWR);
    if(fd < 0)
    {
        perror("fail to open\n");
        return -1;;
    }

    memset(&g_mtd, 0, sizeof(g_mtd));
    ioctl(fd, MEMGETINFO, &g_mtd);
    block_cnt = g_mtd.size / g_mtd.erasesize;
    printf("flash type: %d,size: %x,erasesize: %x, block_cnt: %d\n", g_mtd.type, g_mtd.size, g_mtd.erasesize, block_cnt);

    g_vendor_blockid = 0;

    for (start_addr = 0; start_addr <  g_mtd.size; start_addr += g_mtd.erasesize) 
    {
        loff_t offset = start_addr;
        int ret = ioctl(fd, MEMGETBADBLOCK, &offset); //bad block check
        if (ret > 0) 
        {
            printf("\nSkipping bad block offset 0x%08x, bad address offset 0x%08x\n", start_addr, offset);
            g_vendor_blockid = offset / g_mtd.erasesize + 1;
            if (g_vendor_blockid >= block_cnt)
                g_vendor_blockid = -1;
            continue;   //skip bad block
        } 
    }

    if (g_vendor_blockid < 0)
    {
        printf("get vendor block fail...\n");
        return -1;
    }
    else
        printf("vendor block id is %d\n", g_vendor_blockid);

    return 0;
}

/*
 * @brief get the specified length of data from the vendor block
 * @param data: the buffer used to save read data
 *       len: the size of data buffer
 * @return >=0: the actual length of data to be read
 *         <0: read error
 */
int read_vendor_block(char *data, int len)
{
	int fd;
    int readlen = 0;

    fd = open (g_vendor_mtdname, O_SYNC | O_RDWR);
    if(fd < 0)
    {
        perror("fail to open mtd dev\n");
        return -1;;
    }

	lseek(fd, g_vendor_blockid*g_mtd.erasesize, 0);

    if (len > g_mtd.erasesize)
        len = g_mtd.erasesize;

    readlen = read(fd, data, len);
    if (readlen < 0)
    {
        printf("read vendor_block failde\n");
        close(fd);
        return -1;
    }

    close(fd);
    return readlen;
}

/*
 * @brief erase the vendor block
 * @param none
 * @return 0: success
 *        -1: fail
 */
int erase_vendor_block()
{
#if USE_EMMC
    char command[128];
    int ret = 0;

    memset(command, 0, sizeof(command));
    sprintf(command, "dd bs=512 count=64 seek=80 if=/dev/zero of=/dev/mmcblk0");
    ret = system(command);
    if(ret == -1)
    {
        perror("fail to run system\n");
        return -1;
    }

     return 0;
#else
    int fd;
    int ret;
    struct erase_info_user eraseInfor;
	
    fd = open (g_vendor_mtdname, O_SYNC | O_RDWR);
    if(fd < 0)
    {
        perror("fail to open\n");
        return -1;
    }
	
    memset(&eraseInfor, 0, sizeof(eraseInfor));
    eraseInfor.start = g_vendor_blockid * g_mtd.erasesize;
    eraseInfor.length = g_mtd.erasesize;

    ret = ioctl(fd, MEMERASE, &eraseInfor);
    if (ret)
    {
        printf("Flash erase failed, WriteFd = %d, eraseAddr_now=0x%x\n", fd, eraseInfor.start);
        close(fd);
        return -1;
    }

	close(fd);
	printf("erase vendor block succeed!\n");
	
	return 0;
#endif
}

/*
 * @brief write data to the vendor block
 * @param data: the buffer used to write
 *         len: the size of data to be written
 * @return 0: success
 *        -1: fail
 */
int write_vendor_block(char *data, int len)
{
    int fd;
    int ret = 0;
    int i = 0;
    char *pLastPageData = NULL;
    int writelen = len; 
    int offset = g_vendor_blockid * g_mtd.erasesize;

    if (erase_vendor_block())
        return -1;

    if (writelen > g_mtd.erasesize)
    {
        writelen = g_mtd.erasesize;
        len = g_mtd.erasesize;
    }
    else
        writelen = (writelen + FLASH_PAGE_SIZE-1)/FLASH_PAGE_SIZE * FLASH_PAGE_SIZE;

    fd = open (g_vendor_mtdname, O_SYNC | O_RDWR);
    if(fd < 0)
    {
        perror("fail to open vendor dev...\n");
        return -1;;
    }

    for (i=0; i < (len/FLASH_PAGE_SIZE); i++)
    {
        lseek(fd, offset, 0);
        ret = write(fd, data+i*FLASH_PAGE_SIZE, FLASH_PAGE_SIZE);
        if(ret < 0)
        {
            printf("write page %d fail, (len: %d)errno: %d\n", i, len, errno);
            close(fd);
            return -1;
        }

        offset += ret;
    }

    if (writelen > len)
    {
        pLastPageData = (char*)malloc(FLASH_PAGE_SIZE);
        memset(pLastPageData, 0, FLASH_PAGE_SIZE);
        memcpy(pLastPageData, data+i*FLASH_PAGE_SIZE, len%FLASH_PAGE_SIZE);

        lseek(fd, offset, 0);
        ret = write(fd, pLastPageData, FLASH_PAGE_SIZE);
        if(ret < 0)
        {
            printf("write page %d fail, errno: %d\n", i, errno);
            free(pLastPageData);
            pLastPageData = NULL;
            close(fd);
            return -1;
        }

        free(pLastPageData);
        pLastPageData = NULL;
    }

    close(fd);
    printf("write data to vendor block success!\n");

    return 0;
}

/*
 * @brief get vendor info from the specified section of the vendor block
 * @param id: section index
 *      data: the buffer used to save read data
 *       len: the size of data buffer
 * @return 0: not write data yet
 *        -1: fail
 *        >0: the actual length of data to be read
 */
int read_vendor_info(int id, char *data, int len)
{
#if USE_EMMC
    char command[128];
    char *pBlockData = NULL;
    int readlen = 0;
    int fd;
    int ret = 0;

    if (id < SEC_SN || id >= SEC_MAX_CNT)
    {
        printf("input id %d is out of range...\n", id);
        return -1;
    }

    fd = open (EMMC_TEMP_DATA_FILE, O_CREAT | O_RDWR, 0666);
    if(fd < 0)
    {
        perror("fail to open\n");
        return -1;
    }

    memset(command, 0, sizeof(command));
    sprintf(command, "dd bs=512 count=1 skip=%d if=/dev/mmcblk0 of=%s", 80 + id, EMMC_TEMP_DATA_FILE);
    ret = system(command);
    if(ret == -1)
    {
        perror("fail to run system\n");
        return -1;
    }

    pBlockData = (char*)malloc(EMMC_BLK_SIZE);
    memset(pBlockData, 0, EMMC_BLK_SIZE);
    lseek(fd, 0, 0);
    ret = read(fd, pBlockData, EMMC_BLK_SIZE);
    if(ret != EMMC_BLK_SIZE)
    {
        printf("read emmc blk data failed, correct len: 512, read len: %d\n", ret);
        free(pBlockData);
        pBlockData = NULL;
        close(fd);
        return -1;
    }
    close(fd);

    readlen = *((int*)(pBlockData + MAGIC_DATA_MAX_LENGTH + sizeof(int)));
    if(readlen > VENDOR_DATA_MAX_LENGTH)
    {
        printf("vendor data length invalid, max len: %d, read len: %d\n", VENDOR_DATA_MAX_LENGTH, readlen);
        free(pBlockData);
        pBlockData = NULL;
        return -1;
    }
    if (len > readlen)
        len = readlen;

    strncpy(data, pBlockData + MAGIC_DATA_MAX_LENGTH + sizeof(int)*2, len);
    printf("read vendor info, data: %s, len: %d\n", data, len);
    free(pBlockData);
    pBlockData = NULL;

    memset(command, 0, sizeof(command));
    sprintf(command, "rm %s", EMMC_TEMP_DATA_FILE);
    ret = system(command);
    if(ret == -1)
    {
        perror("fail to run system\n");
        return -1;
    }

    return len;
#else
    int readlen = 0;
    char *pBlockData = NULL;
    
    if (id < SEC_SN || id >= SEC_MAX_CNT)
    {
        printf("input id %d is out of range...\n", id);
        return -1;
    }

    pBlockData = (char*)malloc(g_mtd.erasesize);
    memset(pBlockData, 0, g_mtd.erasesize);
    readlen = read_vendor_block(pBlockData, g_mtd.erasesize);
    if (readlen != g_mtd.erasesize)
    {
        printf("read block data length error, block size: %d, read len: %d\n", g_mtd.erasesize, readlen);
        free(pBlockData);
        pBlockData = NULL;
        return -1;
    }

    if (memcmp(pBlockData+id*VENDOR_INFO_LENGTH, MAGIC_INFO, strlen(MAGIC_INFO)))
    {
        printf("no vendor info has been written, please write vendor info first.\n");
        free(pBlockData);
        pBlockData = NULL;
        return 0;
    }

    readlen = *((int*)(pBlockData + id*VENDOR_INFO_LENGTH + MAGIC_DATA_MAX_LENGTH + sizeof(int)));
    if(readlen > VENDOR_DATA_MAX_LENGTH)
    {
        printf("vendor data length invalid, max len: %d, read len: %d\n", VENDOR_DATA_MAX_LENGTH, readlen);
        free(pBlockData);
        pBlockData = NULL;
        return -1;
    }

    if (len > readlen)
        len = readlen;

    strncpy(data, pBlockData + id*VENDOR_INFO_LENGTH + MAGIC_DATA_MAX_LENGTH + sizeof(int)*2, len);
    printf("read vendor info, data: %s, len: %d\n", data, len);
    
    free(pBlockData);
    pBlockData = NULL;
    return len;
#endif
}

/*
 * @brief erase the specified section of the vendor block
 * @param id: section index
 * @return 0: success
 *        -1: fail
 */
int erase_vendor_info(int id)
{
#if USE_EMMC
    char command[128];
    int ret = 0;

    memset(command, 0, sizeof(command));
    sprintf(command, "dd bs=512 count=1 seek=%d if=/dev/zero of=/dev/mmcblk0", 80 + id);
    ret = system(command);
    if(ret == -1)
    {
        perror("fail to run system\n");
        return -1;
    }

     return 0;
#else
    int readlen = 0;
    char *pBlockData = NULL;
    
    if (id < SEC_SN || id >= SEC_MAX_CNT)
    {
        printf("input id %d is out of range...\n", id);
        return -1;
    }

    pBlockData = (char*)malloc(g_mtd.erasesize);
    memset(pBlockData, 0, g_mtd.erasesize);
    readlen = read_vendor_block(pBlockData, g_mtd.erasesize);
    if (readlen != g_mtd.erasesize)
    {
        printf("read block data length error, block size: %d, read len: %d\n", g_mtd.erasesize, readlen);
        free(pBlockData);
        pBlockData = NULL;
        return -1;
    }

    memset(pBlockData + id*VENDOR_INFO_LENGTH, 0, VENDOR_INFO_LENGTH);

    if (write_vendor_block(pBlockData, g_mtd.erasesize))
    {
        printf("erase vendor info fail\n");
        free(pBlockData);
        pBlockData = NULL;
        return -1;
    }

    printf("erase vendor info success\n");
    free(pBlockData);
    pBlockData = NULL;
    return 0;
#endif
}

/*
 * @brief write vendor info to the specified section of the vendor block
 * @param id: section index
 *      data: the buffer used to write
 *       len: the size of data to be written
 * @return 0: success
 *        -1: fail
 */
int write_vendor_info(int id, char *data, int len)
{
#if USE_EMMC
    char command[128];
    char *pBlockData = NULL;
    int fd;
    int ret = 0;

    if (id < SEC_SN || id >= SEC_MAX_CNT)
    {
        printf("input id %d is out of range...\n", id);
        return -1;
    }

    if (len > VENDOR_DATA_MAX_LENGTH)
    {
        printf("the length of the vendor data has exceeded the max data length, data:%s len:%d, max data len: %d\n", data, len, MAGIC_DATA_MAX_LENGTH);
        //return -1;

        len = VENDOR_DATA_MAX_LENGTH;
    }

    pBlockData = (char*)malloc(EMMC_BLK_SIZE);
    memset(pBlockData, 0, EMMC_BLK_SIZE);
    memcpy(pBlockData, MAGIC_INFO, strlen(MAGIC_INFO));
    memcpy(pBlockData + MAGIC_DATA_MAX_LENGTH, &id, sizeof(int));
    memcpy(pBlockData + MAGIC_DATA_MAX_LENGTH + sizeof(int), &len, sizeof(int));
    memcpy(pBlockData + MAGIC_DATA_MAX_LENGTH + sizeof(int)*2, data, len);

    fd = open (EMMC_TEMP_DATA_FILE, O_CREAT | O_RDWR, 0666);
    if(fd < 0)
    {
        perror("fail to open\n");
        return -1;
    }
    lseek(fd, 0, 0);
    ret = write(fd, pBlockData, EMMC_BLK_SIZE);
    if(ret != EMMC_BLK_SIZE)
    {
        printf("write emmc blk data failed, correct len: 512, write len: %d\n", ret);
        free(pBlockData);
        pBlockData = NULL;
        close(fd);
        return -1;
    }
    close(fd);

    memset(command, 0, sizeof(command));
    sprintf(command, "dd bs=512 count=1 seek=%d if=%s of=/dev/mmcblk0", 80 + id, EMMC_TEMP_DATA_FILE);
    ret = system(command);
    if(ret == -1)
    {
        perror("fail to run system\n");
        return -1;
    }

    memset(command, 0, sizeof(command));
    sprintf(command, "rm %s", EMMC_TEMP_DATA_FILE);
    ret = system(command);
    if(ret == -1)
    {
        perror("fail to run system\n");
        return -1;
    }

    printf("write vendor info success\n");
    free(pBlockData);
    pBlockData = NULL;

    return 0;
#else
    int readlen = 0;
    char *pBlockData = NULL;
    
    if (id < SEC_SN || id >= SEC_MAX_CNT)
    {
        printf("input id %d is out of range...\n", id);
        return -1;
    }

    if (len > VENDOR_DATA_MAX_LENGTH)
    {
        printf("the length of the vendor data has exceeded the max data length, data:%s len:%d, max data len: %d\n", data, len, MAGIC_DATA_MAX_LENGTH);
        //return -1;

        len = VENDOR_DATA_MAX_LENGTH;
    }

    pBlockData = (char*)malloc(g_mtd.erasesize);
    memset(pBlockData, 0, g_mtd.erasesize);
    readlen = read_vendor_block(pBlockData, g_mtd.erasesize);
    if (readlen != g_mtd.erasesize)
    {
        printf("read block data length error, block size: %d, read len: %d\n", g_mtd.erasesize, readlen);
        free(pBlockData);
        pBlockData = NULL;
        return -1;
    }

    memset(pBlockData + id*VENDOR_INFO_LENGTH, 0, VENDOR_INFO_LENGTH);
    memcpy(pBlockData + id*VENDOR_INFO_LENGTH, MAGIC_INFO, strlen(MAGIC_INFO));
    memcpy(pBlockData + id*VENDOR_INFO_LENGTH + MAGIC_DATA_MAX_LENGTH, &id, sizeof(int));
    memcpy(pBlockData + id*VENDOR_INFO_LENGTH + MAGIC_DATA_MAX_LENGTH + sizeof(int), &len, sizeof(int));
    memcpy(pBlockData + id*VENDOR_INFO_LENGTH + MAGIC_DATA_MAX_LENGTH + sizeof(int)*2, data, len);

    if (write_vendor_block(pBlockData, g_mtd.erasesize))
    {
        printf("write vendor info fail\n");
        free(pBlockData);
        pBlockData = NULL;
        return -1;
    }

    printf("write vendor info success\n");
    free(pBlockData);
    pBlockData = NULL;
    return 0;
#endif
}
