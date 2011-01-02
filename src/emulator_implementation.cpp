/*
 * fceuSupport.cpp
 *
 *  Created on: Oct 12, 2010
 *      Author: halsafar
 */
#include "fceu_ps3.h"

#include "FceuGraphics.h"

#include "cellframework/graphics/PSGLGraphics.h"
#include "cellframework/logger/Logger.h"
#include "cellframework/audio/audioport.hpp"
#include "cellframework/input/cellInput.h"

#include "emulator_implementation.h"

#define SYS_CONFIG_FILE "/dev_hdd0/game/FCEU90000/USRDIR/fceu.conf"

ConfigFile	currentconfig;

int current_cheat_position = 0;

// FCEU - handle turbo mode for fceu core but implemented externally
bool turbo = false;

// FCEU - as it says, fceu must
int closeFinishedMovie = false;

ControlStyle control_style;

int current_state_save = 0;


// FIXME: implement
#ifdef PS3_SDK_3_41
extern FILE* tmpfile()
{
	return 0;
}
#endif


int Emulator_CurrentSaveStateSlot()
{
	return current_state_save;
}


void Emulator_DecrementCurrentSaveStateSlot()
{
	current_state_save = (current_state_save-1);
	if (current_state_save < 0) current_state_save = 9;
	FCEUI_SelectState(current_state_save, true);
}


void Emulator_IncrementCurrentSaveStateSlot()
{
	current_state_save = (current_state_save+1) % 9;
	FCEUI_SelectState(current_state_save, true);
}


static bool try_load_config_file (const char *fname, ConfigFile &conf)
{
	LOG_DBG("try_load_config_file(%s)", fname);
	FILE * fp;

	fp = fopen(fname, "r");
	if (fp)
	{
		fprintf(stdout, "Reading config file %s.\n", fname);
		conf.LoadFile(new fReader(fp));
		fclose(fp);
	}

	return (false);
}

int Emulator_Implementation_CurrentCheatPosition()
{
	return current_cheat_position;
}

void Emulator_Implementation_DecrementCurrentCheatPosition()
{
	if(Emulator_Implementation_CurrentCheatPosition() > 0)
	{
		current_cheat_position--;
	}
}

void Emulator_Implementation_IncrementCurrentCheatPosition()
{
		current_cheat_position++;
}

bool Emulator_Implementation_InitSettings()
{
	LOG_DBG("Emulator_Implementation_InitSettings()");

	memset((&Settings), 0, (sizeof(Settings)));

	currentconfig.Clear();

	#ifdef SYS_CONFIG_FILE
		try_load_config_file(SYS_CONFIG_FILE, currentconfig);
	#endif

	//PS3 - General settings	
	if (currentconfig.Exists("PS3General::KeepAspect"))
	{
		Settings.PS3KeepAspect		=	currentconfig.GetBool("PS3General::KeepAspect");
	}
	else
	{
		Settings.PS3KeepAspect		=	true;
	}
	Graphics->SetAspectRatio(Settings.PS3KeepAspect ? SCREEN_4_3_ASPECT_RATIO : SCREEN_16_9_ASPECT_RATIO);

	if (currentconfig.Exists("PS3General::Smooth"))
	{
		Settings.PS3Smooth		=	currentconfig.GetBool("PS3General::Smooth");
	}
	else
	{
		Settings.PS3Smooth		=	false;
	}
	Graphics->SetSmooth(Settings.PS3Smooth);
	if (currentconfig.Exists("PS3General::OverscanEnabled"))
	{
		Settings.PS3OverscanEnabled	= currentconfig.GetBool("PS3General::OverscanEnabled");
	}
	else
	{
		Settings.PS3OverscanEnabled	= false;
	}
	if (currentconfig.Exists("PS3General::OverscanAmount"))
	{
		Settings.PS3OverscanAmount	= currentconfig.GetInt("PS3General::OverscanAmount");
	}
	else
	{
		Settings.PS3OverscanAmount	= 0;
	}
	Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);

	if (currentconfig.Exists("FCEU::Controlstyle"))
	{
		Settings.FCEUControlstyle = currentconfig.GetInt("FCEU::Controlstyle");
		control_style = (ControlStyle)(((int)Settings.FCEUControlstyle));
	}
	else
	{
		Settings.FCEUControlstyle = CONTROL_STYLE_ORIGINAL;
		control_style = CONTROL_STYLE_ORIGINAL;
	}

	if (currentconfig.Exists("FCEU::DisableSpriteLimitation"))
	{
		Settings.FCEUDisableSpriteLimitation = currentconfig.GetBool("FCEU::DisableSpriteLimitation");
		FCEUI_DisableSpriteLimitation(Settings.FCEUDisableSpriteLimitation);
	}
	else
	{
		Settings.FCEUDisableSpriteLimitation = false;
	}

	if (currentconfig.Exists("FCEU::Shader"))
	{
		Graphics->LoadFragmentShader(currentconfig.GetString("FCEU::Shader"));
	}
	else
	{
		Graphics->LoadFragmentShader(DEFAULT_SHADER_FILE);
	}
	if (currentconfig.Exists("FCEU::GameGenie"))
	{
		Settings.FCEUGameGenie = currentconfig.GetBool("FCEU::GameGenie");
		FCEUI_SetGameGenie(Settings.FCEUGameGenie);
	}
	else
	{
		Settings.FCEUGameGenie = false;
	}
	FCEUI_SetGameGenie(Settings.FCEUGameGenie);
	if (currentconfig.Exists("PS3General::PS3PALTemporalMode60Hz"))
	{
		Settings.PS3PALTemporalMode60Hz = currentconfig.GetBool("PS3General::PS3PALTemporalMode60Hz");
		Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
	}
	else
	{
		Settings.PS3PALTemporalMode60Hz = false;
		Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
	}
	//RSound Settings
	if(currentconfig.Exists("RSound::RSoundEnabled"))
	{
		Settings.RSoundEnabled		= currentconfig.GetBool("RSound::RSoundEnabled");
	}
	else
	{
		Settings.RSoundEnabled		= false;
	}
	if(currentconfig.Exists("RSound::RSoundServerIPAddress"))
	{
		Settings.RSoundServerIPAddress	= currentconfig.GetString("RSound::RSoundServerIPAddress");
	}
	else
	{
		Settings.RSoundServerIPAddress = "0.0.0.0";
	}
	// PS3 Path Settings
	if (currentconfig.Exists("PS3Paths::PathSaveStates"))
	{
		Settings.PS3PathSaveStates		= currentconfig.GetString("PS3Paths::PathSaveStates");
	}
	else
	{
		Settings.PS3PathSaveStates		= USRDIR;
	}
	
	if (currentconfig.Exists("PS3Paths::PathSRAM"))
	{
		Settings.PS3PathSRAM		= currentconfig.GetString("PS3Paths::PathSRAM");
	}
	else
	{
		Settings.PS3PathSRAM		= USRDIR;
	}

	if (currentconfig.Exists("PS3Paths::PathCheats"))
	{
		Settings.PS3PathCheats		= currentconfig.GetString("PS3Paths::PathCheats");
	}
	else
	{
		Settings.PS3PathCheats		= USRDIR;
	}
	
	if (currentconfig.Exists("PS3Paths::PathScreenshots"))
	{
		Settings.PS3PathScreenshots		= currentconfig.GetString("PS3Paths::PathScreenshots");
	}
	else
	{
		Settings.PS3PathScreenshots		= USRDIR;
	}
	
	if (currentconfig.Exists("PS3Paths::PathROMDirectory"))
	{
		Settings.PS3PathROMDirectory		= currentconfig.GetString("PS3Paths::PathROMDirectory");
	}
	else
	{
		Settings.PS3PathROMDirectory		= "/";
	}
	if (currentconfig.Exists("PS3Paths::BaseDirectory"))
	{
		Settings.PS3PathBaseDirectory		= currentconfig.GetString("PS3Paths::PathBaseDirectory");
	}
	else
	{
		Settings.PS3PathBaseDirectory		= USRDIR;
	}
	Emulator_Implementation_ButtonMappingSettings(MAP_BUTTONS_OPTION_GETTER);

	LOG_DBG("SUCCESS - Emulator_Implementation_InitSettings()");
	return true;
}

void Emulator_Implementation_ButtonMappingSettings(bool map_button_option_enum)
{
	switch(map_button_option_enum)
	{
		case MAP_BUTTONS_OPTION_SETTER:
			currentconfig.SetInt("PS3ButtonMappings::DPad_Up",Settings.DPad_Up);
			currentconfig.SetInt("PS3ButtonMappings::DPad_Down",Settings.DPad_Down);
			currentconfig.SetInt("PS3ButtonMappings::DPad_Left",Settings.DPad_Left);
			currentconfig.SetInt("PS3ButtonMappings::DPad_Right",Settings.DPad_Right);
			currentconfig.SetInt("PS3ButtonMappings::ButtonCircle",Settings.ButtonCircle);
			currentconfig.SetInt("PS3ButtonMappings::ButtonCross",Settings.ButtonCross);
			currentconfig.SetInt("PS3ButtonMappings::ButtonTriangle",Settings.ButtonTriangle);
			currentconfig.SetInt("PS3ButtonMappings::ButtonSquare",Settings.ButtonSquare);
			currentconfig.SetInt("PS3ButtonMappings::ButtonSelect",Settings.ButtonSelect);
			currentconfig.SetInt("PS3ButtonMappings::ButtonStart",Settings.ButtonStart);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL1",Settings.ButtonL1);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR1",Settings.ButtonR1);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL2",Settings.ButtonL2);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR2",Settings.ButtonR2);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL2_ButtonL3",Settings.ButtonL2_ButtonL3);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL2_ButtonR3",Settings.ButtonL2_ButtonR3);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR3",Settings.ButtonR3);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL3",Settings.ButtonL3);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL2_ButtonR2",Settings.ButtonL2_ButtonR2);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL2_AnalogR_Right",Settings.ButtonL2_AnalogR_Right);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL2_AnalogR_Left",Settings.ButtonL2_AnalogR_Left);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL2_AnalogR_Up",Settings.ButtonL2_AnalogR_Up);
			currentconfig.SetInt("PS3ButtonMappings::ButtonL2_AnalogR_Down",Settings.ButtonL2_AnalogR_Down);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR2_AnalogR_Right",Settings.ButtonR2_AnalogR_Right);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR2_AnalogR_Left",Settings.ButtonR2_AnalogR_Left);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR2_AnalogR_Up",Settings.ButtonR2_AnalogR_Up);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR2_AnalogR_Down",Settings.ButtonR2_AnalogR_Down);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR2_ButtonR3",Settings.ButtonR2_ButtonR3);
			currentconfig.SetInt("PS3ButtonMappings::ButtonR3_ButtonL3",Settings.ButtonR3_ButtonL3);
			currentconfig.SetInt("PS3ButtonMappings::AnalogR_Up",Settings.AnalogR_Up);
			currentconfig.SetInt("PS3ButtonMappings::AnalogR_Down",Settings.AnalogR_Down);
			currentconfig.SetInt("PS3ButtonMappings::AnalogR_Left",Settings.AnalogR_Left);
			currentconfig.SetInt("PS3ButtonMappings::AnalogR_Right",Settings.AnalogR_Right);

			currentconfig.SetBool("PS3ButtonMappings::AnalogR_Up_Type",Settings.AnalogR_Up_Type);
			currentconfig.SetBool("PS3ButtonMappings::AnalogR_Down_Type",Settings.AnalogR_Down_Type);
			currentconfig.SetBool("PS3ButtonMappings::AnalogR_Left_Type",Settings.AnalogR_Left_Type);
			currentconfig.SetBool("PS3ButtonMappings::AnalogR_Right_Type",Settings.AnalogR_Right_Type);
			break;
		case MAP_BUTTONS_OPTION_GETTER:
			Settings.DPad_Up		= currentconfig.GetInt("PS3ButtonMappings::DPad_Up",BTN_UP);
			Settings.DPad_Down		= currentconfig.GetInt("PS3ButtonMappings::DPad_Down",BTN_DOWN);
			Settings.DPad_Left		= currentconfig.GetInt("PS3ButtonMappings::DPad_Left",BTN_LEFT);
			Settings.DPad_Right		= currentconfig.GetInt("PS3ButtonMappings::DPad_Right",BTN_RIGHT);
			Settings.ButtonCircle		= currentconfig.GetInt("PS3ButtonMappings::ButtonCircle",BTN_A);
			Settings.ButtonCross		= currentconfig.GetInt("PS3ButtonMappings::ButtonCross",BTN_B);
			Settings.ButtonTriangle		= currentconfig.GetInt("PS3ButtonMappings::ButtonTriangle",BTN_NONE);
			Settings.ButtonSquare		= currentconfig.GetInt("PS3ButtonMappings::ButtonSquare",BTN_NONE);
			Settings.ButtonSelect		= currentconfig.GetInt("PS3ButtonMappings::ButtonSelect",BTN_SELECT);
			Settings.ButtonStart		= currentconfig.GetInt("PS3ButtonMappings::ButtonStart",BTN_START);
			Settings.ButtonL1		= currentconfig.GetInt("PS3ButtonMappings::ButtonL1",BTN_NONE);
			Settings.ButtonR1		= currentconfig.GetInt("PS3ButtonMappings::ButtonR1",BTN_NONE);
			Settings.ButtonL2		= currentconfig.GetInt("PS3ButtonMappings::ButtonL2",BTN_NONE);
			Settings.ButtonR2		= currentconfig.GetInt("PS3ButtonMappings::ButtonR2",BTN_NONE);
			Settings.ButtonL2_ButtonL3	= currentconfig.GetInt("PS3ButtonMappings::ButtonL2_ButtonL3",BTN_NONE);
			Settings.ButtonL2_ButtonR3	= currentconfig.GetInt("PS3ButtonMappings::ButtonL2_ButtonR3",BTN_QUICKLOAD);
			Settings.ButtonR3		= currentconfig.GetInt("PS3ButtonMappings::ButtonR3",BTN_NONE);
			Settings.ButtonL3		= currentconfig.GetInt("PS3ButtonMappings::ButtonL3",BTN_NONE);
			Settings.ButtonL2_ButtonR2	= currentconfig.GetInt("PS3ButtonMappings::ButtonL2_ButtonR2",BTN_NONE);
			Settings.ButtonL2_AnalogR_Right = currentconfig.GetInt("PS3ButtonMappings::ButtonL2_AnalogR_Right",BTN_INCREMENTCHEAT);
			Settings.ButtonL2_AnalogR_Left	= currentconfig.GetInt("PS3ButtonMappings::ButtonL2_AnalogR_Left",BTN_DECREMENTCHEAT);
			Settings.ButtonL2_AnalogR_Up	= currentconfig.GetInt("PS3ButtonMappings::ButtonL2_AnalogR_Up",BTN_NONE);
			Settings.ButtonL2_AnalogR_Down	= currentconfig.GetInt("PS3ButtonMappings::ButtonL2_AnalogR_Down",BTN_NONE);
			Settings.ButtonR2_AnalogR_Right	= currentconfig.GetInt("PS3ButtonMappings::ButtonR2_AnalogR_Right",BTN_NONE);
			Settings.ButtonR2_AnalogR_Left	= currentconfig.GetInt("PS3ButtonMappings::ButtonR2_AnalogR_Left",BTN_NONE);
			Settings.ButtonR2_AnalogR_Up	= currentconfig.GetInt("PS3ButtonMappings::ButtonR2_AnalogR_Up",BTN_NONE);
			Settings.ButtonR2_AnalogR_Down	= currentconfig.GetInt("PS3ButtonMappings::ButtonR2_AnalogR_Down",BTN_NONE);
			Settings.ButtonR2_ButtonR3	= currentconfig.GetInt("PS3ButtonMappings::ButtonR2_ButtonR3",BTN_QUICKSAVE);
			Settings.ButtonR3_ButtonL3	= currentconfig.GetInt("PS3ButtonMappings::ButtonR3_ButtonL3",BTN_EXITTOMENU);
			Settings.AnalogR_Up		= currentconfig.GetInt("PS3ButtonMappings::AnalogR_Up",BTN_CHEATENABLE);
			Settings.AnalogR_Down		= currentconfig.GetInt("PS3ButtonMappings::AnalogR_Down",BTN_NONE);
			Settings.AnalogR_Left		= currentconfig.GetInt("PS3ButtonMappings::AnalogR_Left",BTN_DECREMENTSAVE);
			Settings.AnalogR_Right		= currentconfig.GetInt("PS3ButtonMappings::AnalogR_Right",BTN_INCREMENTSAVE);

			Settings.AnalogR_Up_Type	= currentconfig.GetBool("PS3ButtonMappings::AnalogR_Up_Type",false);
			Settings.AnalogR_Down_Type	= currentconfig.GetBool("PS3ButtonMappings::AnalogR_Down_Type",false);
			Settings.AnalogR_Left_Type	= currentconfig.GetBool("PS3ButtonMappings::AnalogR_Left_Type",false);
			Settings.AnalogR_Right_Type	= currentconfig.GetBool("PS3ButtonMappings::AnalogR_Right_Type",false);
			break;
		case MAP_BUTTONS_OPTION_DEFAULT:
			Settings.DPad_Up			= BTN_UP;
			Settings.DPad_Down			= BTN_DOWN;
			Settings.DPad_Left			= BTN_LEFT;
			Settings.DPad_Right			= BTN_RIGHT;
			Settings.ButtonCircle			= BTN_A;
			Settings.ButtonCross			= BTN_B;
			Settings.ButtonTriangle			= BTN_NONE;
			Settings.ButtonSquare			= BTN_NONE;
			Settings.ButtonSelect			= BTN_SELECT;
			Settings.ButtonStart			= BTN_START;
			Settings.ButtonL1			= BTN_NONE;
			Settings.ButtonR1			= BTN_NONE;
			Settings.ButtonL2			= BTN_NONE;
			Settings.ButtonR2			= BTN_NONE;
			Settings.ButtonL2_ButtonL3		= BTN_NONE;
			Settings.ButtonL2_ButtonR3		= BTN_QUICKLOAD;	
			Settings.ButtonR3			= BTN_NONE;
			Settings.ButtonL3			= BTN_NONE;
			Settings.ButtonL2_ButtonR2		= BTN_NONE;
			Settings.ButtonL2_AnalogR_Right		= BTN_INCREMENTCHEAT;
			Settings.ButtonL2_AnalogR_Left		= BTN_DECREMENTCHEAT;
			Settings.ButtonL2_AnalogR_Up		= BTN_NONE;
			Settings.ButtonL2_AnalogR_Down		= BTN_NONE;
			Settings.ButtonR2_AnalogR_Right		= BTN_NONE;
			Settings.ButtonR2_AnalogR_Left		= BTN_NONE;
			Settings.ButtonR2_AnalogR_Up		= BTN_NONE;
			Settings.ButtonR2_AnalogR_Down		= BTN_NONE;
			Settings.ButtonR2_ButtonR3		= BTN_QUICKSAVE;
			Settings.ButtonR3_ButtonL3		= BTN_EXITTOMENU;
			Settings.AnalogR_Up			= BTN_CHEATENABLE;
			Settings.AnalogR_Down			= BTN_NONE;
			Settings.AnalogR_Left			= BTN_DECREMENTSAVE;
			Settings.AnalogR_Right			= BTN_INCREMENTSAVE;
			Settings.AnalogR_Up_Type		= false;
			Settings.AnalogR_Down_Type		= false;
			Settings.AnalogR_Left_Type		= false;
			Settings.AnalogR_Right_Type		= false;
			break;
	}
}

bool Emulator_Implementation_SaveSettings()
{
	currentconfig.SetBool("FCEU::GameGenie",Settings.FCEUGameGenie);
	currentconfig.SetBool("PS3General::KeepAspect",Settings.PS3KeepAspect);
	currentconfig.SetBool("PS3General::Smooth", Settings.PS3Smooth);
	currentconfig.SetBool("PS3General::OverscanEnabled", Settings.PS3OverscanEnabled);
	currentconfig.SetInt("PS3General::OverscanAmount",Settings.PS3OverscanAmount);
	currentconfig.SetBool("PS3General::PS3PALTemporalMode60Hz",Settings.PS3PALTemporalMode60Hz);
	currentconfig.SetInt("FCEU::Controlstyle",Settings.FCEUControlstyle);
	currentconfig.SetBool("FCEU::DisableSpriteLimitation",Settings.FCEUDisableSpriteLimitation);
	currentconfig.SetString("FCEU::Shader",Graphics->GetFragmentShaderPath());
	currentconfig.SetString("PS3Paths::PathSaveStates",Settings.PS3PathSaveStates);
	currentconfig.SetString("PS3Paths::PathCheats",Settings.PS3PathCheats);
	currentconfig.SetString("PS3Paths::PathROMDirectory",Settings.PS3PathROMDirectory);
	currentconfig.SetString("PS3Paths::PathSRAM",Settings.PS3PathSRAM);
	currentconfig.SetString("PS3Paths::PathBaseDirectory",Settings.PS3PathBaseDirectory);
	currentconfig.SetString("PS3Paths::PathScreenshots",Settings.PS3PathScreenshots);
	currentconfig.SetString("RSound::RSoundServerIPAddress",Settings.RSoundServerIPAddress);
	currentconfig.SetBool("RSound::RSoundEnabled",Settings.RSoundEnabled);
	Emulator_Implementation_ButtonMappingSettings(MAP_BUTTONS_OPTION_SETTER);
	return currentconfig.SaveTo(SYS_CONFIG_FILE);
}


void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g,
        unsigned char b) {
    // Make PC compatible copy
    Graphics->Palette[index].r = r;
    Graphics->Palette[index].g = g;
    Graphics->Palette[index].b = b;
}


void FCEUD_GetPalette(unsigned char i, unsigned char *r, unsigned char *g,
        unsigned char *b) {
    *r = Graphics->Palette[i].r;
    *g = Graphics->Palette[i].g;
    *b = Graphics->Palette[i].b;
}


FILE *FCEUD_UTF8fopen(const char *n, const char *m)
{
    return fopen(n, m);

}

EMUFILE_FILE* FCEUD_UTF8_fstream(const char *n, const char *m)
{
	EMUFILE_FILE *p = new EMUFILE_FILE(n, m);
	return p;
}

bool FCEUD_ShouldDrawInputAids()
{
        return false;
}


// General Logging
void FCEUD_PrintError(const char *c)
{
	EMU_DBG("%s", c);
	FCEU_DispMessage((char*)c, 20);
}

void FCEUD_Message(const char *text)
{
	EMU_DBG("%s", text);
	FCEU_DispMessage((char*)text, 20);
}

void FCEUD_VideoChanged()
{
	LOG_DBG("Videomode: %s\n", GameInfo->vidsys == 1 ? "PAL" : "NTSC");
	if (Graphics->GetCurrentResolution() == CELL_VIDEO_OUT_RESOLUTION_576)
	{
		if(Graphics->CheckResolution(CELL_VIDEO_OUT_RESOLUTION_576))
		{
			switch(Graphics->GetPAL60Hz())
			{
				case 0:
					//PAL60 is OFF
					if(GameInfo->vidsys == 0)
					{
						Settings.PS3PALTemporalMode60Hz = true;
						Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
						Graphics->SwitchResolution(Graphics->GetCurrentResolution(), Settings.PS3PALTemporalMode60Hz);
					}
					break;
				case 1:
					//PAL60 is ON
					if(GameInfo->vidsys == 1)
					{
						Settings.PS3PALTemporalMode60Hz = false;
						Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
						Graphics->SwitchResolution(Graphics->GetCurrentResolution(), Settings.PS3PALTemporalMode60Hz);
					}
					break;
			}
		}
	}
}

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex)
{
	return 0;
}

FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename)
{
	return 0;
}

ArchiveScanRecord FCEUD_ScanArchive(std::string fname)
{
	return ArchiveScanRecord();
}

// Netplay
int FCEUD_SendData(void *data, uint32 len)
{
    return 1;
}

int FCEUD_RecvData(void *data, uint32 len)
{
    return 0;
}

void FCEUD_NetworkClose(void)
{
}

void FCEUD_NetplayText(uint8 *text)
{
}

// dummy functions

#define DUMMY(f) void f(void) { }
DUMMY(FCEUD_HideMenuToggle)
DUMMY(FCEUD_TurboOn)
DUMMY(FCEUD_TurboOff)
DUMMY(FCEUD_TurboToggle)
DUMMY(FCEUD_SaveStateAs)
DUMMY(FCEUD_LoadStateFrom)
DUMMY(FCEUD_MovieRecordTo)
DUMMY(FCEUD_MovieReplayFrom)
DUMMY(FCEUD_ToggleStatusIcon)
DUMMY(FCEUD_DebugBreakpoint)
DUMMY(FCEUD_SoundToggle)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char* buffer) { }
int FCEUD_ShowStatusIcon(void) { return 0; }
bool FCEUI_AviIsRecording(void) { return 0; }
bool FCEUI_AviDisableMovieMessages() { return true; }
const char *FCEUD_GetCompilerString() { return NULL; }
void FCEUI_UseInputPreset(int preset) { }
void FCEUD_SoundVolumeAdjust(int n) { }
void FCEUD_SetEmulationSpeed(int cmd) { }
bool FCEUD_PauseAfterPlayback() { }
void FCEUD_SetInput(bool fourscore, bool microphone, ESI port0, ESI port1, ESIFC fcexp) { }




/*
struct st_palettes palettes[] = {
        { "asqrealc", "AspiringSquire's Real palette",
                { 0x6c6c6c, 0x00268e, 0x0000a8, 0x400094,
                        0x700070, 0x780040, 0x700000, 0x621600,
                        0x442400, 0x343400, 0x005000, 0x004444,
                        0x004060, 0x000000, 0x101010, 0x101010,
                        0xbababa, 0x205cdc, 0x3838ff, 0x8020f0,
                        0xc000c0, 0xd01474, 0xd02020, 0xac4014,
                        0x7c5400, 0x586400, 0x008800, 0x007468,
                        0x00749c, 0x202020, 0x101010, 0x101010,
                        0xffffff, 0x4ca0ff, 0x8888ff, 0xc06cff,
                        0xff50ff, 0xff64b8, 0xff7878, 0xff9638,
                        0xdbab00, 0xa2ca20, 0x4adc4a, 0x2ccca4,
                        0x1cc2ea, 0x585858, 0x101010, 0x101010,
                        0xffffff, 0xb0d4ff, 0xc4c4ff, 0xe8b8ff,
                        0xffb0ff, 0xffb8e8, 0xffc4c4, 0xffd4a8,
                        0xffe890, 0xf0f4a4, 0xc0ffc0, 0xacf4f0,
                        0xa0e8ff, 0xc2c2c2, 0x202020, 0x101010 }
        },
    { "loopy", "Loopy's palette",
        { 0x757575, 0x271b8f, 0x0000ab, 0x47009f,
            0x8f0077, 0xab0013, 0xa70000, 0x7f0b00,
            0x432f00, 0x004700, 0x005100, 0x003f17,
            0x1b3f5f, 0x000000, 0x000000, 0x000000,
            0xbcbcbc, 0x0073ef, 0x233bef, 0x8300f3,
            0xbf00bf, 0xe7005b, 0xdb2b00, 0xcb4f0f,
            0x8b7300, 0x009700, 0x00ab00, 0x00933b,
            0x00838b, 0x000000, 0x000000, 0x000000,
            0xffffff, 0x3fbfff, 0x5f97ff, 0xa78bfd,
            0xf77bff, 0xff77b7, 0xff7763, 0xff9b3b,
            0xf3bf3f, 0x83d313, 0x4fdf4b, 0x58f898,
            0x00ebdb, 0x000000, 0x000000, 0x000000,
            0xffffff, 0xabe7ff, 0xc7d7ff, 0xd7cbff,
            0xffc7ff, 0xffc7db, 0xffbfb3, 0xffdbab,
            0xffe7a3, 0xe3ffa3, 0xabf3bf, 0xb3ffcf,
            0x9ffff3, 0x000000, 0x000000, 0x000000 }
    },
    { "quor", "Quor's palette",
        { 0x3f3f3f, 0x001f3f, 0x00003f, 0x1f003f,
            0x3f003f, 0x3f0020, 0x3f0000, 0x3f2000,
            0x3f3f00, 0x203f00, 0x003f00, 0x003f20,
            0x003f3f, 0x000000, 0x000000, 0x000000,
            0x7f7f7f, 0x405f7f, 0x40407f, 0x5f407f,
            0x7f407f, 0x7f4060, 0x7f4040, 0x7f6040,
            0x7f7f40, 0x607f40, 0x407f40, 0x407f60,
            0x407f7f, 0x000000, 0x000000, 0x000000,
            0xbfbfbf, 0x809fbf, 0x8080bf, 0x9f80bf,
            0xbf80bf, 0xbf80a0, 0xbf8080, 0xbfa080,
            0xbfbf80, 0xa0bf80, 0x80bf80, 0x80bfa0,
            0x80bfbf, 0x000000, 0x000000, 0x000000,
            0xffffff, 0xc0dfff, 0xc0c0ff, 0xdfc0ff,
            0xffc0ff, 0xffc0e0, 0xffc0c0, 0xffe0c0,
            0xffffc0, 0xe0ffc0, 0xc0ffc0, 0xc0ffe0,
            0xc0ffff, 0x000000, 0x000000, 0x000000 }
    },
    { "chris", "Chris Covell's palette",
        { 0x808080, 0x003DA6, 0x0012B0, 0x440096,
            0xA1005E, 0xC70028, 0xBA0600, 0x8C1700,
            0x5C2F00, 0x104500, 0x054A00, 0x00472E,
            0x004166, 0x000000, 0x050505, 0x050505,
            0xC7C7C7, 0x0077FF, 0x2155FF, 0x8237FA,
            0xEB2FB5, 0xFF2950, 0xFF2200, 0xD63200,
            0xC46200, 0x358000, 0x058F00, 0x008A55,
            0x0099CC, 0x212121, 0x090909, 0x090909,
            0xFFFFFF, 0x0FD7FF, 0x69A2FF, 0xD480FF,
            0xFF45F3, 0xFF618B, 0xFF8833, 0xFF9C12,
            0xFABC20, 0x9FE30E, 0x2BF035, 0x0CF0A4,
            0x05FBFF, 0x5E5E5E, 0x0D0D0D, 0x0D0D0D,
            0xFFFFFF, 0xA6FCFF, 0xB3ECFF, 0xDAABEB,
            0xFFA8F9, 0xFFABB3, 0xFFD2B0, 0xFFEFA6,
            0xFFF79C, 0xD7E895, 0xA6EDAF, 0xA2F2DA,
            0x99FFFC, 0xDDDDDD, 0x111111, 0x111111 }
    },
    { "matt", "Matthew Conte's palette",
        { 0x808080, 0x0000bb, 0x3700bf, 0x8400a6,
            0xbb006a, 0xb7001e, 0xb30000, 0x912600,
            0x7b2b00, 0x003e00, 0x00480d, 0x003c22,
            0x002f66, 0x000000, 0x050505, 0x050505,
            0xc8c8c8, 0x0059ff, 0x443cff, 0xb733cc,
            0xff33aa, 0xff375e, 0xff371a, 0xd54b00,
            0xc46200, 0x3c7b00, 0x1e8415, 0x009566,
            0x0084c4, 0x111111, 0x090909, 0x090909,
            0xffffff, 0x0095ff, 0x6f84ff, 0xd56fff,
            0xff77cc, 0xff6f99, 0xff7b59, 0xff915f,
            0xffa233, 0xa6bf00, 0x51d96a, 0x4dd5ae,
            0x00d9ff, 0x666666, 0x0d0d0d, 0x0d0d0d,
            0xffffff, 0x84bfff, 0xbbbbff, 0xd0bbff,
            0xffbfea, 0xffbfcc, 0xffc4b7, 0xffccae,
            0xffd9a2, 0xcce199, 0xaeeeb7, 0xaaf7ee,
            0xb3eeff, 0xdddddd, 0x111111, 0x111111 }
    },
    { "pasofami", "PasoFami/99 palette",
        { 0x7f7f7f, 0x0000ff, 0x0000bf, 0x472bbf,
            0x970087, 0xab0023, 0xab1300, 0x8b1700,
            0x533000, 0x007800, 0x006b00, 0x005b00,
            0x004358, 0x000000, 0x000000, 0x000000,
            0xbfbfbf, 0x0078f8, 0x0058f8, 0x6b47ff,
            0xdb00cd, 0xe7005b, 0xf83800, 0xe75f13,
            0xaf7f00, 0x00b800, 0x00ab00, 0x00ab47,
            0x008b8b, 0x000000, 0x000000, 0x000000,
            0xf8f8f8, 0x3fbfff, 0x6b88ff, 0x9878f8,
            0xf878f8, 0xf85898, 0xf87858, 0xffa347,
            0xf8b800, 0xb8f818, 0x5bdb57, 0x58f898,
            0x00ebdb, 0x787878, 0x000000, 0x000000,
            0xffffff, 0xa7e7ff, 0xb8b8f8, 0xd8b8f8,
            0xf8b8f8, 0xfba7c3, 0xf0d0b0, 0xffe3ab,
            0xfbdb7b, 0xd8f878, 0xb8f8b8, 0xb8f8d8,
            0x00ffff, 0xf8d8f8, 0x000000, 0x000000 }
    },
    { "crashman", "CrashMan's palette",
        { 0x585858, 0x001173, 0x000062, 0x472bbf,
            0x970087, 0x910009, 0x6f1100, 0x4c1008,
            0x371e00, 0x002f00, 0x005500, 0x004d15,
            0x002840, 0x000000, 0x000000, 0x000000,
            0xa0a0a0, 0x004499, 0x2c2cc8, 0x590daa,
            0xae006a, 0xb00040, 0xb83418, 0x983010,
            0x704000, 0x308000, 0x207808, 0x007b33,
            0x1c6888, 0x000000, 0x000000, 0x000000,
            0xf8f8f8, 0x267be1, 0x5870f0, 0x9878f8,
            0xff73c8, 0xf060a8, 0xd07b37, 0xe09040,
            0xf8b300, 0x8cbc00, 0x40a858, 0x58f898,
            0x00b7bf, 0x787878, 0x000000, 0x000000,
            0xffffff, 0xa7e7ff, 0xb8b8f8, 0xd8b8f8,
            0xe6a6ff, 0xf29dc4, 0xf0c0b0, 0xfce4b0,
            0xe0e01e, 0xd8f878, 0xc0e890, 0x95f7c8,
            0x98e0e8, 0xf8d8f8, 0x000000, 0x000000 }
    },
    { "mess", "MESS palette",
        { 0x747474, 0x24188c, 0x0000a8, 0x44009c,
            0x8c0074, 0xa80010, 0xa40000, 0x7c0800,
            0x402c00, 0x004400, 0x005000, 0x003c14,
            0x183c5c, 0x000000, 0x000000, 0x000000,
            0xbcbcbc, 0x0070ec, 0x2038ec, 0x8000f0,
            0xbc00bc, 0xe40058, 0xd82800, 0xc84c0c,
            0x887000, 0x009400, 0x00a800, 0x009038,
            0x008088, 0x000000, 0x000000, 0x000000,
            0xfcfcfc, 0x3cbcfc, 0x5c94fc, 0x4088fc,
            0xf478fc, 0xfc74b4, 0xfc7460, 0xfc9838,
            0xf0bc3c, 0x80d010, 0x4cdc48, 0x58f898,
            0x00e8d8, 0x000000, 0x000000, 0x000000,
            0xfcfcfc, 0xa8e4fc, 0xc4d4fc, 0xd4c8fc,
            0xfcc4fc, 0xfcc4d8, 0xfcbcb0, 0xfcd8a8,
            0xfce4a0, 0xe0fca0, 0xa8f0bc, 0xb0fccc,
            0x9cfcf0, 0x000000, 0x000000, 0x000000 }
    },
    { "zaphod-cv", "Zaphod's VS Castlevania palette",
        { 0x7f7f7f, 0xffa347, 0x0000bf, 0x472bbf,
            0x970087, 0xf85898, 0xab1300, 0xf8b8f8,
            0xbf0000, 0x007800, 0x006b00, 0x005b00,
            0xffffff, 0x9878f8, 0x000000, 0x000000,
            0xbfbfbf, 0x0078f8, 0xab1300, 0x6b47ff,
            0x00ae00, 0xe7005b, 0xf83800, 0x7777ff,
            0xaf7f00, 0x00b800, 0x00ab00, 0x00ab47,
            0x008b8b, 0x000000, 0x000000, 0x472bbf,
            0xf8f8f8, 0xffe3ab, 0xf87858, 0x9878f8,
            0x0078f8, 0xf85898, 0xbfbfbf, 0xffa347,
            0xc800c8, 0xb8f818, 0x7f7f7f, 0x007800,
            0x00ebdb, 0x000000, 0x000000, 0xffffff,
            0xffffff, 0xa7e7ff, 0x5bdb57, 0xe75f13,
            0x004358, 0x0000ff, 0xe7005b, 0x00b800,
            0xfbdb7b, 0xd8f878, 0x8b1700, 0xffe3ab,
            0x00ffff, 0xab0023, 0x000000, 0x000000 }
    },
    { "zaphod-smb", "Zaphod's VS SMB palette",
        { 0x626a00, 0x0000ff, 0x006a77, 0x472bbf,
            0x970087, 0xab0023, 0xab1300, 0xb74800,
            0xa2a2a2, 0x007800, 0x006b00, 0x005b00,
            0xffd599, 0xffff00, 0x009900, 0x000000,
            0xff66ff, 0x0078f8, 0x0058f8, 0x6b47ff,
            0x000000, 0xe7005b, 0xf83800, 0xe75f13,
            0xaf7f00, 0x00b800, 0x5173ff, 0x00ab47,
            0x008b8b, 0x000000, 0x91ff88, 0x000088,
            0xf8f8f8, 0x3fbfff, 0x6b0000, 0x4855f8,
            0xf878f8, 0xf85898, 0x595958, 0xff009d,
            0x002f2f, 0xb8f818, 0x5bdb57, 0x58f898,
            0x00ebdb, 0x787878, 0x000000, 0x000000,
            0xffffff, 0xa7e7ff, 0x590400, 0xbb0000,
            0xf8b8f8, 0xfba7c3, 0xffffff, 0x00e3e1,
            0xfbdb7b, 0xffae00, 0xb8f8b8, 0xb8f8d8,
            0x00ff00, 0xf8d8f8, 0xffaaaa, 0x004000 }
    },
    { "vs-drmar", "VS Dr. Mario palette",
        { 0x5f97ff, 0x000000, 0x000000, 0x47009f,
            0x00ab00, 0xffffff, 0xabe7ff, 0x000000,
            0x000000, 0x000000, 0x000000, 0x000000,
            0xe7005b, 0x000000, 0x000000, 0x000000,
            0x5f97ff, 0x000000, 0x000000, 0x000000,
            0x000000, 0x8b7300, 0xcb4f0f, 0x000000,
            0xbcbcbc, 0x000000, 0x000000, 0x000000,
            0x000000, 0x000000, 0x000000, 0x000000,
            0x00ebdb, 0x000000, 0x000000, 0x000000,
            0x000000, 0xff9b3b, 0x000000, 0x000000,
            0x83d313, 0x000000, 0x3fbfff, 0x000000,
            0x0073ef, 0x000000, 0x000000, 0x000000,
            0x00ebdb, 0x000000, 0x000000, 0x000000,
            0x000000, 0x000000, 0xf3bf3f, 0x000000,
            0x005100, 0x000000, 0xc7d7ff, 0xffdbab,
            0x000000, 0x000000, 0x000000, 0x000000 }
    },
    { "vs-cv", "VS Castlevania palette",
        { 0xaf7f00, 0xffa347, 0x008b8b, 0x472bbf,
            0x970087, 0xf85898, 0xab1300, 0xf8b8f8,
            0xf83800, 0x007800, 0x006b00, 0x005b00,
            0xffffff, 0x9878f8, 0x00ab00, 0x000000,
            0xbfbfbf, 0x0078f8, 0xab1300, 0x6b47ff,
            0x000000, 0xe7005b, 0xf83800, 0x6b88ff,
            0xaf7f00, 0x00b800, 0x6b88ff, 0x00ab47,
            0x008b8b, 0x000000, 0x000000, 0x472bbf,
            0xf8f8f8, 0xffe3ab, 0xf87858, 0x9878f8,
            0x0078f8, 0xf85898, 0xbfbfbf, 0xffa347,
            0x004358, 0xb8f818, 0x7f7f7f, 0x007800,
            0x00ebdb, 0x000000, 0x000000, 0xffffff,
            0xffffff, 0xa7e7ff, 0x5bdb57, 0x6b88ff,
            0x004358, 0x0000ff, 0xe7005b, 0x00b800,
            0xfbdb7b, 0xffa347, 0x8b1700, 0xffe3ab,
            0xb8f818, 0xab0023, 0x000000, 0x007800 }
    },
    { "vs-smb", "VS SMB/VS Ice Climber palette",
        { 0xaf7f00, 0x0000ff, 0x008b8b, 0x472bbf,
            0x970087, 0xab0023, 0x0000ff, 0xe75f13,
            0xbfbfbf, 0x007800, 0x5bdb57, 0x005b00,
            0xf0d0b0, 0xffe3ab, 0x00ab00, 0x000000,
            0xbfbfbf, 0x0078f8, 0x0058f8, 0x6b47ff,
            0x000000, 0xe7005b, 0xf83800, 0xf87858,
            0xaf7f00, 0x00b800, 0x6b88ff, 0x00ab47,
            0x008b8b, 0x000000, 0x000000, 0x3fbfff,
            0xf8f8f8, 0x006b00, 0x8b1700, 0x9878f8,
            0x6b47ff, 0xf85898, 0x7f7f7f, 0xe7005b,
            0x004358, 0xb8f818, 0x0078f8, 0x58f898,
            0x00ebdb, 0xfbdb7b, 0x000000, 0x000000,
            0xffffff, 0xa7e7ff, 0xb8b8f8, 0xf83800,
            0xf8b8f8, 0xfba7c3, 0xffffff, 0x00ffff,
            0xfbdb7b, 0xffa347, 0xb8f8b8, 0xb8f8d8,
            0xb8f818, 0xf8d8f8, 0x000000, 0x007800 }
    }
};
*/
