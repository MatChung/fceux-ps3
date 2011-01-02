#ifndef FCEUPS3_H_
#define FCEUPS3_H_

#include <string>

using namespace std;

#include <cell/control_console.h>

#include "cellframework/audio/stream.hpp"
#include "cellframework/input/cellInput.h"
#include "cellframework/audio/audioport.hpp"
#include "cellframework/audio/rsound.hpp"

#include "FceuGraphics.h"
#include "emulator_input.h"

#define USRDIR "/dev_hdd0/game/FCEU90000/USRDIR/"

enum Emulator_Modes
{
	MODE_MENU,
	MODE_EMULATION,
	MODE_EXIT
};

void Emulator_RequestLoadROM(string rom, bool forceReload);
bool Emulator_IsROMLoaded();
void Emulator_SwitchMode(Emulator_Modes);
void Emulator_Shutdown();
void Emulator_StopROMRunning();
void Emulator_StartROMRunning();

void Emulator_OSKStart(const wchar_t* msg, const wchar_t* init);
const char * Emulator_OSKOutputString();

bool Emulator_Snes9xInitialized();
bool Emulator_RomRunning();
void Emulator_ToggleSound();
void Emulator_FceuSetPaths();

void Emulator_GraphicsInit();

extern int eoptions;
extern Audio::Stream<int16_t>* 	CellAudio;
extern CellInputFacade* 		CellInput;
extern FceuGraphics* 			Graphics;

#endif
