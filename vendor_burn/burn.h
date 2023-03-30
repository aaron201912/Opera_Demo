#ifndef __BURNKEY_H__
#define __BURNKEY_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * vendor_id format(nand || nor):
 *
 *  0       16 20  24                           88                    128     144 148 152                          216                  256		(bytes)
 *  -------------------------------------------------------------------------------------------------------------------------------------
 *  | Magic |id_num|len|          Data              |     reserve         | Magic |id_num|len|          Data               |     reserve        |   ......
 *  -------------------------------------------------------------------------------------------------------------------------------------
 *  |<-                         id_num 0                           ->|<-                         id_num 1                           ->|
 */

/*
 * vendor_id format(emmc):
 *
 * block0
 *  0       16 20  24                           88                    511  	(bytes)
 *  --------------------------------------------------------------------
 *  | Magic |id_num|len|          Data              |     reserve         | 
 *  --------------------------------------------------------------------
 *  |<-                         id_num 0                           ->|
 * 
 * block1
*  0       16 20  24                           88                     511  	(bytes)
 *  --------------------------------------------------------------------
 *  | Magic |id_num|len|          Data              |     reserve         | 
 *  --------------------------------------------------------------------
 *  |<-                         id_num 1                           ->|
 *  
 *                              ......
 */


#ifndef USE_EMMC
    #define USE_EMMC    0
#endif 

#ifdef USE_EMMC
    #define EMMC_TEMP_DATA_FILE     "/tmp/emmc_temp.bin"
#endif 

#define FLASH_TYPE      1    //0:nor 1：nand

#define MAGIC_INFO              "vendor"
#define MAGIC_DATA_MAX_LENGTH   16          //不建议修改
#define VENDOR_DATA_MAX_LENGTH  64          //不建议修改
#define VENDOR_INFO_LENGTH      128         //不建议修改
#define NAND_PAGE_SIZE          (2*1024)    //根据NAND数据手册修改
#define NOR_PAGE_SIZE           (4*1024)    //根据NAND数据手册修改
#define EMMC_BLK_SIZE           512         //固定不变

#if FLASH_TYPE
    #define FLASH_PAGE_SIZE NAND_PAGE_SIZE
#else
    #define FLASH_PAGE_SIZE NOR_PAGE_SIZE
#endif

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
} VendorBlock_e;        


/*
 * @brief get the name of vendor partition
 * @param none
 * @return 0: success
 *        -1: fail
 */
int get_vendor_partition_name();

/*
 * @brief get the id of vendor block
 * @param none
 * @return 0: success
 *        -1: fail
 */
int get_vendor_block_id();

/*
 * @brief erase the vendor block
 * @param none
 * @return 0: success
 *        -1: fail
 */
int erase_vendor_block();

/*
 * @brief get vendor info from the specified section of the vendor block
 * @param id: section index
 *      data: the buffer used to save read data
 *       len: the size of data buffer
 * @return 0: not write data yet
 *        -1: fail
 *        >0: the actual length of data to be read
 */
int read_vendor_info(int id, char *data, int len);

/*
 * @brief erase the specified section of the vendor block
 * @param id: section index
 * @return 0: success
 *        -1: fail
 */
int erase_vendor_info(int id);

/*
 * @brief write vendor info to the specified section of the vendor block
 * @param id: section index
 *      data: the buffer used to write
 *       len: the size of data to be written
 * @return 0: success
 *        -1: fail
 */
int write_vendor_info(int id, char *data, int len);

#ifdef __cplusplus
}
#endif

#endif