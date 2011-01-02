/*
 * FceuGraphics.cpp
 *
 *  Created on: Nov 4, 2010
 *      Author: halsafar
 */

#include "FceuGraphics.h"

#include <assert.h>

#include "cellframework/logger/Logger.h"

// FIXME: make this use the faster ABGR_SCE instead of the easy RGBA

FceuGraphics::FceuGraphics() : PSGLGraphics(), gl_buffer(NULL), vertex_buf(NULL)
{
	m_smooth = true;
	m_pal60Hz = false;
	m_overscan = false;
	m_overscan_amount = 0.0f;
}


FceuGraphics::~FceuGraphics()
{
	DeInit();
}


void FceuGraphics::DeInit()
{
	if (vertex_buf)
	{
		free(vertex_buf);
		vertex_buf = NULL;
	}

	if (gl_buffer)
	{
		free(gl_buffer);
		gl_buffer = NULL;
	}
}


void FceuGraphics::Swap() const
{
	psglSwap();
}


void FceuGraphics::Draw() const
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_pitch / 4, m_height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);

	glDrawArrays(GL_QUADS, 0, 4);
	glFlush();

	Swap();
}


void FceuGraphics::Draw(uint8_t *XBuf, int nesw, int nesh)
{
	Clear();

	uint32_t* texture = MapPixels();
	pcpal* pcpalette = Palette;

	for(int i = 0; i != nesw * nesh; i ++)
	{
		texture[i] = ((pcpalette[XBuf[i]].r) << 24) | ((pcpalette[XBuf[i]].g) << 16) | (pcpalette[XBuf[i]].b << 8) | 0xFF;
	}

	UnmapPixels();

	Draw();
}

void FceuGraphics::FlushDbgFont() const
{
	cellDbgFontDraw();
}


void FceuGraphics::Sleep(uint64_t usec) const
{
	sys_timer_usleep(usec);
}


void FceuGraphics::Clear() const
{
	glClear(GL_COLOR_BUFFER_BIT);
}


uint32_t* FceuGraphics::MapPixels()
{
	return (uint32_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, GL_WRITE_ONLY);
}


void FceuGraphics::UnmapPixels()
{
	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER_ARB);
}


void FceuGraphics::SetAspectRatio(float ratio)
{
	m_ratio = ratio;
	set_aspect();
}


void FceuGraphics::SetDimensions(unsigned height, unsigned pitch)
{
	LOG_DBG("FceuGraphics->SetDimensions(%d, %d)\n", height, pitch);
	assert(height > 0);
	assert(pitch > 0);
	
	m_height = height;
	m_pitch = pitch;
	
	if (gl_buffer)
	{
		free(gl_buffer);
	}
	gl_buffer = (uint32_t*)memalign(128, m_height * m_pitch);
	memset(gl_buffer, 0, m_height * m_pitch);
	glBufferData(GL_PIXEL_UNPACK_BUFFER_ARB, m_height * m_pitch, gl_buffer, GL_STREAM_DRAW);
}


void FceuGraphics::SetRect(const Rect &view)
{
	LOG_DBG("FceuGraphics::Setrect(const Rect &view)\n");
	unsigned pitch = m_pitch >> 2;
	GLfloat tex_coords[] = {
		(float)view.x / pitch, (float)(view.y + view.h) / m_height,            // Upper Left
		(float)view.x / pitch, (float)view.y / m_height,                       // Lower Left
		(float)(view.x + view.w) / pitch, (float)view.y / m_height,            // Lower Right
		(float)(view.x + view.w) / pitch, (float)(view.y + view.h) / m_height  // Upper Right
	};
	
	memcpy(vertex_buf + 128, tex_coords, 8 * sizeof(GLfloat));
	glBufferData(GL_ARRAY_BUFFER, 256, vertex_buf, GL_STATIC_DRAW);
}


void FceuGraphics::Printf(float x, float y, const char *fmt, ...) const
{
	va_list ap;
	va_start(ap, fmt);
	cellDbgFontVprintf(x, y, 1.0, 0xffffffff, fmt, ap);
	va_end(ap);
	cellDbgFontDraw();
}


void FceuGraphics::DbgPrintf(const char *fmt, ...) const
{
	va_list ap;
	va_start(ap, fmt);
	cellDbgFontVprintf(0.1, 0.1, 1.0, 0xffffffff, fmt, ap);
	va_end(ap);
	cellDbgFontDraw();
	Swap();
	Sleep(600000);
}

int32_t FceuGraphics::ChangeResolution(uint32_t resId, uint16_t pal60Hz)
{
	LOG_DBG("SNES9xGraphics::ChangeResolution(%d, %d)\n", resId, pal60Hz);
	int32_t ret;

	PSGLGraphics::DeinitDbgFont();
	Deinit();
	
	PSGLGraphics::Init(resId, pal60Hz);
	PSGLInit();
	SetDimensions(240, 256 * 4);

	Rect r;
	r.x = 0;
	r.y = 0;
	r.w = 256;
	r.h = 240;
	SetRect(r);
	SetAspectRatio(m_ratio);
	PSGLGraphics::InitDbgFont();
	PSGLGraphics::SetResolution();
}

void FceuGraphics::PSGLInit()
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_VSYNC_SCE);

	uint32_t ret = InitCg();
	if (ret != CELL_OK)
	{
		LOG_DBG("Failed to InitCg: %d\n", __LINE__);
	}

	//FIXME: Change to SetViewports?
	set_aspect();

	vertex_buf = (uint8_t*)memalign(128, 256);
	assert(vertex_buf);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	Clear();
	Swap();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glGenBuffers(1, &pbo);
	glGenBuffers(1, &vbo);

	// Vertexes
	GLfloat vertexes[] = {
	  0, 0, 0,
	  0, 1, 0,
	  1, 1, 0,
	  1, 0, 0,
	  0, 1,
	  0, 0,
	  1, 0,
	  1, 1
	};

	memcpy(vertex_buf, vertexes, 12 * sizeof(GLfloat));
	memcpy(vertex_buf + 128, vertexes + 12, 8 * sizeof(GLfloat));

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 256, vertex_buf, GL_STATIC_DRAW);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, 0);
	glTexCoordPointer(2, GL_FLOAT, 0, (void*)128);

	glEnable(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, pbo);
	SetSmooth(m_smooth);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	Clear();
	Swap();
}

void FceuGraphics::Init()
{
	LOG_DBG("FceuGraphics::Init()\n");
	PSGLGraphics::Init(NULL, m_pal60Hz);
	PSGLInit();

	PSGLGraphics::SetResolution();
	GetAllAvailableResolutions();
}

void FceuGraphics::Init(uint32_t resId)
{
	LOG_DBG("FceuGraphics::Init(%d)\n", resId);
	PSGLGraphics::Init(resId, m_pal60Hz);
	PSGLInit();

	SetDimensions(240, 256 * 4);

	Rect r;
	r.x = 0;
	r.y = 0;
	r.w = 256;
	r.h = 240;
	SetRect(r);
	SetAspectRatio(m_ratio);
}


CGerror CheckCgError(int line)
{
	CGerror err = cgGetError ();

	if (err != CG_NO_ERROR)
	{
		LOG ("CG error:%d at line %d %s\n", err, line, cgGetErrorString (err));
	}

	return err;
}


CGprogram LoadShaderFromFile(CGcontext cgtx, CGprofile target, const char* filename, const char* entry)
{
	CGprogram id = cgCreateProgramFromFile(cgtx, CG_BINARY, filename, target, "main", NULL);
	if(!id)
	{
		LOG_DBG("Failed to load shader program >>%s<<\nExiting\n", filename);
		CheckCgError(__LINE__);
	}

	return id;
}


CGprogram LoadShaderFromSource(CGcontext cgtx, CGprofile target, const char* filename, const char* entry)
{
	CGprogram id = cgCreateProgramFromFile(cgtx, CG_SOURCE, filename, target, entry, NULL);
	if(!id)
	{
		LOG_DBG("Failed to load shader program >>%s<< \nExiting\n", filename);
		CheckCgError(__LINE__);
	}

	return id;
}


int32_t FceuGraphics::InitCg()
{
	LOG_DBG("FceuGraphics::InitCg()\n");

	cgRTCgcInit();

	LOG_DBG("FceuGraphics::InitCg() - About to create CgContext\n");
	_cgContext = cgCreateContext();
	if (_cgContext == NULL)
	{
		LOG_DBG("Error Creating Cg Context\n");
		return 1;
	}
	if (strlen(_curFragmentShaderPath.c_str()) > 0)
	{
		return LoadFragmentShader(_curFragmentShaderPath.c_str());
	}
	else
	{
		_curFragmentShaderPath = DEFAULT_SHADER_FILE;
		return LoadFragmentShader(_curFragmentShaderPath.c_str());
	}
}


int32_t FceuGraphics::LoadFragmentShader(string shaderPath)
{
	LOG_DBG("LoadFragmentShader(%s)\n", shaderPath.c_str());

	// store the cur path
	_curFragmentShaderPath = shaderPath;

	_vertexProgram = LoadShaderFromSource(_cgContext, CG_PROFILE_SCE_VP_RSX, shaderPath.c_str(), "main_vertex");
	if (_vertexProgram <= 0)
	{
		LOG_DBG("Error loading vertex shader...");
		return 1;
	}

	_fragmentProgram = LoadShaderFromSource(_cgContext, CG_PROFILE_SCE_FP_RSX, shaderPath.c_str(), "main_fragment");
	if (_fragmentProgram <= 0)
	{
		LOG_DBG("Error loading fragment shader...");
		return 1;
	}

	// bind and enable the vertex and fragment programs
	cgGLEnableProfile(CG_PROFILE_SCE_VP_RSX);
	cgGLEnableProfile(CG_PROFILE_SCE_FP_RSX);
	cgGLBindProgram(_vertexProgram);
	cgGLBindProgram(_fragmentProgram);

	// acquire mvp param from v shader
	_cgpModelViewProj = cgGetNamedParameter(_vertexProgram, "modelViewProj");
	if (CheckCgError (__LINE__) != CG_NO_ERROR)
	{
		// FIXME: WHY DOES THIS GIVE ERROR ON OTHER LOADS
		//return 1;
	}
	
	_cgpVideoSize = cgGetNamedParameter(_fragmentProgram, "IN.video_size");
	_cgpTextureSize = cgGetNamedParameter(_fragmentProgram, "IN.texture_size");
	_cgpOutputSize = cgGetNamedParameter(_fragmentProgram, "IN.output_size");
	
	LOG_DBG("SUCCESS - LoadFragmentShader(%s)\n", shaderPath.c_str());
	return CELL_OK;
}


void FceuGraphics::UpdateCgParams(unsigned width, unsigned height, unsigned tex_width, unsigned tex_height)
{
	LOG_DBG("FceuGraphics::UpdateCgParams(%d, %d, %d, %d)\n", width, height, tex_width, tex_height);
	cgGLSetStateMatrixParameter(_cgpModelViewProj, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);
	cgGLSetParameter2f(_cgpVideoSize, width, height);
	cgGLSetParameter2f(_cgpTextureSize, tex_width, tex_height);
	cgGLSetParameter2f(_cgpOutputSize, _cgViewWidth, _cgViewHeight);
}

void FceuGraphics::SetPAL60Hz(bool pal60Hz)
{
	LOG_DBG("FceuGraphics::SetPAL60Hz(%d)\n",pal60Hz);
	m_pal60Hz = pal60Hz;
}

bool FceuGraphics::GetPAL60Hz()
{
	LOG_DBG("FceuGraphics::GetPAL60Hz()\n");
	return m_pal60Hz;
}

void FceuGraphics::SetSmooth(bool smooth)
{
	LOG_DBG("FceuGraphics::SetSmooth(%d)\n", smooth);
	m_smooth = smooth;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
}


void FceuGraphics::SetOverscan(bool will_overscan, float amount)
{
	LOG_DBG("FceuGraphics::SetOverscan(%d, %f)\n", will_overscan, amount);
	m_overscan_amount = amount;
	m_overscan = will_overscan;
	set_aspect();
}

//FIXME: Change name to SetViewports?
void FceuGraphics::set_aspect()
{
	LOG_DBG("FceuGraphics::set_aspect()\n");
	float device_aspect = this->GetDeviceAspectRatio();
	GLuint width = this->GetResolutionWidth();
	GLuint height = this->GetResolutionHeight();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// calculate the glOrtho matrix needed to transform the texture to the desired aspect ratio
	float desired_aspect = m_ratio;

	GLuint real_width = width, real_height = height;

	// If the aspect ratios of screen and desired aspect ratio are sufficiently equal (floating point stuff),
	// assume they are actually equal.
	if ( (int)(device_aspect*1000) > (int)(desired_aspect*1000) )
	{
		float delta = (desired_aspect / device_aspect - 1.0) / 2.0 + 0.5;
		glViewport(width * (0.5 - delta), 0, 2.0 * width * delta, height);
		real_width = (int)(2.0 * width * delta);
	}

	else if ( (int)(device_aspect*1000) < (int)(desired_aspect*1000) )
	{
		float delta = (device_aspect / desired_aspect - 1.0) / 2.0 + 0.5;
		glViewport(0, height * (0.5 - delta), width, 2.0 * height * delta);
		real_height = (int)(2.0 * height * delta);
	}
	else
	{
		glViewport(0, 0, width, height);
	}

	if (m_overscan)
	{
		glOrthof(-m_overscan_amount/2, 1 + m_overscan_amount/2, -m_overscan_amount/2, 1 + m_overscan_amount/2, -1, 1);
	}
	else
	{
		glOrthof(0, 1, 0, 1, -1, 1);
	}

	_cgViewWidth = real_width;
	_cgViewHeight = real_height;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}


