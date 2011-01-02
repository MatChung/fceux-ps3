#specify build tools
CELL_BUILD_TOOLS	=	GCC
#explicitly set some cell sdk defaults
CELL_SDK		?=	/usr/local/cell
# CELL_GPU_TYPE (currently RSX is only one option)  
CELL_GPU_TYPE		=	RSX    
#CELL_PSGL_VERSION is debug, dpm or opt  
CELL_PSGL_VERSION	=	opt  

CELL_MK_DIR		?=	$(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

# bin paths
MKFSELF			=	$(CELL_HOST_PATH)/bin/make_fself
MKFSELF_NPDRM		=	$(CELL_HOST_PATH)/bin/make_fself_npdrm
MKPKG_NPDRM		=	$(CELL_HOST_PATH)/bin/make_package_npdrm
STRIP			=	$(CELL_HOST_PATH)/ppu/bin/ppu-lv2-strip
COPY			=	cp

# important directories
SRC_DIR			=	./src
CELL_FRAMEWORK_DIR	=	./src/cellframework
FCEU_API_DIR		=	./src/fceu
UTIL_DIR		=	./utils

# all source directories
SOURCES			:=	. $(FCEU_API_DIR)/boards \
				$(FCEU_API_DIR)/mappers \
				$(FCEU_API_DIR)/input \
				$(FCEU_API_DIR)/mbshare \
				$(FCEU_API_DIR)/drivers/common \
				$(FCEU_API_DIR)/drivers/vieolog \
				$(UTIL_DIR)/unzip \
				$(UTIL_DIR)/zlib \
				$(UTIL_DIR)/sz \
				$(SRC_DIR)/ \
				$(SRC_DIR)/conf \
				$(CELL_FRAMEWORK_DIR)/graphics \
				$(CELL_FRAMEWORK_DIR)/gui \
				$(CELL_FRAMEWORK_DIR)/input  \
				$(CELL_FRAMEWORK_DIR)/audio \
				$(CELL_FRAMEWORK_DIR)/threads \
				$(CELL_FRAMEWORK_DIR)/logger \
				$(CELL_FRAMEWORK_DIR)/fileio \
				$(CELL_FRAMEWORK_DIR)/network \
				$(CELL_FRAMEWORK_DIR)/utility

PPU_SRCS		=	$(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp)) $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c)) \
				$(FCEU_API_DIR)/asm.cpp \
				$(FCEU_API_DIR)/cart.cpp \
				$(FCEU_API_DIR)/cheat.cpp \
				$(FCEU_API_DIR)/conddebug.cpp \
				$(FCEU_API_DIR)/config.cpp \
				$(FCEU_API_DIR)/debug.cpp \
				$(FCEU_API_DIR)/drawing.cpp \
				$(FCEU_API_DIR)/emufile.cpp \
				$(FCEU_API_DIR)/fceu.cpp \
				$(FCEU_API_DIR)/fds.cpp \
				$(FCEU_API_DIR)/file.cpp \
				$(FCEU_API_DIR)/filter.cpp \
				$(FCEU_API_DIR)/ines.cpp \
				$(FCEU_API_DIR)/input.cpp \
				$(FCEU_API_DIR)/movie.cpp \
				$(FCEU_API_DIR)/netplay.cpp \
				$(FCEU_API_DIR)/nsf.cpp \
				$(FCEU_API_DIR)/oldmovie.cpp \
				$(FCEU_API_DIR)/palette.cpp \
				$(FCEU_API_DIR)/ppu.cpp \
				$(FCEU_API_DIR)/sound.cpp \
				$(FCEU_API_DIR)/state.cpp \
				$(FCEU_API_DIR)/unif.cpp \
				$(FCEU_API_DIR)/video.cpp \
				$(FCEU_API_DIR)/vsuni.cpp \
				$(FCEU_API_DIR)/wave.cpp \
				$(FCEU_API_DIR)/x6502.cpp \
				$(FCEU_API_DIR)/utils/ConvertUTF.c \
				$(FCEU_API_DIR)/utils/crc32.cpp \
				$(FCEU_API_DIR)/utils/endian.cpp \
				$(FCEU_API_DIR)/utils/general.cpp \
				$(FCEU_API_DIR)/utils/guid.cpp \
				$(FCEU_API_DIR)/utils/md5.cpp \
				$(FCEU_API_DIR)/utils/memory.cpp \
				$(FCEU_API_DIR)/utils/xstring.cpp
#PPU_SRCS		+=	$(FCEU_API_DIR)/lua/src/lapi.c \
				$(FCEU_API_DIR)/lua/src/luauxlib.c \
				$(FCEU_API_DIR)/lua/src/lbaselib.c \
				$(FCEU_API_DIR)/lua/src/lcode.c \
				$(FCEU_API_DIR)/lua/src/ldblib.c \
				$(FCEU_API_DIR)/lua/src/ldebug.c \
				$(FCEU_API_DIR)/lua/src/ldo.c \
				$(FCEU_API_DIR)/lua/src/ldump.c \
				$(FCEU_API_DIR)/lua/src/lfunc.c \
				$(FCEU_API_DIR)/lua/src/lgc.c \
				$(FCEU_API_DIR)/lua/src/linit.c \
				$(FCEU_API_DIR)/lua/src/liolib.c \
				$(FCEU_API_DIR)/lua/src/llex.c \
				$(FCEU_API_DIR)/lua/src/lmathlib.c \
				$(FCEU_API_DIR)/lua/src/lmem.c \
				$(FCEU_API_DIR)/lua/src/loadlib.c \
				$(FCEU_API_DIR)/lua/src/lobject.c \
				$(FCEU_API_DIR)/lua/src/lopcodes.c \
				$(FCEU_API_DIR)/lua/src/loslib.c \
				$(FCEU_API_DIR)/lua/src/lparser.c \
				$(FCEU_API_DIR)/lua/src/lstate.c \
				$(FCEU_API_DIR)/lua/src/lstring.c \
				$(FCEU_API_DIR)/lua/src/lstrlib.c \
				$(FCEU_API_DIR)/lua/src/ltable.c \
				$(FCEU_API_DIR)/lua/src/ltablib.c \
				$(FCEU_API_DIR)/lua/src/ltm.c \
				$(FCEU_API_DIR)/lua/src/lundump.c \
				$(FCEU_API_DIR)/lua/src/lvm.c \
				$(FCEU_API_DIR)/lua/src/lzio.c \
				$(FCEU_API_DIR)/lua/src/print.c \
				$(FCEU_API_DIR)/lua-engine.cpp
PPU_TARGET		=	fceu.ppu.elf

PPU_CXXFLAGS		+=	-I. -I$(FCEU_API_DIR) -I$(UTIL_DIR)/zlib -I$(UTIL_DIR)/unzip -I$(UTIL_DIR)/sz -DPSS_STYLE=1 -DGEKKO -DPS3_SDK_3_41 \
				-DPSGL -DPATH_MAX=1024 
PPU_CFLAGS		+=	-I. -I$(FCEU_API_DIR) -I$(UTIL_DIR)/zlib -I$(UTIL_DIR)/unzip -I$(UTIL_DIR)/sz -DPSS_STYLE=1 -DGEKKO -DPS3_SDK_3_41 \
				-DPSGL -DPATH_MAX=1024

# use 7z less mem intensive
PPU_CXXFLAGS		+=	-D_SZ_ONE_DIRECTORY -D_LZMA_IN_CB -D_LZMA_OUT_READ
PPU_CFLAGS		+=	-D_SZ_ONE_DIRECTORY -D_LZMA_IN_CB -D_LZMA_OUT_READ

# enable FCEU frameskip
PPU_CXXFLAGS		+=	-DFRAMESKIP 
PPU_CFLAGS		+=	-DFRAMESKIP

# enable LUA
#PPU_CXXFLAGS		+=	-D_S9XLUA_H -I$(FCEU_API_DIR)/lua/src 
#PPU_CFLAGS		+=	-D_S9XLUA_H -I$(FCEU_API_DIR)/lua/src

# enable FCEU debugging
# enable telnet control console
#PPU_CSTDFLAGS		+=	-D__PPU__
#PPU_CSTDFLAGS		+=	-DCONSOLE_USE_NETWORK
#PPU_CXXSTDFLAGS	+=	-D__PPU__
#PPU_CXXSTDFLAGS	+=	-DCONSOLE_USE_NETWORK
#PPU_CXXFLAGS		+=	-DEMUDEBUG -DEMU_DBG_DELAY=500000 
#PPU_CFLAGS		+=	-DEMUDEBUG -DEMU_DBG_DELAY=500000

# netlogger debugging
PPU_CXXFLAGS 		+=	-DCELL_DEBUG -DPS3_DEBUG_IP=\"192.168.1.7\" -DPS3_DEBUG_PORT=9002
PPU_CFLAGS		+=	-DCELL_DEBUG -DPS3_DEBUG_IP=\"192.168.1.7\" -DPS3_DEBUG_PORT=9002

ifeq ($(CELL_BUILD_TOOLS),SNC)
PPU_CFLAGS		+= -Xbranchless=1 -Xfastmath=1 -Xassumecorrectsign=1 -Xassumecorrectalignment=1 \
			-Xunroll=1 -Xautovecreg=1 -DSNC_COMPILER 
PPU_CXXFLAGS		+= -Xbranchless=1 -Xfastmath=1 -Xassumecorrectsign=1 -Xassumecorrectalignment=1 \
			-Xunroll=1 -Xautovecreg=1 -DSNC_COMPILER -Xc=cp+exceptions+rtti+wchar_t+bool+array_nd+tmplname
else
PPU_CFLAGS		+= -funroll-loops -DGCC_COMPILER
PPU_CXXFLAGS		+= -funroll-loops -DGCC_COMPILER
PPU_LDFLAGS		+= -finline-limit=5000
PPU_LDFLAGS		+= -Wl
endif

PPU_LDLIBS		+= 	-L. -L$(CELL_SDK)/target/ppu/lib/PSGL/RSX/opt -ldbgfont -lPSGL \
				-lPSGLcgc -lcgc -lgcm_cmd -lgcm_sys_stub -lresc_stub \
				-lm -lio_stub -lfs_stub -lsysutil_stub -lcontrol_console_ppu -lsysmodule_stub \
				-lnet_stub -lnetctl_stub -laudio_stub -lpthread

ifeq ($(CONSOLE_USE_NETWORK),1)
PPU_CSTDFLAGS +=	-DCONSOLE_USE_NETWORK
PPU_CXXSTDFLAGS +=	-DCONSOLE_USE_NETWORK
PPU_LDLIBS+=	-lnet_stub -lnetctl_stub
endif


include $(CELL_MK_DIR)/sdk.target.mk

.PHONY: pkg
pkg: $(PPU_TARGET)
	$(STRIP) $(PPU_TARGET) 
	$(MKFSELF_NPDRM) $(PPU_TARGET) pkg/USRDIR/EBOOT.BIN
	$(COPY) -r ./src/cellframework/extra/shaders pkg/USRDIR/
	$(MKPKG_NPDRM) pkg/package.conf pkg
