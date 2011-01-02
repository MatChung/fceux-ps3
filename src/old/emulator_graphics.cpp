/*
 * emulator_graphics.cpp
 *
 *  Created on: Oct 22, 2010
 *      Author: Squarepusher2 
 */

#if 0

#include <stdio.h>
#include <string.h>

#include "emulator_graphics.h"

#include "conffile.h"

CellVideoOutState stored_video_state;

#ifdef PSGL
static GLuint vbo[2]; // vbo[0] main, vbo[2] vertexes
uint8_t *gl_buffer;
uint8_t *vertex_buf;
static GLuint tex_filter;
#else
//FIXME: DEFINE GCM required buffers when that day comes
#endif

static bool keep_aspect = EMULATOR_ASPECT_RATIO_4_3;
static bool smooth = true;

Quad screenQuad;

PSGLGraphics Graphics;


CellVideoOutState Graphics_GetVideoState()
{
	return stored_video_state;
}


bool Graphics_GetKeepAspectRatio()
{
	return keep_aspect;
}

bool Graphics_GetIsSmooth()
{
	return smooth;
}


void Graphics_ClearSurface()
{
#ifdef PSGL
	glClear(GL_COLOR_BUFFER_BIT);
#else
	cellGcmSetClearSurface(gCellGcmCurrentContext, CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);
	memset(display, 0, color_buf[0].pitch * color_buf[0].height);
#endif
}


void Graphics_InitDbgFont()
{
	#ifdef PSGL
		Graphics.InitDbgFont();
	#else
		cellGcmUtilInitDbgFont();
	#endif
}


void Graphics_DeInitDbgFont()
{
	#ifdef PSGL
		Graphics.DeinitDbgFont();
	#else
		cellGcmUtilDeinitDbgFont();
	#endif
}

void Graphics_SetViewports()
{
#ifdef PSGL
   float device_aspect = Graphics.GetDeviceAspectRatio();
   GLuint width = Graphics.GetResolutionWidth();
   GLuint height = Graphics.GetResolutionHeight();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

   // calculate the glOrtho matrix needed to transform the texture to the desired aspect ratio
   //float desired_aspect = keep_aspect ? SCREEN_REAL_ASPECT_RATIO : SCREEN_16_9_ASPECT_RATIO;
   float desired_aspect = Settings.PS3KeepAspect ? SCREEN_REAL_ASPECT_RATIO : SCREEN_16_9_ASPECT_RATIO;

   // If the aspect ratios of screen and desired aspect ratio are sufficiently equal (floating point stuff), 
   // assume they are actually equal.
   if ( (int)(device_aspect*1000) > (int)(desired_aspect*1000) )
   {
      float delta = (device_aspect / desired_aspect - 1.0) / 2.0 + 0.5;
      glOrthof(0.5 - delta, 0.5 + delta, 0, 1, -1, 1);
   }

   else if ( (int)(device_aspect*1000) < (int)(desired_aspect*1000) )
   {
      float delta = (desired_aspect / device_aspect - 1.0) / 2.0 + 0.5;
      glOrthof(0, 1, 0.5 - delta, 0.5 + delta, -1, 1);
   }
   else
      glOrthof(0, 1, 0, 1, -1, 1);

	glViewport(0, 0, width, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#else

#endif
}

void Graphics_SetAspectRatio(bool keep_aspect_ratio)
{
   //keep_aspect = keep_aspect_ratio;
   Settings.PS3KeepAspect = keep_aspect_ratio;
   Graphics_SetViewports();
}

int32_t Graphics_ChangeResolution(uint32_t resId, uint16_t refreshrateId)
{
#ifdef PSGL
	int32_t ret;
	CellVideoOutState video_state;
	CellVideoOutConfiguration video_config;
	CellVideoOutResolution resolution;
	uint32_t depth_pitch;

	cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &video_state);
	video_state.displayMode.refreshRates = refreshrateId;
	video_state.displayMode.resolutionId = resId;
	cellVideoOutGetResolution(video_state.displayMode.resolutionId, &resolution);

	depth_pitch = resolution.width * 4;

	memset(&video_config, 0, sizeof(CellVideoOutConfiguration));

	video_config.resolutionId = video_state.displayMode.resolutionId;
	video_config.format = CELL_VIDEO_OUT_BUFFER_COLOR_FORMAT_X8R8G8B8;
	video_config.aspect = video_state.displayMode.aspect;
	video_config.pitch = resolution.width * 4;

	Graphics_DeInitDbgFont();
	Graphics_DeInit();

	ret = cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &video_config, NULL, 0);
	if (ret != CELL_OK)
	{
		return ret;
	}

	Graphics_Init();
	Graphics_InitDbgFont();

#else
	cellChangeResolution(resId, refreshrateId);
#endif

	cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &stored_video_state);
}


void Graphics_RenderDbgFont()
{
#ifdef PSGL
	cellDbgFontDraw();
#else
	cellDbgFontDrawGcm();
#endif
}

void Graphics_Render(bool sync)
{
	#ifdef PSGL
		cellDbgFontDraw();
		psglSwap();
	#else
		cellGcmUtilRender(sync);
	#endif
}


void InitScreenQuad(int width, int height)
{
	screenQuad.v1.x = 0;
	screenQuad.v1.y = 0;
	screenQuad.v1.z = 0;

	screenQuad.v2.x = 0;
	screenQuad.v2.y = 1;
	screenQuad.v2.z = 0;

	screenQuad.v3.x = 1;
	screenQuad.v3.y = 1;
	screenQuad.v3.z = 0;

	screenQuad.v4.x = 1;
	screenQuad.v4.y = 0;
	screenQuad.v4.z = 0;

	screenQuad.t1.u = 0;
	screenQuad.t1.v = (float)height/SCREEN_RENDER_TEXTURE_HEIGHT;

	screenQuad.t2.u = 0;
	screenQuad.t2.v = 0;

	screenQuad.t3.u = (float)width/SCREEN_RENDER_TEXTURE_WIDTH;
	screenQuad.t3.v = 0;

	screenQuad.t4.u = (float)width/SCREEN_RENDER_TEXTURE_WIDTH;
	screenQuad.t4.v = (float)height/SCREEN_RENDER_TEXTURE_HEIGHT;

#ifdef PSGL
	memcpy(vertex_buf, ((float*)&screenQuad), 12 * sizeof(GLfloat));
	memcpy(vertex_buf + 128, ((float*)&screenQuad) + 12, 8 * sizeof(GLfloat));
#else
	//FIXME: copy screen quad in gcm buffer space when gcm is ready
#endif
}

void Graphics_SetTextureFilter(bool keep_smooth)
{
#ifdef PSGL
   //smooth = keep_smooth;
   Settings.PS3Smooth = keep_smooth;
   //if (smooth)
   if(Settings.PS3Smooth)
   {
	   tex_filter = GL_LINEAR;
   }
   else
   {
	   tex_filter = GL_NEAREST;
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex_filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex_filter);
#else
#endif
}

void Graphics_DeInit()
{
	#ifdef PSGL
		Graphics.Deinit();

		free(gl_buffer);
		free(vertex_buf);
	#else
		cellDbgFontExitGcm();
	#endif
}


#ifdef PSGL
int32_t _PSGLInit()
{
	Graphics.Init();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);

	//TODO: Enable it later
	//glDisable(GL_VSYNC_SCE);
	glEnable(GL_VSYNC_SCE);

	Graphics_SetViewports();

	// PSGL doesn't clear the screen on startup, so let's do that here.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	psglSwap();

	//gl_buffer = (uint8_t*)memalign(128, SCREEN_RENDER_TEXTURE_HEIGHT * FCEU_SCREEN_PITCH); // Allocate memory for texture.
	vertex_buf = (uint8_t*)memalign(128, 256);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glGenBuffers(2, vbo);

	//glBindBuffer(GL_TEXTURE_REFERENCE_BUFFER_SCE, vbo[0]);
	//glBufferData(GL_TEXTURE_REFERENCE_BUFFER_SCE, SCREEN_RENDER_TEXTURE_HEIGHT * FCEU_SCREEN_PITCH, gl_buffer, GL_DYNAMIC_DRAW);

	//glTextureReferenceSCE(GL_TEXTURE_2D, 1, SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT, 1, SCREEN_TEXTURE_PIXEL_FORMAT, SCREEN_RENDER_TEXTURE_PITCH, 0);


   // Use some initial values for the screen quad.
	InitScreenQuad(SCREEN_RENDER_TEXTURE_WIDTH, SCREEN_RENDER_TEXTURE_HEIGHT);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, 256, vertex_buf, GL_DYNAMIC_DRAW);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	glTexCoordPointer(2, GL_FLOAT, 0, (void*)128);

	glEnable(GL_TEXTURE_2D);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//Graphics_SetTextureFilter(smooth);
	Graphics_SetTextureFilter(Settings.PS3Smooth);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, vbo[0]);

	return CELL_OK;
}
#endif


#ifndef PSGL
int32_t _GCMInit()
{
	return cellGcmUtilInitDisplay();
}
#endif

int Graphics_Init()
{
	int32_t ret;
#ifdef PSGL
	ret = _PSGLInit();
#else
	ret = _GCMInit();
#endif

	if (ret == CELL_OK)
	{
		cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &stored_video_state);
	}

	return ret;
}


#endif
