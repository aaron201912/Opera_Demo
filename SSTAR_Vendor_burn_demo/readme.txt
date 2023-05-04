build:
arm-linux-gnueabihf-gcc main.c burn.c -o vendor

或 aarch64-linux-gnu-gcc main.c burn.c -o vendor

setting:
#define USE_EMMC    1:使用EMMC 0:使用flash	
#define FLASH_TYPE   0:使用nor flash 1:使用nand flash
#define MAGIC_INFO  校验关键字，需要和USB_Write_SN_Tool设置的Magic一致
#define EMMC_TEMP_DATA_FILE 用于存放读写emmc存放临时数据的文件路径
typedef enum
{
    SEC_SN = 0,
    SEC_MAC,
    SEC_DID,
    SEC_KEY,
    SEC_SSID,
    SEC_RESERVE_0,
    SEC_RESERVE_1,
    SEC_RESERVE_2,
    SEC_MAX_CNT
} VendorBlock_e;    目前枚举的数值即为id_num与USB_Write_SN_Tool界面中的顺序从上往下依次匹配，可自行修改vendor_id名

run:
先找到vendor_storage 所在的mtd设备，然后检查vendor_storage的block，跳过坏块后找到最近的可正常操作的block。对于EMMC不需要这步。

write:向指定section id写数据
vendor [write] [id] [data]
eg:
./vendor write 0 01234567890123456789012345678901
./vendor write 1 abcdefghijabcdefghijabcdefghijab

read:从指定section id读数据
vendor [read] [id]
eg:
./vendor read 0
./vendor read 1

erase:擦除指定section id的数据
vendor [erase] [id]
eg:
./vendor erase 0
./vendor erase 1

eraseall:擦除所有section id的数据
vendor [eraseall]
eg:
./vendor eraseall

test log(nand):
/userdata # ./vendor read 0
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
read vendor info, data: 0123456789, len: 10
/userdata # ./vendor read 1
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
read vendor info, data: adbcefghijk, len: 11
/userdata # 
/userdata # ./vendor erase 0
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
erase vendor block succeed!
write data to vendor block success!
erase vendor info success
/userdata # 
/userdata # ./vendor erase 1
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
erase vendor block succeed!
write data to vendor block success!
erase vendor info success
/userdata # 
/userdata # ./vendor write 0 qwertyuiop
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
erase vendor block succeed!
write data to vendor block success!
write vendor info success
/userdata # 
/userdata # ./vendor write 1 asdfghjklasdafaffgafafafafafafaf
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
erase vendor block succeed!
write data to vendor block success!
write vendor info success
/userdata # ./vendor eraseall
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
erase vendor block succeed!
/userdata # 
/userdata # ./vendor write 0 qwertyuiop
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
erase vendor block succeed!
write data to vendor block success!
write vendor info success
/userdata # 
/userdata # ./vendor write 1 asdfghjklasdafaffgafafafafafafaf
find vendor_storage partition
8
mtdname: /dev/mtd8
flash type: 4,size: c0000,erasesize: 20000, block_cnt: 6

Skipping bad block offset 0x00000000, bad address offset 0x00000000
vendor block id is 1
erase vendor block succeed!
write data to vendor block success!
write vendor info success
/userdata # 
/userdata # dd if=/dev/mtd8 of=/userdata/vendor_blk
1536+0 records in
1536+0 records out
786432 bytes (768.0KB) copied, 0.312859 seconds, 2.4MB/s
/userdata # 

