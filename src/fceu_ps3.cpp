#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timer.h>
#include <sys/return_code.h>
#include <sys/process.h>
#include <cell/sysmodule.h>
#include <cell/cell_fs.h>
#include <cell/pad.h>
#include <stddef.h>
#include <math.h>
#include <sysutil/sysutil_sysparam.h>
#include <sys/spu_initialize.h>

#include "fceu_ps3.h"

#include "fceu/emufile.h"
#include "fceu/types.h"
#include "fceu/fceu.h"
#include "fceu/file.h"
#include "fceu/driver.h"
#include "fceu/fds.h"

#include "cellframework/logger/Logger.h"
#include "cellframework/input/cellInput.h"
#include "cellframework/utility/OSKUtil.h"
#include "cellframework/audio/audioport.hpp"

#include "cellControlConsole.h"
#include "emulator_implementation.h"
#include "FceuGraphics.h"
#include "emulator_fileio.h"
#include "menu.h"



SYS_PROCESS_PARAM(1001, 0x10000);

FceuGraphics* Graphics = NULL;
CellInputFacade* CellInput = NULL;
Audio::Stream<int16_t>* CellAudio = NULL;
OSKUtil* oskutil = NULL;

//define struct
struct SSettings Settings;

// mode the main loop is in
Emulator_Modes mode_switch = MODE_MENU;

// is a ROM running
bool emulation_running;

bool lua_loaded = false;

int frameSkipAmt = 18;
int frameSkipCounter = 0; //Counter for managing frame skip

bool turbo_enabled = false;

int eoptions = 0;

int fskip = 0;

// is fceu loaded
bool fceu_loaded = false;

// needs settings loaded
bool load_settings = true;

// current rom loaded
bool rom_loaded = false;

// current rom being emulated
string current_rom;


bool Emulator_Initialized()
{
	return fceu_loaded;
}

bool Emulator_ToggleTurbo()
{
	turbo_enabled = !turbo_enabled;
}

void Emulator_TurboDecrement()
{
	if(frameSkipAmt != 0)
	{
		frameSkipAmt -= 1;
	}
}

void Emulator_TurboIncrement()
{
	frameSkipAmt += 1;
}

int Emulator_GetTurboSpeed()
{
	return frameSkipAmt;
}


bool Emulator_IsROMLoaded()
{
	return rom_loaded;
}


bool Emulator_ROMRunning()
{
	return emulation_running;
}


void Emulator_SwitchMode(Emulator_Modes m)
{
	mode_switch = m;
}

void Emulator_OSKStart(const wchar_t* msg, const wchar_t* init)
{
	oskutil->Start(msg,init);
}

const char * Emulator_OSKOutputString()
{
	return oskutil->OutputString();
}

void Emulator_Shutdown()
{
	// do any clean up... save stuff etc
	// ...
	if (rom_loaded)
	{
		if (iNESCart.battery)
			FCEU_SaveGameSave(&iNESCart);

		if (UNIFCart.battery)
			FCEU_SaveGameSave(&UNIFCart);
	}

	//add saving back of conf file
	Emulator_Implementation_SaveSettings();

#ifdef PS3_PROFILING
	// shutdown everything
	Graphics->DeinitDbgFont();
	Graphics->Deinit();

	ControlConsole_DeInit();

	if (CellInput)
		delete CellInput;

	if (CellAudio)
		delete CellAudio;

	if (Graphics)
		delete Graphics;

	if (oskutil)
		delete oskutil;

	LOG_CLOSE();

	cellSysmoduleUnloadModule(CELL_SYSMODULE_FS);
	cellSysmoduleUnloadModule(CELL_SYSMODULE_IO);
	cellSysutilUnregisterCallback(0);

	// force exit
	exit(0);
#else
	sys_process_exit(0);
#endif
}


int Emulator_CloseGame()
{
    if(!rom_loaded) {
        return(0);
    }
//FIXME: Lua - Implement this
/*
#ifdef _S9XLUA_H
if(lua_loaded)
{
	FCEU_LuaStop(); // kill lua script before the gui dies
}
#endif
*/
    FCEUI_CloseGame();
    GameInfo = 0;
    return(1);
}


void Emulator_RequestLoadROM(string rom, bool forceReload)
{
	if (!rom_loaded || forceReload || current_rom.empty() || current_rom.compare(rom) != 0)
	{
		current_rom = rom;

		Emulator_CloseGame();

		size_t fileSize = LoadFile((char*)nesrom, current_rom.c_str(), 0, true);
		if (fileSize <= 0)
		{
			FCEU_PrintError("1: ERROR LOADING FILE!\n");
			Emulator_Shutdown();
		}

		if (!LoadRom(current_rom.c_str(), fileSize))
		{
			FCEU_PrintError("2: ERROR LOADING ROM!\n");
			Emulator_Shutdown();
		}

		// load battery ram
		if (iNESCart.battery)
		{
			FCEU_LoadGameSave(&iNESCart);
		}
		else if (UNIFCart.battery)
		{
			FCEU_LoadGameSave(&UNIFCart);
		}

		rom_loaded = true;
		FCEUD_Message("Successfully loaded ROM!");
	}
}


void Emulator_StopROMRunning()
{
	emulation_running = false;

	// load battery ram
	if (iNESCart.battery)
	{
		FCEU_SaveGameSave(&iNESCart);
	}
	else if (UNIFCart.battery)
	{
		FCEU_SaveGameSave(&UNIFCart);
	}

}

void Emulator_StartROMRunning()
{
	Emulator_SwitchMode(MODE_EMULATION);
}


void Emulator_FceuInit()
{
    if (!FCEUI_Initialize())
    {
            Emulator_Shutdown();
    }

    Emulator_FceuSetPaths();
    fceu_loaded = true;
}

void Emulator_FceuSetPaths()
{
    LOG_DBG("Settings.PS3PathSaveStates: %s\n", Settings.PS3PathSaveStates.c_str());
    LOG_DBG("SRAM dir: %s\n", Settings.PS3PathSRAM.c_str());
    LOG_DBG("Cheats dir: %s\n", Settings.PS3PathCheats.c_str());
    FCEUI_SetBaseDirectory(USRDIR);
    FCEUI_SetDirOverride(FCEUIOD_STATES, (char *)Settings.PS3PathSaveStates.c_str());
    FCEUI_SetDirOverride(FCEUIOD_NV, (char *)Settings.PS3PathSRAM.c_str());
    FCEUI_SetDirOverride(FCEUIOD_CHEATS, (char *)Settings.PS3PathCheats.c_str());
}

void Emulator_FceuGraphicsInit()
{
	LOG_DBG("SetDimensions(240, 256 * 4)\n");
	Graphics->SetDimensions(240, 256 * 4);

	Rect r;
	r.x = 0;
	r.y = 0;
	r.w = 256;
	r.h = 240;
	Graphics->SetRect(r);
}

void Emulator_GraphicsInit()
{
	LOG_DBG("Emulator_GraphicsInit()\n");

	if (Graphics == NULL)
	{
		Graphics = new FceuGraphics();
	}

	Graphics->Init();
	Emulator_FceuGraphicsInit();

	LOG_DBG("Emulator_GraphicsInit->InitDebug Font\n");
	Graphics->InitDbgFont();
}


static uint32 JSReturn = 0;
void *InputDPR;
static unsigned char pad[4];


void Input_SetFceuInput()
{
	InputDPR = &JSReturn;

    // Default ports back to gamepad - init 4 players always for now FIXME
    FCEUI_SetInput(0, SI_GAMEPAD, InputDPR, 0);
    FCEUI_SetInput(1, SI_GAMEPAD, InputDPR, 0);
    FCEUI_SetInput(2, SI_GAMEPAD, InputDPR, 0);
    FCEUI_SetInput(3, SI_GAMEPAD, InputDPR, 0);
}

int special_button_mappings(int controllerno, int specialbuttonmap)
{

	if((specialbuttonmap != BTN_EXITTOMENU) &&
	(specialbuttonmap != BTN_QUICKSAVE) &&
	(specialbuttonmap != BTN_QUICKLOAD) &&
	(specialbuttonmap != BTN_CHEATENABLE) &&
	(specialbuttonmap != BTN_INCREMENTCHEAT) &&
	(specialbuttonmap != BTN_DECREMENTCHEAT) &&
	(specialbuttonmap != BTN_INCREMENTSAVE) &&
	(specialbuttonmap != BTN_DECREMENTSAVE) &&
	(specialbuttonmap != BTN_FASTFORWARD) &&
	(specialbuttonmap != BTN_INCREMENTTURBO) &&
	(specialbuttonmap != BTN_DECREMENTTURBO) &&
	(specialbuttonmap != BTN_NONE))
	{
		pad[controllerno] |= specialbuttonmap;
	}
	else
	{
		char msg[128];
		switch(specialbuttonmap)
		{
			case BTN_CHEATENABLE:
				if (FCEUI_ToggleCheat(Emulator_Implementation_CurrentCheatPosition()))
				{
					sprintf(msg,"Activated cheat: %d", Emulator_Implementation_CurrentCheatPosition());
				}
				else
				{
					sprintf(msg, "Disabled cheat: %d", Emulator_Implementation_CurrentCheatPosition());
				}
				FCEUD_Message(msg);
				break;
			case BTN_INCREMENTCHEAT:
				Emulator_Implementation_IncrementCurrentCheatPosition();
				sprintf(msg, "Cheat pos. changed to: %d (%s)", Emulator_Implementation_CurrentCheatPosition(), FCEUI_GetCheatLabel(Emulator_Implementation_CurrentCheatPosition()));
				FCEUD_Message(msg);
				break;
			case BTN_DECREMENTCHEAT:
				Emulator_Implementation_DecrementCurrentCheatPosition();
				sprintf(msg, "Cheat pos. changed to: %d (%s)", Emulator_Implementation_CurrentCheatPosition(), FCEUI_GetCheatLabel(Emulator_Implementation_CurrentCheatPosition()));
				FCEUD_Message(msg);
				break;
			case BTN_EXITTOMENU:
				Emulator_StopROMRunning();
				Emulator_SwitchMode(MODE_MENU);
				break;
			case BTN_DECREMENTSAVE:
				Emulator_DecrementCurrentSaveStateSlot();
				break;
			case BTN_INCREMENTSAVE:
				Emulator_IncrementCurrentSaveStateSlot();
				break;
			case BTN_QUICKSAVE:
				FCEUI_SaveState(NULL);
				break;
			case BTN_QUICKLOAD:
				FCEUI_LoadState(NULL);
				break;
			case BTN_FASTFORWARD:
				Emulator_ToggleTurbo();
				sprintf(msg, turbo_enabled ? "Fast-forwarding enabled" : "Fast-forwarding disabled");
				FCEUD_Message(msg);
				break;
			case BTN_INCREMENTTURBO:
				Emulator_TurboIncrement();
				sprintf(msg, "Fast-forwarding speed set to: %d", Emulator_GetTurboSpeed());
				FCEUD_Message(msg);
				break;
			case BTN_DECREMENTTURBO:
				Emulator_TurboDecrement();
				sprintf(msg, "Fast-forwarding speed set to: %d", Emulator_GetTurboSpeed());
				FCEUD_Message(msg);
				break;
			default:
				break;
		}
	}
}

void UpdateInput()
{
	JSReturn = 0;
	memset(pad, 0, sizeof(char) * 4);
	uint8_t pads_connected = CellInput->NumberPadsConnected();

	for (uint8_t i = 0; i < pads_connected; ++i) {
		CellInput->UpdateDevice(i);

    		if (CellInput->IsButtonPressed(i, CTRL_UP))
		{
			special_button_mappings(i,Settings.DPad_Up);
		}
		else if (CellInput->IsButtonPressed(i,CTRL_DOWN))
		{
			special_button_mappings(i,Settings.DPad_Down);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_LEFT))
		{
			special_button_mappings(i,Settings.DPad_Left);
		}
    		else if (CellInput->IsButtonPressed(i,CTRL_RIGHT))
		{
			special_button_mappings(i,Settings.DPad_Right);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_SQUARE))
		{
			special_button_mappings(i,Settings.ButtonSquare);
		}
		if (CellInput->IsButtonPressed(i,CTRL_CROSS))
		{
			special_button_mappings(i,Settings.ButtonCross);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_CIRCLE))
		{
			special_button_mappings(i,Settings.ButtonCircle);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_TRIANGLE))
		{
			special_button_mappings(i,Settings.ButtonTriangle);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_START))
		{
			special_button_mappings(i,Settings.ButtonStart);
		}
    		if (CellInput->IsButtonPressed(i,CTRL_SELECT))
		{
			special_button_mappings(i,Settings.ButtonSelect);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L1))
		{
			special_button_mappings(i,Settings.ButtonL1);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2))
		{
			special_button_mappings(i,Settings.ButtonL2);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L3))
		{
			special_button_mappings(i,Settings.ButtonL3);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R1))
		{
			special_button_mappings(i,Settings.ButtonR1);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2))
		{
			special_button_mappings(i,Settings.ButtonR2);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R3))
		{
			special_button_mappings(i,Settings.ButtonR3);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_L3) && CellInput->IsButtonPressed(i,CTRL_R3)))
		{
			special_button_mappings(i,Settings.ButtonR3_ButtonL3);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasButtonPressed(i,CTRL_R3)))    
		{
			special_button_mappings(i,Settings.ButtonR2_ButtonR3);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasButtonPressed(i,CTRL_R2)))
		{
			special_button_mappings(i,Settings.ButtonL2_ButtonR2);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasButtonPressed(i,CTRL_R3)))
		{
			special_button_mappings(i,Settings.ButtonL2_ButtonR3);
		}
		if ((CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasButtonPressed(i,CTRL_L3)))
		{
			special_button_mappings(i,Settings.ButtonL2_ButtonL3);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasAnalogPressedRight(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonL2_AnalogR_Right);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasAnalogPressedLeft(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonL2_AnalogR_Left);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasAnalogPressedUp(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonL2_AnalogR_Up);
		}
		if (CellInput->IsButtonPressed(i,CTRL_L2) && CellInput->WasAnalogPressedDown(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonL2_AnalogR_Down);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasAnalogPressedRight(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonR2_AnalogR_Right);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasAnalogPressedLeft(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonR2_AnalogR_Left);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasAnalogPressedUp(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonR2_AnalogR_Up);
		}
		if (CellInput->IsButtonPressed(i,CTRL_R2) && CellInput->WasAnalogPressedDown(i,CTRL_RSTICK))
		{
			special_button_mappings(i,Settings.ButtonR2_AnalogR_Down);
		}
		if (Settings.AnalogR_Down_Type ? CellInput->IsAnalogPressedDown(i,CTRL_RSTICK) && !(CellInput->IsButtonPressed(i,CTRL_L2)) : CellInput->WasAnalogPressedDown(i,CTRL_RSTICK) && !(CellInput->IsButtonPressed(i,CTRL_L2)))
		{
			special_button_mappings(i,Settings.AnalogR_Down);
		}
		if (Settings.AnalogR_Up_Type ? CellInput->IsAnalogPressedUp(i,CTRL_RSTICK) && !(CellInput->IsButtonPressed(i,CTRL_L2)) : CellInput->WasAnalogPressedUp(i,CTRL_RSTICK) && !(CellInput->IsButtonPressed(i,CTRL_L2)))
		{
			special_button_mappings(i,Settings.AnalogR_Up);
		}
		if (Settings.AnalogR_Left_Type ? CellInput->IsAnalogPressedLeft(i,CTRL_RSTICK) && !(CellInput->IsButtonPressed(i,CTRL_L2)) : CellInput->WasAnalogPressedLeft(i,CTRL_RSTICK) && !(CellInput->IsButtonPressed(i,CTRL_L2)))
		{
			special_button_mappings(i,Settings.AnalogR_Left);
		}
		if (Settings.AnalogR_Right_Type ? CellInput->IsAnalogPressedRight(i,CTRL_RSTICK) && !(CellInput->IsButtonPressed(i,CTRL_L2)) : CellInput->WasAnalogPressedRight(i,CTRL_RSTICK) && !(CellInput->IsButtonPressed(i,CTRL_L2)))
		{
			special_button_mappings(i,Settings.AnalogR_Right);
		}

		if (Settings.AnalogL_Down_Type ? CellInput->IsAnalogPressedDown(i,CTRL_LSTICK) : CellInput->WasAnalogPressedDown(i,CTRL_LSTICK))
		{
			special_button_mappings(i,Settings.AnalogL_Down);
		}
		if (Settings.AnalogL_Up_Type ? CellInput->IsAnalogPressedUp(i,CTRL_LSTICK) : CellInput->WasAnalogPressedUp(i,CTRL_LSTICK))
		{
			special_button_mappings(i,Settings.AnalogL_Up);
		}
		if (Settings.AnalogL_Left_Type ? CellInput->IsAnalogPressedLeft(i,CTRL_LSTICK) : CellInput->WasAnalogPressedLeft(i,CTRL_LSTICK))
		{
			special_button_mappings(i,Settings.AnalogL_Left);
		}
		if (Settings.AnalogL_Right_Type ? CellInput->IsAnalogPressedRight(i,CTRL_LSTICK) : CellInput->WasAnalogPressedRight(i,CTRL_LSTICK))
		{
			special_button_mappings(i,Settings.AnalogL_Right);
		}
	}
	JSReturn = pad[0] | pad[1] << 8 | pad[2] << 16 | pad[3] << 24;

}


// FCEU quirk. Copy left
static void Emulator_Convert_Samples(float * __restrict__ out, unsigned, const int16_t * __restrict__ in, size_t frames)
{
   // Dupe right channel over to left.
   for (size_t i = 0; i < frames; i++)
   {
      out[2 * i] = (float)in[2 * i + 1] / 0x7FFF;
      out[2 * i + 1] = (float)in[2 * i + 1] / 0x7FFF;
   }
}


void Emulator_EnableSound()
{
	if(CellAudio)
	{
		delete CellAudio;
	}

	CellAudio = new Audio::AudioPort<int16_t>(2, AUDIO_INPUT_RATE);
	CellAudio->set_float_conv_func(Emulator_Convert_Samples);
}


// main interface to FCE Ultra
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int32 Count)
{
        if(XBuf)
        {
        	Graphics->Draw(XBuf, 256, 240);
        }

        if(Buffer && Count > 0)
        {
        	CellAudio->write((int16_t*)Buffer, Count * 2);
        }

	extern bool JustFrameAdvanced;
	bool throttle = true;
	//FIXME: Remove this?
	/*
	if( (eoptions&EO_NOTHROTTLE) )
	{
	}
	*/

	/*
	if(throttle) //if throttling is enabled...
		if(!turboenabled) //and turbo is disabled
			if(!FCEUI_EmulationPaused()
				||JustFrameAdvanced
				)
				//then throttle
				while(SpeedThrottle()) {
					FCEUD_UpdateInput();
					}
	*/

	if(!JustFrameAdvanced && FCEUI_EmulationPaused())
	{
		sys_timer_usleep(50);
	}

	//FIXME: Implement this?
	//FCEUD_UpdateInput();
}

void Emulator_ToggleSound()
{
	if(CellAudio)
	{
		delete CellAudio;
	}
	if((Settings.RSoundEnabled) && (strlen(Settings.RSoundServerIPAddress) > 0))
	{
		CellAudio = new Audio::RSound<int16_t>(Settings.RSoundServerIPAddress, 2, AUDIO_INPUT_RATE);
		CellAudio->set_float_conv_func(Emulator_Convert_Samples);
		// If we couldn't connect, fall back to normal audio...
		if (!CellAudio->alive())
		{
			delete CellAudio;
			CellAudio = new Audio::AudioPort<int16_t>(2, AUDIO_INPUT_RATE);
			CellAudio->set_float_conv_func(Emulator_Convert_Samples);
			Settings.RSoundEnabled = false;
			Graphics->Clear();
			cellDbgFontPuts(0.2f, 0.4f, 1.0f, 0xffffffff, "Couldn't connect to RSound server.\nFalling back to regular audio.");
			Graphics->FlushDbgFont();
			Graphics->Swap();
			sys_timer_usleep(3000000);
		}
	}
	else
	{
		CellAudio = new Audio::AudioPort<int16_t>(2, AUDIO_INPUT_RATE);
		CellAudio->set_float_conv_func(Emulator_Convert_Samples);
	}
}

void EmulationLoop()
{
	uint8 *gfx=0;
	int32 *sound=0;
	int32 ssize=0;

	if (!fceu_loaded)
	{
		Emulator_FceuInit();
	}

	if (!rom_loaded)
	{
		FCEU_PrintError("No Rom Loaded!");
		Emulator_SwitchMode(MODE_MENU);
		return;
	}

	// init cell audio
	Emulator_ToggleSound();

	// set the shader cg params
	Graphics->UpdateCgParams(256, 240, 256, 240);

	// set fceu input
	Input_SetFceuInput();

	FCEU_ResetPalette();

	// FIXME: verify this is the correct place to do this
	memset(FDSBIOS, 0, sizeof(FDSBIOS)); // clear FDS BIOS memory


	LOG_DBG("Videomode: %s\n", GameInfo->vidsys == 1 ? "PAL" : "NTSC");

	emulation_running = true;
	
	CellAudio->unpause();
	
	while (emulation_running)
	{
		UpdateInput();

		if (turbo_enabled)
		{	
			if(!frameSkipCounter)
			{
				frameSkipCounter = frameSkipAmt;
				fskip = 0;
			}
			else
			{
				frameSkipCounter--;
				/*
				if (muteTurbo) fskip = 2;
				else
				*/
				fskip = 1;
			}
		}
		else
		{
			fskip = 0;
		}

        	FCEUI_Emulate(&gfx, &sound, &ssize, fskip);
        	FCEUD_Update(gfx, sound, ssize);

#ifdef EMUDEBUG
		if (CellConsole_IsInitialized())
		{
			cellConsolePoll();
		}
#endif
		cellSysutilCheckCallback();
	}
}


void sysutil_exit_callback (uint64_t status, uint64_t param, void *userdata) {
	(void) param;
	(void) userdata;

	switch (status) {
		case CELL_SYSUTIL_REQUEST_EXITGAME:
			MenuStop();
			Emulator_StopROMRunning();
			mode_switch = MODE_EXIT;
			break;
		case CELL_SYSUTIL_DRAWING_BEGIN:
		case CELL_SYSUTIL_DRAWING_END:
			break;
		case CELL_SYSUTIL_OSKDIALOG_LOADED:
			break;
		case CELL_SYSUTIL_OSKDIALOG_FINISHED:
			oskutil->Stop();
			break;
		case CELL_SYSUTIL_OSKDIALOG_UNLOADED:
			oskutil->Close();
			break;
	}
}


int main (void)
{
	// Initialize 6 SPUs but reserve 1 SPU as a raw SPU for PSGL
	sys_spu_initialize(6, 1);

	cellSysmoduleLoadModule(CELL_SYSMODULE_FS);
	cellSysmoduleLoadModule(CELL_SYSMODULE_IO);

	cellSysutilRegisterCallback(0, sysutil_exit_callback, NULL);

#ifdef EMUDEBUG
	ControlConsole_Init();
#endif

	LOG_INIT();
	LOG_DBG("LOGGER LOADED!\n");

	Graphics = new FceuGraphics();
	Emulator_GraphicsInit();

	CellInput = new CellInputFacade();
	CellInput->Init();

	oskutil = new OSKUtil();

	//needs to come first before graphics
	if(Emulator_Implementation_InitSettings())
	{
		load_settings = false;
	}

	Emulator_FceuInit();

    // allocate memory to store rom
    nesrom = (unsigned char *)memalign(32,1024*1024*4); // 4 MB should be plenty

	while(1)
	{
		switch(mode_switch)
		{
			case MODE_MENU:
				MenuMainLoop();
				break;
			case MODE_EMULATION:
				EmulationLoop();
				CellAudio->pause();
				break;
			case MODE_EXIT:
				Emulator_Shutdown();
				return 0;
		}
	}

	return 0;
}
