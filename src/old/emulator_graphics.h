#if 0
/*
 * emulator_graphics.h
 *
 *  Created on: Oct 22, 2010
 *      Author: Squarepusher2
 */
#ifndef EMULATOR_GRAPHICS_H_
#define EMULATOR_GRAPHICS_H_

#include <sysutil/sysutil_sysparam.h>

#include "cellframework/graphics/PSGLGraphics.h"
#include "cellframework/graphics/GraphicsTypes.h"

#ifdef EMUDEBUG
	#ifdef PSGL
	#define EMU_DBG(fmt, args...) do {\
	   gl_dprintf(0.1, 0.1, 1.0, fmt, ##args);\
	   sys_timer_usleep(EMU_DBG_DELAY);\
	   } while(0)
	#else
	#define EMU_DBG(fmt, args...) do {\
	   cellDbgFontPrintf(0.1f, 0.1f, DEBUG_FONT_SIZE, RED, fmt, ##args);\
	   sys_timer_usleep(EMU_DBG_DELAY);\
	   } while(0)
	#endif
#else
#define EMU_DBG(fmt, args...) ((void)0)
#endif

#define EMULATOR_ASPECT_RATIO_16_9 0;
#define EMULATOR_ASPECT_RATIO_4_3 1;

#define FCEU_RENDER_TEXTURE_WIDTH (256.0)
#define FCEU_RENDER_TEXTURE_HEIGHT (256.0)

#define SCREEN_16_9_ASPECT_RATIO (16.0/9)
#define SCREEN_4_3_ASPECT_RATIO (4.0/3)
#define NES_ASPECT_RATIO SCREEN_4_3_ASPECT_RATIO

#define FCEU_SCREEN_PITCH (256)
//#define FCEU_SCREEN_PITCH (256)

#define SCREEN_RENDER_TEXTURE_WIDTH FCEU_RENDER_TEXTURE_WIDTH
#define SCREEN_RENDER_TEXTURE_HEIGHT FCEU_RENDER_TEXTURE_HEIGHT
#define SCREEN_REAL_ASPECT_RATIO NES_ASPECT_RATIO
#define SCREEN_RENDER_TEXTURE_PITCH FCEU_SCREEN_PITCH

#define SCREEN_TEXTURE_PIXEL_FORMAT GL_RGB8



extern uint8_t *vertex_buf;
extern Quad screenQuad;
extern uint8_t *gl_buffer;

bool Graphics_GetKeepAspectRatio();
bool Graphics_GetIsSmooth();
void Graphics_ClearSurface();
void Graphics_InitDbgFont();
void Graphics_DeInitDbgFont();
void Graphics_RenderDbgFont();
void Graphics_SetViewports();
void Graphics_SetAspectRatio(bool keep_aspect_ratio);
void Graphics_Render(bool sync);
void Graphics_SetTextureFilter(bool keep_smooth);
int Graphics_Init();
void Graphics_DeInit();
int32_t Graphics_ChangeResolution(uint32_t resId, uint16_t refreshrateId);
CellVideoOutState Graphics_GetVideoState();

#endif /* EMULATOR_GRAPHICS_H_ */

#endif
