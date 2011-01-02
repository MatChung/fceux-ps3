/*
 * fceuSupport.h
 *
 *  Created on: Oct 12, 2010
 *      Author: halsafar
 */

#ifndef FCEUSUPPORT_H_
#define FCEUSUPPORT_H_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fceu/driver.h"
#include "fceu/fceu.h"
#include "fceu/input.h"
#include "fceu/types.h"
#include "fceu/state.h"
#include "fceu/ppu.h"
#include "fceu/cart.h"
#include "fceu/x6502.h"
#include "fceu/git.h"
#include "fceu/palette.h"
#include "fceu/sound.h"
#include "fceu/file.h"
#include "fceu/cheat.h"
#include "fceu/ines.h"
#include "fceu/unif.h"

#include "conf/conffile.h"

#define EMULATOR_VERSION "1.4"

// max fs path
#define MAX_PATH 1024

// color palettes
#define MAXPAL 13

/*struct st_palettes {
    char name[32], desc[32];
    unsigned int data[64];
};

extern struct st_palettes palettes[];*/

enum
{
	MAP_BUTTONS_OPTION_SETTER,
	MAP_BUTTONS_OPTION_GETTER,
	MAP_BUTTONS_OPTION_DEFAULT
};


// NES ROM
extern unsigned char *nesrom;

// BIOS, GENIE, GAMEINFO (used to query game info...)
extern uint8 FDSBIOS[8192];
extern uint8 *GENIEROM;
extern FCEUGI *GameInfo;

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int32 Count);

void RebuildSubCheats(void);
int AddCheatEntry(char *name, uint32 addr, uint8 val, int compare, int status, int type);

extern int FDSLoad(const char *name, FCEUFILE *fp);
extern int iNESLoad(const char *name, FCEUFILE *fp, int o);
extern int UNIFLoad(const char *name, FCEUFILE *fp);
extern int NSFLoad(const char *name, FCEUFILE *fp);
extern void PowerNES(void);
extern INPUTC *FCEU_InitZapper(int w);
extern void FCEU_ResetPalette(void);
extern int CloseGame();


extern u32 iNESGameCRC32;
extern CartInfo iNESCart;
extern CartInfo UNIFCart;

enum ControlStyle
{
	CONTROL_STYLE_ORIGINAL,
	CONTROL_STYLE_BETTER
};



extern ControlStyle control_style;

void Input_SetFceuInput();


int Emulator_CurrentSaveStateSlot();
void Emulator_IncrementCurrentSaveStateSlot();
void Emulator_DecrementCurrentSaveStateSlot();
int Emulator_Implementation_CurrentCheatPosition();
void Emulator_Implementation_IncrementCurrentCheatPosition();
void Emulator_Implementation_DecrementCurrentCheatPosition();
bool Emulator_Implementation_InitSettings();
bool Emulator_Implementation_SaveSettings();
void Emulator_Implementation_ButtonMappingSettings(bool map_button_option_enum);


#define AUDIO_INPUT_RATE (48000)
#define AUDIO_BUFFER_SAMPLES (4096)


#endif /* FCEUSUPPORT_H_ */
