/******************************************************************************* 
 *  -- Cellframework -  Open framework to abstract the common tasks related to
 *                      PS3 application development.
 *
 *  Copyright (C) 2010
 *       Hans-Kristian Arntzen
 *       Stephen A. Damm
 *       Daniel De Matteis
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ********************************************************************************/





#ifndef __PSGLGRAPHICS_H__
#define __PSGLGRAPHICS_H__

#include <vector>
#include "IGraphics.h"

#include <PSGL/psgl.h>
#include <PSGL/psglu.h>
#include <cell/dbgfont.h>

class PSGLGraphics : IGraphics<GLfloat, GLint>
{
public:
	PSGLGraphics();
	~PSGLGraphics();

	virtual void Init();
	virtual void Init(uint32_t resolutionId, uint16_t pal60Hz);
	virtual void Deinit();

	virtual void InitDbgFont();
	virtual void DeinitDbgFont();

	virtual GLfloat GetDeviceAspectRatio();
	virtual GLint GetResolutionWidth();
	virtual GLint GetResolutionHeight();
	
	int CheckResolution(uint32_t resId);
	void SwitchResolution(uint32_t resId, uint16_t pal60Hz);
	uint32_t GetInitialResolution();
	uint32_t GetCurrentResolution();
	void PreviousResolution();
	void NextResolution();
	int AddResolution(uint32_t resId);
	CellVideoOutState GetVideoOutState();
	void GetAllAvailableResolutions();
	void SetResolution();
private:
	PSGLdeviceParameters InitCommon(uint32_t resolutionId, uint16_t pal60Hz);
	PSGLdevice* psgl_device;
	PSGLcontext* psgl_context;
	GLuint gl_width;
	GLuint gl_height;
	CellVideoOutState stored_video_state;
	int currentResolution;
	uint32_t initialResolution;

	CellDbgFontConsoleId dbg_id;
	std::vector<uint32_t> supportedResolutions;
	virtual int32_t ChangeResolution(uint32_t resId, uint16_t pal60Hz);
};

static CellDbgFontConsoleId dbg_id;

//FIXME: classify this
void dprintf_console(const char* fmt, ...);
void dprintf_noswap(float x, float y, float scale, const char* fmt, ...);
void write_fps(void);
void gl_dprintf(float x, float y, float scale, const char* fmt, ...);


#endif
