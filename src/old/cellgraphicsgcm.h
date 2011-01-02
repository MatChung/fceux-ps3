#ifndef CELLGRAPHICS_GCM_H_
#define CELLGRAPHICS_GCM_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <cell/gcm.h>
#include <cell/dbgfont.h>

#include <sys/cdefs.h>
#include <sysutil/sysutil_sysparam.h>

#define CMD_BUFFER_SIZE (1024 * 64)
#define IO_BUFFER_SIZE (1024 * 1024)
#define COLOR_BUFFER_NUM 2
#define SHADER(x) extern uint32_t _binary_##x##_start; static unsigned char *shader_##x##_ptr = (unsigned char *)&_binary_##x##_start; CGprogram shader_##x; static void *shader_##x##_ucode; static uint32_t shader_##x##_offset

#define FONT_SIZE 1.0f
#define DEBUG_FONT_SIZE 1.0f


typedef struct {
	void *address;
	uint32_t offset;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
} color_buf_t;

typedef struct {
	uint8_t a;
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color4_t;

extern color4_t *display;
extern color4_t *display_back;
extern color_buf_t color_buf[COLOR_BUFFER_NUM];

void cellGcmUtilWaitFlip();

void cellGcmUtilFlip();

void cellGcmUtilInitShader();

void cellGcmUtilSetDrawEnv(void);

void cellGcmUtilSetRenderColor();

void cellGcmUtilRender(bool sync);

void cellGcmUtilSetRenderTarget(const uint32_t Index);

int32_t cellGcmUtilInitDisplay();

int32_t cellGcmUtilInitDbgFont();
void cellUtilDbgFontDraw();

int32_t cellChangeResolution(uint32_t resId, uint16_t refreshrateId=CELL_VIDEO_OUT_REFRESH_RATE_59_94HZ);

void cellGcmUtilDeinitDbgFont();

#endif /* CELLGRAPHICS_GCM_H */
