#ifndef PSGL

#include <stdio.h>
#include <string.h>
#include <math.h>


#include <sys/timer.h>

#include <cell/gcm.h>


#include "cellgraphicsgcm.h"


using namespace cell::Gcm;


static const uint32_t dma_label_id = 128;
static uint32_t *dma_label;
static uint32_t dma_label_value;

static uint32_t local_heap_next;
static void *host_io_buffer;

SHADER(fpshader_fpo);

color4_t *display;
color4_t *display_back;
uint32_t frame_index;
color_buf_t color_buf[COLOR_BUFFER_NUM];
uint32_t depth_pitch, depth_offset, display_offset, display_back_offset;


void *localHeapAlloc(const uint32_t size)
{
	uint32_t allocated_size = (size + 1023) & (~1023);
	void *addr = (void *)local_heap_next;

	local_heap_next += allocated_size;
	return addr;
}


void *localHeapAlign(const uint32_t align, const uint32_t size)
{
	local_heap_next = (local_heap_next + align - 1) & (~(align - 1));
	return localHeapAlloc(size);
}


void cellGcmUtilWaitFlip()
{
	while (cellGcmGetFlipStatus() != 0)
	{
		sys_timer_usleep(300);
	}
	cellGcmResetFlipStatus();
}


void cellGcmUtilSetDrawEnv(void)
{
	cellGcmSetColorMask(gCellGcmCurrentContext, CELL_GCM_COLOR_MASK_B|
			CELL_GCM_COLOR_MASK_G|
			CELL_GCM_COLOR_MASK_R|
			CELL_GCM_COLOR_MASK_A);

	cellGcmSetColorMaskMrt(gCellGcmCurrentContext, 0);
	uint16_t x,y,w,h;
	float min, max;
	float scale[4],offset[4];

	x = 0;
	y = 0;
	w = color_buf[0].width;
	h = color_buf[0].height;
	min = 0.0f;
	max = 1.0f;
	scale[0] = w * 0.5f;
	scale[1] = h * -0.5f;
	scale[2] = (max - min) * 0.5f;
	scale[3] = 0.0f;
	offset[0] = x + scale[0];
	offset[1] = h - y + scale[1];
	offset[2] = (max + min) * 0.5f;
	offset[3] = 0.0f;

	cellGcmSetViewport(gCellGcmCurrentContext, x, y, w, h, min, max, scale, offset);
	cellGcmSetClearColor(gCellGcmCurrentContext, (64<<0)|(64<<8)|(64<<16)|(64<<24));
}

void cellGcmUtilFlip()
{
	static bool first = true;

	if (!first) cellGcmUtilWaitFlip();
	else cellGcmResetFlipStatus();

	if (cellGcmSetFlip(frame_index) != CELL_OK) return;
	cellGcmFlush();

	cellGcmUtilSetDrawEnv();
	cellGcmUtilSetRenderColor();

	cellGcmSetWaitFlip(gCellGcmCurrentContext);

	// New render target
	frame_index = (frame_index + 1) % COLOR_BUFFER_NUM;
	cellGcmUtilSetRenderTarget(frame_index);

	first = false;
}


void cellGcmUtilSetRenderColor()
{
	cellGcmSetFragmentProgram(shader_fpshader_fpo, shader_fpshader_fpo_offset);
}

void cellGcmUtilRender(bool sync)
{
	unsigned int render_to = frame_index;

	cellGcmSetClearSurface(CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);
	cellGcmSetTransferData(gCellGcmCurrentContext,
		       	CELL_GCM_TRANSFER_MAIN_TO_LOCAL, color_buf[render_to].offset, color_buf[render_to].pitch, 
			display_offset, color_buf[0].pitch, color_buf[0].pitch, color_buf[0].height);
	
	cellDbgFontDrawGcm();	// always draw dbgfont buffer items immediately before a flip
	cellGcmUtilFlip();

	if (sync)
	{
		cellGcmSetWriteBackEndLabel(dma_label_id, ++dma_label_value);
		cellGcmSetWaitLabel(dma_label_id, dma_label_value);
	}

	uint32_t display_offset_tmp = display_offset;
	color4_t *display_tmp = display;

	display = display_back;
	display_offset = display_back_offset;

	display_back = display_tmp;
	display_back_offset = display_offset_tmp;
}

void cellGcmUtilSetRenderTarget(const uint32_t Index)
{
	CellGcmSurface sf;
	sf.colorFormat 	= CELL_GCM_SURFACE_A8R8G8B8;
	sf.colorTarget	= CELL_GCM_SURFACE_TARGET_0;
	sf.colorLocation[0]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorOffset[0] 	= color_buf[Index].offset;
	sf.colorPitch[0] 	= color_buf[Index].pitch;

	sf.colorLocation[1]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[2]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorLocation[3]	= CELL_GCM_LOCATION_LOCAL;
	sf.colorOffset[1] 	= 0;
	sf.colorOffset[2] 	= 0;
	sf.colorOffset[3] 	= 0;
	sf.colorPitch[1]	= 64;
	sf.colorPitch[2]	= 64;
	sf.colorPitch[3]	= 64;

	sf.depthFormat 	= CELL_GCM_SURFACE_Z24S8;
	sf.depthLocation	= CELL_GCM_LOCATION_LOCAL;
	sf.depthOffset	= depth_offset;
	sf.depthPitch 	= depth_pitch;

	sf.type		= CELL_GCM_SURFACE_PITCH;
	sf.antialias	= CELL_GCM_SURFACE_CENTER_1;

	sf.width 		= color_buf[0].width;
	sf.height 		= color_buf[0].height;
	sf.x 		= 0;
	sf.y 		= 0;
	cellGcmSetSurface(&sf);	
}

void cellGcmUtilClearSurface()
{
	cellGcmSetClearSurface(CELL_GCM_CLEAR_Z | CELL_GCM_CLEAR_R | CELL_GCM_CLEAR_G | CELL_GCM_CLEAR_B | CELL_GCM_CLEAR_A);	
}

void cellGcmUtilDeinitDbgFont()
{
	cellDbgFontExitGcm();
}

int32_t cellGcmUtilInitDbgFont()
{
	int frag_size = CELL_DBGFONT_FRAGMENT_SIZE;
	int font_tex = CELL_DBGFONT_TEXTURE_SIZE;
	int vertex_size = 8192 * CELL_DBGFONT_VERTEX_SIZE;
	int local_size = frag_size + vertex_size + font_tex;
	void* localmem = (void*)localHeapAlign(128, local_size);

	CellDbgFontConfigGcm cfg;
	memset(&cfg, 0, sizeof(CellDbgFontConfigGcm));
	cfg.localBufAddr = (sys_addr_t)localmem;
	cfg.localBufSize = local_size;
	cfg.mainBufAddr = NULL;
	cfg.mainBufSize = 0;
	cfg.option = CELL_DBGFONT_VERTEX_LOCAL |
		CELL_DBGFONT_TEXTURE_LOCAL |
		CELL_DBGFONT_SYNC_OFF;

	cellDbgFontExitGcm();

	return cellDbgFontInitGcm(&cfg);
}


void cellGcmUtilInitShader()
{
	shader_fpshader_fpo = (CGprogram)shader_fpshader_fpo_ptr;
	cellGcmCgInitProgram(shader_fpshader_fpo);

	uint32_t ucode_size;
	void *ucode;
	cellGcmCgGetUCode(shader_fpshader_fpo, &ucode, &ucode_size);
	shader_fpshader_fpo_ucode = localHeapAlign(128, ucode_size);
	memcpy(shader_fpshader_fpo_ucode, ucode, ucode_size);
	cellGcmAddressToOffset(shader_fpshader_fpo_ucode, &shader_fpshader_fpo_offset);
}


void cellGcmBuildDrawBuffers()
{
	CellVideoOutState video_state;
	CellVideoOutResolution resolution;
	CellGcmConfig gcm_config;
	uint32_t color_buffer_size;
	void *color_buffer_base;

	cellGcmGetConfiguration(&gcm_config);

	/* reset local memory: should be done AFTER cellVideoOutConfigure() returns success value */
	local_heap_next = (uint32_t)gcm_config.localAddress;
	cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &video_state);
	cellVideoOutGetResolution(video_state.displayMode.resolutionId, &resolution);
	color_buffer_size = resolution.width * resolution.height * 4;
	color_buffer_base = localHeapAlign(16, color_buffer_size * COLOR_BUFFER_NUM);
	memset(color_buffer_base, 0, color_buffer_size);

	for (int i = 0; i < COLOR_BUFFER_NUM; ++i)
	{
		void *buf_addr = (void *)((uint32_t)color_buffer_base + (i * color_buffer_size));

		color_buf[i].address = buf_addr;
		color_buf[i].width = resolution.width;
		color_buf[i].height = resolution.height;
		color_buf[i].pitch = resolution.width * 4;

		cellGcmAddressToOffset(buf_addr, &color_buf[i].offset);
		cellGcmSetDisplayBuffer(i, color_buf[i].offset, color_buf[i].pitch, color_buf[i].width, color_buf[i].height);
	}

	// init the shaders next
	cellGcmUtilInitShader();

	// not researched this yet - unknown functionality.
	void *depth_addr = localHeapAlign(16, depth_pitch * resolution.height);
	cellGcmAddressToOffset(depth_addr, &depth_offset);

	dma_label = cellGcmGetLabelAddress(dma_label_id);
	*dma_label = dma_label_value;

	uint32_t buf_size = sizeof(color4_t) * color_buf[0].width * color_buf[0].height;
	buf_size = (buf_size + 1024 * 1024) & (~(1024 * 1024 - 1));

	color4_t *buf = (color4_t *)memalign(1024 * 1024, buf_size);
	cellGcmMapMainMemory(buf, buf_size, &display_back_offset);

	memset(buf, 0, buf_size);
	display_back = buf;

	buf = (color4_t *)memalign(1024 * 1024, buf_size);
	cellGcmMapMainMemory(buf, buf_size, &display_offset);

	memset(buf, 0, buf_size);
	display = buf;

	memset(display, 0, color_buf[0].height * color_buf[0].pitch);
	memset(display_back, 0, color_buf[0].height * color_buf[0].pitch);

	cellGcmUtilSetDrawEnv();

	cellGcmUtilSetRenderColor();
	cellGcmUtilSetRenderTarget(frame_index);
}


// FIXME: Comment this for ppl to understand
int32_t cellChangeResolution(uint32_t resId, uint16_t refreshrateId)
{
	int32_t ret;
	CellVideoOutState video_state;
	CellVideoOutConfiguration video_config;
	CellVideoOutResolution resolution;

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

	cellGcmSetWaitFlip(gCellGcmCurrentContext);
	cellGcmFinish(gCellGcmCurrentContext, 0);
	ret = cellVideoOutConfigure(CELL_VIDEO_OUT_PRIMARY, &video_config, NULL, 0);
	if (ret != CELL_OK)
	{
		return ret;
	}
	else
	{
		cellGcmBuildDrawBuffers();

		cellGcmSetFlipMode(CELL_GCM_DISPLAY_VSYNC);
	}

	// finally init our font
	return cellGcmUtilInitDbgFont();
}



int32_t cellGcmUtilInitDisplay ()
{
	int32_t ret;
	CellVideoOutState video_state;

	host_io_buffer = memalign(1024 * 1024, IO_BUFFER_SIZE);
	cellGcmInit(CMD_BUFFER_SIZE, IO_BUFFER_SIZE, host_io_buffer);

	ret = cellVideoOutGetState(CELL_VIDEO_OUT_PRIMARY, 0, &video_state);

	return cellChangeResolution(video_state.displayMode.resolutionId);
}


#endif
