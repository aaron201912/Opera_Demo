/* SigmaStar trade secret */
/* Copyright (c) [2019~2020] SigmaStar Technology.
All rights reserved.

Unless otherwise stipulated in writing, any and all information contained
herein regardless in any format shall remain the sole proprietary of
SigmaStar and be kept in strict confidence
(SigmaStar Confidential Information) by the recipient.
Any unauthorized act including without limitation unauthorized disclosure,
copying, use, reproduction, sale, distribution, modification, disassembling,
reverse engineering and compiling of the contents of SigmaStar Confidential
Information is unlawful and strictly prohibited. SigmaStar hereby reserves the
rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#ifndef __DOT_MATRIX_FONT_H__
#define __DOT_MATRIX_FONT_H__

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

typedef struct
{
    int         charNumPerLine; // ÿ��/����ʾ���ַ���, ����Ҳһ���ַ�
    int         bgColor;        // ����ɫ
    int         fgColor;        // ǰ��ɫ
    int         leftMargin;     // margin ����
    int         rightMargin;
    int         upMargin;
    int         downMargin;
    int         verticalFlag;   // ���Ű�
    int         charSpace;      // �ַ����
    int         lineSpace;      // �м��
} DMF_BitMapAttr_S;

typedef struct
{
    int             width;
    int             height;

    unsigned char*  pBuf;
    int             bufLen;
} DMF_BitMapData_S;

int DMF_LoadBitMapFile(int width, int height, const char *szFile);

int DMF_SetBitMapAttr(DMF_BitMapAttr_S *pstDMFBitMapAttr);

int DMF_DumpToBMPFile(const char *string, char *szFile);

int DMF_DumpToBMPBuf(const char *string, DMF_BitMapData_S *pstBitMapData);

int DMF_CloseBitMapFile(void);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif  // __DOT_MATRIX_FONT_H__

