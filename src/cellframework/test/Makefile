
### 
##
# Generic makefile that avoids Sony makefile. (ugh)
###
##

## Platform detection. Only two applicable hosts for PS3 SDK. (Well, actually one :P But you can set up wrapper scripts in Linux.
ifeq ($(platform),)
  uname := $(shell uname -a)
  ifeq ($(uname),)
    platform := win
  else
    platform := linux
  endif
endif

CELL_SDK := /usr/local/cell
ifeq ($(platform),win)
   HOST_DIR := host-win32
else
   HOST_DIR := host-linux
endif

CC = $(CELL_SDK)/$(HOST_DIR)/ppu/bin/ppu-lv2-gcc
CXX = $(CELL_SDK)/$(HOST_DIR)/ppu/bin/ppu-lv2-g++
STRIP = $(CELL_SDK)/$(HOST_DIR)/ppu/bin/ppu-lv2-strip

PPU_TARGET := test.ppu.elf

LDDIRS = -L. -L$(CELL_SDK)/target/ppu/lib/PSGL/RSX/ultra-opt
INCDIRS = -I../

MKFSELF_NPDRM = $(CELL_SDK)/$(HOST_DIR)/bin/make_fself_npdrm
MKPKG_NPDRM = $(CELL_SDK)/$(HOST_DIR)/bin/make_package_npdrm

OBJ = TestAudio.o TestGFX.o main.o
OBJ += ../threads/mutex.cpp ../threads/thread.cpp ../threads/cond.cpp ../threads/scoped_lock.cpp \
../graphics/PSGLGraphics.cpp \
../input/cellInput.cpp \
../audio/resampler.cpp ../audio/quadratic_resampler.cpp

LIBS = -ldbgfont -lPSGL -lgcm_cmd -lgcm_sys_stub -lresc_stub -lm -lio_stub -lfs_stub -lsysutil_stub -lsysmodule_stub -laudio_stub -lnet_stub -lpthread

CXXFLAGS = -Wall -O3 -DPS3_SDK_3_41 -mcpu=cell -mtune=cell

all: $(PPU_TARGET)

$(PPU_TARGET): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(CXXFLAGS) $(LDDIRS) $(LIBS)
	$(STRIP) $(PPU_TARGET)

%.o: %.c
	$(CC) $(INCDIRS) $(CFLAGS) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(INCDIRS) $(CXXFLAGS) -c -o $@ $<

pkg: $(PPU_TARGET)
	@mkdir -p pkg/SAVEDATA
	@mkdir -p pkg/USRDIR
	$(MKFSELF_NPDRM) $(PPU_TARGET) pkg/USRDIR/EBOOT.BIN
	$(MKPKG_NPDRM) pkg/package.conf pkg

clean:
	rm -f *.o 
	rm -f *.pkg
	rm -f *.elf

.PHONY: all clean pkg
