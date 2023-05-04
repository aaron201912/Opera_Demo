#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "burn.h"

void usage_info()
{
    printf("please input valid parameter!\n");
    printf("eg: vendor [write] [id] [data]\n");
    printf("        - write data to the specified part of vendor partition\n");
    printf("    vendor [read] [id]\n");
    printf("        - read data from the specified part of vendor partition");
    printf("    vendor [erase] [id]\n");
    printf("        - erase the specified part of vendor partition");
    printf("    vendor [eraseall]\n");
    printf("        - erase the full of vendor partition\n");
}

int main(int argc, char **argv)
{
    FILE *fp = NULL;
	int ret = -1;
	int fd;

    
#if USE_EMMC == 0
    // check mtd dev
    ret = get_vendor_partition_name();
    if (ret)
    {
        printf("can't find vendor_storage partition.\n");
        return -1;
    }

    // check bad block
    ret = get_vendor_block_id();
    if (ret < 0)
    {
        printf("no blocks are available.\n");
        return -1;
    }
#endif 

    // check input param
    if ((argc < 2) || (argc > 4))
    {
        usage_info();
        return -1;
    }

    if ((argc == 2) && !strcmp(argv[1], "eraseall"))
    {
        ret = erase_vendor_block();
        if (ret)
        {
            printf("erase block fail...\n");
            return -1;
        }
    }
    else if ((argc == 3) && !strcmp(argv[1], "erase"))
    {
        int sec_id = atoi(argv[2]);

        ret = erase_vendor_info(sec_id);
        if (ret)
        {
            printf("erase vendor section %d fail...\n", sec_id);
            return -1;
        }
    }
    else if ((argc == 3) && !strcmp(argv[1], "read"))
    {
        int sec_id = atoi(argv[2]);
        char read_buf[VENDOR_DATA_MAX_LENGTH + 1] = {0};

        ret = read_vendor_info(sec_id, read_buf, sizeof(read_buf));
        if (ret < 0)
            return -1;
    }
    else if (argc == 4 && !strcmp(argv[1], "write"))
    {
        int sec_id = atoi(argv[2]);
        int data_len = strlen(argv[3]);

        ret = write_vendor_info(sec_id, argv[3], data_len);
        if (ret)
            return -1; 
    }
    else
    {
        printf("param invalid...\n");
        return -1;
    }
	
    return 0;
}