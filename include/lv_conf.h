#ifndef LV_CONF_H
#define LV_CONF_H

#ifndef __ASSEMBLY__
#include <stdint.h>
#endif

/*------------------
 * GENERAL CONFIG
 *------------------*/
#define LV_COLOR_DEPTH 16
#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 320
#define LV_MEM_SIZE (48U * 1024U)  // ajuste conforme RAM disponivel
#define LV_USE_LOG 0
#define LV_COLOR_16_SWAP 1
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/*------------------
 * FONT CONFIG
 *------------------*/
#define LV_FONT_DEFAULT &lv_font_montserrat_16
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_22 1

/*------------------
 * FEATURE CONFIG
 *------------------*/
#define LV_USE_LODEPNG 0
#define LV_USE_LIBPNG 0
#define LV_USE_BMP 0
#define LV_USE_SJPG 0
#define LV_USE_GIF 0
#define LV_USE_QRCODE 0
#define LV_USE_BARCODE 0
#define LV_USE_SYSMON 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_FILE_EXPLORER 0

#endif // LV_CONF_H
