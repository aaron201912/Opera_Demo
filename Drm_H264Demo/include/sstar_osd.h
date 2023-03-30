#ifndef _SSTAR_OSD_H_
#define _SSTAR_OSD_H_

#if defined (__cplusplus)
extern "C" {
#endif


#ifndef MIN
#define MIN(a, b)       (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)       (((a) > (b)) ? (a) : (b))
#endif


/* AARRGGBB */
typedef struct {
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char a;
} __attribute__((packed)) rgba_t;

typedef union {
    unsigned int  rgba;
    rgba_t        c;//unsigned char c[4];
} pixel_u;

typedef struct OSD_Rect_s
{
    int s32Xpos;
    int s32Ypos;
    int u32Width;
    int u32Height;
} OSD_Rect_t;

void test_fill_nv12(void* buf);
void test_fill_ARGB(void* pdate, OSD_Rect_t sRect);


#if defined (__cplusplus)
#endif
#endif

