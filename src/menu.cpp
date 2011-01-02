/*
 * menu.cpp
 *
 *  Created on: Oct 10, 2010
 *      Author: halsafar
 */
#include <string.h>
#include <stack>

#include <cell/audio.h>
#include <cell/sysmodule.h>
#include <cell/cell_fs.h>
#include <cell/pad.h>
#include <cell/dbgfont.h>
#include <sysutil/sysutil_sysparam.h>

#include "fceu_ps3.h"
#include "menu.h"

#include "FceuGraphics.h"
#include "cellControlConsole.h"
#include "emulator_implementation.h"

#include "cellframework/input/cellInput.h"
#include "cellframework/fileio/FileBrowser.h"
#include "cellframework/logger/Logger.h"

#include "conf/conffile.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#define NUM_ENTRY_PER_PAGE 22
#define NUM_ENTRY_PER_SETTINGS_PAGE 18

// is the menu running
bool menuRunning = false;

// function pointer for menu render functions
typedef void (*curMenuPtr)();

// menu stack
std::stack<curMenuPtr> menuStack;

// main file browser for rom browser
FileBrowser* browser = NULL;

// tmp file browser for everything else
FileBrowser* tmpBrowser = NULL;

int16_t	currently_selected_setting		= FIRST_GENERAL_SETTING;
int16_t currently_selected_fceu_setting		= FIRST_FCEU_SETTING;
int16_t currently_selected_path_setting		= FIRST_PATH_SETTING;
int16_t currently_selected_controller_setting	= FIRST_CONTROLS_SETTING;

#define FILEBROWSER_DELAY	100000
#define SETTINGS_DELAY		150000	

void MenuStop()
{
	menuRunning = false;
}


bool MenuIsRunning()
{
	return menuRunning;
}

void MenuResetControlStyle()
{
	if(Settings.ButtonCircle == BTN_A && Settings.ButtonCross == BTN_B)
	{
		control_style = CONTROL_STYLE_ORIGINAL;
		Settings.FCEUControlstyle = CONTROL_STYLE_ORIGINAL;
	}
}

void UpdateBrowser(FileBrowser* b)
{
	if (CellInput->WasButtonHeld(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0,CTRL_LSTICK))
	{
		if(b->GetCurrentEntryIndex() < b->GetCurrentDirectoryInfo().numEntries-1)
		{
			b->IncrementEntry();
			if(CellInput->IsButtonPressed(0,CTRL_DOWN))
			{
				sys_timer_usleep(FILEBROWSER_DELAY);
			}
		}
	}
	if (CellInput->WasButtonHeld(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0,CTRL_LSTICK))
	{
		if(b->GetCurrentEntryIndex() > 0)
		{
			b->DecrementEntry();
			if(CellInput->IsButtonPressed(0,CTRL_UP))
			{
				sys_timer_usleep(FILEBROWSER_DELAY);
			}
		}
	}
	if (CellInput->WasButtonHeld(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
	{
		b->GotoEntry(MIN(b->GetCurrentEntryIndex()+5, b->GetCurrentDirectoryInfo().numEntries-1));
		if(CellInput->IsButtonPressed(0,CTRL_RIGHT))
		{
			sys_timer_usleep(FILEBROWSER_DELAY);
		}
	}
	if (CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
	{
		if (b->GetCurrentEntryIndex() <= 5)
		{
			b->GotoEntry(0);
			if(CellInput->IsButtonPressed(0,CTRL_LEFT))
			{
				sys_timer_usleep(FILEBROWSER_DELAY);
			}
		}
		else
		{
			b->GotoEntry(b->GetCurrentEntryIndex()-5);
			if(CellInput->IsButtonPressed(0,CTRL_LEFT))
			{
				sys_timer_usleep(FILEBROWSER_DELAY);
			}
		}
	}
	if (CellInput->WasButtonHeld(0,CTRL_R1))
	{
		b->GotoEntry(MIN(b->GetCurrentEntryIndex()+NUM_ENTRY_PER_PAGE, b->GetCurrentDirectoryInfo().numEntries-1));
		if(CellInput->IsButtonPressed(0,CTRL_R1))
		{
			sys_timer_usleep(FILEBROWSER_DELAY);
		}
	}
	if (CellInput->WasButtonHeld(0,CTRL_L1))
	{
		if (b->GetCurrentEntryIndex() <= NUM_ENTRY_PER_PAGE)
		{
			b->GotoEntry(0);
		}
		else
		{
			b->GotoEntry(b->GetCurrentEntryIndex()-NUM_ENTRY_PER_PAGE);
		}
		if(CellInput->IsButtonPressed(0,CTRL_L1))
		{
			sys_timer_usleep(FILEBROWSER_DELAY);
		}
	}

	if (CellInput->WasButtonPressed(0, CTRL_CIRCLE))
	{
		// don't let people back out past root
		if (b->DirectoryStackCount() > 1)
		{
			b->PopDirectory();
		}
	}
}


void RenderBrowser(FileBrowser* b)
{
	uint32_t file_count = b->GetCurrentDirectoryInfo().numEntries;
	int current_index = b->GetCurrentEntryIndex();

	if (file_count <= 0)
	{
		LOG_DBG("1: filecount <= 0");
	}
	else if (current_index > file_count)
	{
		LOG_DBG("2: current_index >= file_count");
	}
	else
	{
		int page_number = current_index / NUM_ENTRY_PER_PAGE;
		int page_base = page_number * NUM_ENTRY_PER_PAGE;
		float currentX = 0.09f;
		float currentY = 0.09f;
		float ySpacing = 0.035f;
		for (int i = page_base; i < file_count && i < page_base + NUM_ENTRY_PER_PAGE; ++i)
		{
			currentY = currentY + ySpacing;
			cellDbgFontPuts(currentX, currentY, 1.00f,
					i == current_index ? RED : (*b)[i]->d_type == CELL_FS_TYPE_DIRECTORY ? GREEN : WHITE,
					(*b)[i]->d_name);
			Graphics->FlushDbgFont();
		}
	}
	Graphics->FlushDbgFont();
}


void do_shaderChoice()
{
	if (tmpBrowser == NULL)
	{
		tmpBrowser = new FileBrowser("/dev_hdd0/game/FCEU90000/USRDIR/shaders/\0");
	}

	string path;

	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		UpdateBrowser(tmpBrowser);

		if (CellInput->WasButtonPressed(0,CTRL_CROSS))
		{
			if(tmpBrowser->IsCurrentADirectory())
			{
				tmpBrowser->PushDirectory(	tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name,
						CELL_FS_TYPE_REGULAR | CELL_FS_TYPE_DIRECTORY,"cg");
			}
			else if (tmpBrowser->IsCurrentAFile())
			{
				path = tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name;

				// load shader
				Graphics->LoadFragmentShader(path);
				Graphics->UpdateCgParams(256, 240, 256, 240);
				menuStack.pop();
			}
		}

		if (CellInput->WasButtonHeld(0, CTRL_TRIANGLE))
		{
			menuStack.pop();
		}
	}

	cellDbgFontPrintf(0.09f, 0.09f, 1.00f, YELLOW, "PATH: %s", tmpBrowser->GetCurrentDirectoryInfo().dir.c_str());
	cellDbgFontPuts	(0.09f,	0.05f,	1.00f,	RED,	"SHADER SELECTION");
	cellDbgFontPuts(0.09f, 0.92f, 1.00f, YELLOW,
	"X - Select shader               /\\ - return to settings");
	cellDbgFontPrintf(0.09f, 0.89f, 0.86f, LIGHTBLUE, "%s",
	"INFO - Select a shader from the menu by pressing the X button. ");
	Graphics->FlushDbgFont();

	RenderBrowser(tmpBrowser);
}

void DisplayHelpMessage(int currentsetting)
{
	switch(currentsetting)
	{
		case SETTING_CURRENT_SAVE_STATE_SLOT:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set the current savestate slot (can also be configured ingame).\n");
			break;
		case SETTING_PATH_SAVESTATES_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set the default path where all the savestate files will be saved.\n");
			break;
		case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set the default ROM startup directory. NOTE: You will have to\nrestart the emulator for this change to have any effect.\n");
			break;
		case SETTING_PATH_SRAM_DIRECTORY:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set the default SRAM (SaveRAM) directory path.\n");
			break;
		case SETTING_PATH_CHEATS:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set the default cheatfile directory path. All CHT files\nwill be stored here.\n");
			break;
		case SETTING_SHADER:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Select a pixel shader. NOTE: Some shaders might be too slow at 1080p.\nIf you experience any slowdown, try another shader.");
			break;
/*
		case SETTING_SNES9X_FPS:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.DisplayFrameRate ? "INFO - Display Framerate is set to 'Enabled' - an FPS counter is displayed onscreen." : "INFO - Display Framerate is set to 'Disabled'.");
			break;
		case SETTING_SNES9X_SOUND_INPUT_RATE:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set the Sound Inputrate. The default value (31942) is ideal for A/V\nsynchronization.");
			break;
		case SETTING_SNES9X_TRANSPARENCY:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.Transparency ? "INFO - Transparency is set to 'On'. This is the default recommended setting." : "INFO - Transparency is set to 'Off'. Certain transparency features used\nby the SNES in games will not be displayed.");
			break;
		case SETTING_SNES9X_SKIP_FRAMES:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set frameskipping. NOTE: This option doesn't work at the moment.");
			break;
		case SETTING_SNES9X_DISABLE_GRAPHIC_WINDOWS:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.DisableGraphicWindows ? "INFO - Graphic windows is set to 'Off'. Certain graphical features used\nby the SNES in games will be disabled." : "INFO - Graphic windows is set to 'On'. This is the default recommended setting.");
			break;
		case SETTING_SNES9X_DISPLAY_PRESSED_KEYS:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.DisplayPressedKeys ? "INFO - Display Pressed Keys is set to 'On'. Button input will be displayed\non-screen." : "INFO - Display Pressed Keys is set to 'Off'.");
			break;
		case SETTING_SNES9X_FORCE_PAL:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.ForcePAL ? "INFO - Force PAL is set to 'On' - this will force NTSC games to run at PAL\nfrequencies (50Hz). NOTE: This needs to be set per-game." : "INFO - Force PAL is set to 'Off'.");
			break;
		case SETTING_SNES9X_FORCE_NTSC:
			if(Settings.ForceNTSC)
			{
				cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Force NTSC is set to 'On' - this will force PAL games to run at\nNTSC frequencies (60Hz). NOTE: This needs to be set per-game.");
			}
			else
			{
				cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Force NTSC is set to 'Off'.");
			}
			break;
		case SETTING_SNES9X_PAL_TIMING:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Change PAL timing frequency. NOTE: Doesn't need changing from the\ndefault value.");
			break;
		case SETTING_SNES9X_AUTO_APPLY_CHEATS:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.ApplyCheats ? "INFO - Auto-apply cheats is set to 'On'. Automatically apply cheat files if it\nhas the same name as the ROM." : "INFO - Auto-apply cheats is set to 'Off'. Cheat files will not be applied.");
			break;
		case SETTING_SNES9X_AUTO_APPLY_PATCH:
			if(Settings.NoPatch == false)
			{
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s","INFO - Auto-apply IPS/UPS patch is set to 'On'. Automatically applies similarly\nnamed patch files if they are in the same directory as the ROM.");
			}
			else
			{
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Auto-apply IPS/UPS patch is set to 'Off'. Translation/hack IPS/UPS\npatches will not be applied.");
			}
			break;
		case SETTING_SNES9X_RESET_BEFORE_RECORDING_MOVIE:
			if(Settings.ResetBeforeRecordingMovie)
			{
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s","INFO - Reset Before Recording Movie is set to 'On'. The game will reset before\nbeginning to record a movie.");
			}
			else
			{
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Reset Before Recording Movie is set to 'Off'. The game will start\nfrom the current position in the game while recording a movie.");
			}
			break;
		case SETTING_SNES9X_TURBO_SKIP_FRAMES:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Set the amount of frames to be skipped during 'Fast Forward'.");
			break;
*/
		case SETTING_CHANGE_RESOLUTION:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", "INFO - Change the display resolution - press X to confirm.");
			#ifndef PS3_SDK_3_41
				cellDbgFontPrintf(0.09f, 0.86f, 0.86f, RED, "%s", "WARNING - This setting might not work correctly on 1.92 FW.");
			#endif
			break;
		case SETTING_PAL60_MODE:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.PS3PALTemporalMode60Hz ? "INFO - PAL 60Hz mode is enabled - 60Hz NTSC games will run correctly at 576p PAL\nresolution. NOTE: This is configured on-the-fly." : "INFO - PAL 60Hz mode disabled - 50Hz PAL games will run correctly at 576p PAL\nresolution. NOTE: This is configured on-the-fly.");
			break;
		case SETTING_HW_TEXTURE_FILTER:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.PS3Smooth ? "INFO - Hardware filtering is set to 'Bilinear filtering'." : "INFO - Hardware filtering is set to 'Point filtering' - most shaders\nlook much better on this setting.");
			break;
		case SETTING_KEEP_ASPECT_RATIO:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.PS3KeepAspect ? "INFO - Aspect ratio is set to 'Scaled' - screen will have black borders\nleft and right on widescreen TVs/monitors." : "INFO - Aspect ratio is set to 'Stretched' - widescreen TVs/monitors will have\na full stretched picture.");
			break;
		case SETTING_RSOUND_ENABLED:
			cellDbgFontPrintf(0.09f, 0.83f, 0.86f, LIGHTBLUE, "%s", Settings.RSoundEnabled ? "INFO - Sound is set to 'RSound' - the sound will be streamed over the network\nto the RSound audio server." : "INFO - Sound is set to 'Normal' - normal audio output will be used.");
			break;
		case SETTING_RSOUND_SERVER_IP_ADDRESS:
			cellDbgFontPuts(0.09f, 0.83f, 0.86f, LIGHTBLUE, "INFO - Enter the IP address of the RSound audio server. IP address must be\nan IPv4 32-bits address, eg: '192.168.0.7'.");
			break;
/*
		case SETTING_FONT_SIZE:
			cellDbgFontPuts(0.09f, 0.83f, 0.86f, LIGHTBLUE, "INFO - Increase or decrease the font size in the menu.");
			break;
*/
		case SETTING_HW_OVERSCAN_AMOUNT:
			cellDbgFontPuts(0.09f, 0.83f, 0.86f, LIGHTBLUE, "INFO - Adjust or decrease overscan. Set this to higher than 0.000\nif the screen doesn't fit on your TV/monitor.");
			break;
		case SETTING_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.83f, 0.86f, LIGHTBLUE, "INFO - Set all 'General' settings back to their default values.");
			break;
		case SETTING_FCEU_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.83f, 0.86f, LIGHTBLUE, "INFO - Set all 'FCEU' settings back to their default values.");
			break;
		case SETTING_PATH_DEFAULT_ALL:
			cellDbgFontPuts(0.09f, 0.83f, 0.86f, LIGHTBLUE, "INFO - Set all 'Path' settings back to their default values.");
			break;
	}
}

void do_pathChoice()
{
	if (tmpBrowser == NULL)
	{
		tmpBrowser = new FileBrowser("/\0");
	}
	string path;

	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		UpdateBrowser(tmpBrowser);
		if (CellInput->WasButtonPressed(0,CTRL_SQUARE))
		{
			if(tmpBrowser->IsCurrentADirectory())
			{
				path = tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name + "/";
				switch(currently_selected_path_setting)
				{
					case SETTING_PATH_SAVESTATES_DIRECTORY:
						Settings.PS3PathSaveStates = path;
						Emulator_FceuSetPaths();
						break;
					case SETTING_PATH_SRAM_DIRECTORY:
						Settings.PS3PathSRAM = path;
						Emulator_FceuSetPaths();
						break;
					case SETTING_PATH_CHEATS:
						Settings.PS3PathCheats = path;
						Emulator_FceuSetPaths();
						break;
					case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
						Settings.PS3PathROMDirectory = path;
						break;
					case SETTING_PATH_BASE_DIRECTORY:
						Settings.PS3PathBaseDirectory = path;
						Emulator_FceuSetPaths();
						break;
				}
				menuStack.pop();
			}
		}
		if (CellInput->WasButtonHeld(0, CTRL_TRIANGLE))
		{
			path = USRDIR;
			switch(currently_selected_path_setting)
			{
				case SETTING_PATH_SAVESTATES_DIRECTORY:
					Settings.PS3PathSaveStates = path;
					break;
				case SETTING_PATH_SRAM_DIRECTORY:
					Settings.PS3PathSRAM = path;
					break;
				case SETTING_PATH_CHEATS:
					Settings.PS3PathCheats = path;
					break;
				case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
					Settings.PS3PathROMDirectory = "/";
					break;
				case SETTING_PATH_BASE_DIRECTORY:
					Settings.PS3PathBaseDirectory = path;
					break;
			}
			menuStack.pop();
		}
		if (CellInput->WasButtonPressed(0,CTRL_CROSS))
		{
			if(tmpBrowser->IsCurrentADirectory())
			{
				tmpBrowser->PushDirectory(tmpBrowser->GetCurrentDirectoryInfo().dir + "/" + tmpBrowser->GetCurrentEntry()->d_name, CELL_FS_TYPE_REGULAR | CELL_FS_TYPE_DIRECTORY, "empty");
			}
		}
	}
			
	cellDbgFontPrintf(0.09f, 0.09f, 1.00f, YELLOW, "PATH: %s", tmpBrowser->GetCurrentDirectoryInfo().dir.c_str());
	cellDbgFontPuts	(0.09f,	0.05f,	1.00f,	RED,	"DIRECTORY SELECTION");
	cellDbgFontPuts(0.09f, 0.93f, 1.00f, YELLOW,"X - Enter directory             /\\ - return to settings");
	//cellDbgFontPuts(0.09f, 0.93f, 1.00f, YELLOW,"SQUARE - Select directory as path");
	cellDbgFontPrintf(0.09f, 0.89f, 0.86f, LIGHTBLUE, "%s",
	"INFO - Browse to a directory and assign it as the path by pressing SQUARE button.");
	Graphics->FlushDbgFont();
			
	RenderBrowser(tmpBrowser);
}

void do_controls_settings()
{
	if(CellInput->UpdateDevice(0) == CELL_OK)
	{
			// back to ROM menu if CIRCLE is pressed
			if (CellInput->WasButtonPressed(0, CTRL_L1) | CellInput->WasButtonPressed(0, CTRL_CIRCLE))
			{
				menuStack.pop();
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_controller_setting++;
				if (currently_selected_controller_setting >= MAX_NO_OF_CONTROLS_SETTINGS)
				{
					currently_selected_controller_setting = FIRST_CONTROLS_SETTING;
				}
				if(CellInput->IsButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))
				{
					sys_timer_usleep(SETTINGS_DELAY);
				}
			}

			if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
			{
				// if a rom is loaded then resume it
				if (Emulator_IsROMLoaded())
				{
					MenuStop();
					Emulator_StartROMRunning();
				}
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_controller_setting--;
					if (currently_selected_controller_setting < FIRST_CONTROLS_SETTING)
					{
						currently_selected_controller_setting = MAX_NO_OF_CONTROLS_SETTINGS-1;
					}
					if(CellInput->IsButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
			}
					switch(currently_selected_controller_setting)
					{
						case SETTING_CONTROLS_DPAD_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.DPad_Up,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.DPad_Up,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.DPad_Up,true,BTN_UP);
						}
							break;
						case SETTING_CONTROLS_DPAD_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.DPad_Down,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.DPad_Down,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.DPad_Down,true,BTN_DOWN);
						}
							break;
						case SETTING_CONTROLS_DPAD_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.DPad_Left,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.DPad_Left,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.DPad_Left,true,BTN_LEFT);
						}
							break;
						case SETTING_CONTROLS_DPAD_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.DPad_Right,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.DPad_Right,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.DPad_Right,true,BTN_RIGHT);
						}
							break;
						case SETTING_CONTROLS_BUTTON_CIRCLE:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonCircle,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonCircle,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonCircle,true,BTN_A);
						}
							break;
						case SETTING_CONTROLS_BUTTON_CROSS:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonCross,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonCross,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonCross,true,BTN_B);
						}
							break;
						case SETTING_CONTROLS_BUTTON_TRIANGLE:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonTriangle,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonTriangle,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonTriangle,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_SQUARE:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonSquare,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonSquare,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonSquare,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_SELECT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonSelect,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonSelect,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonSelect,true,BTN_SELECT);
						}
							break;
						case SETTING_CONTROLS_BUTTON_START:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonStart,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonStart,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonStart,true,BTN_START);
						}
							break;
						case SETTING_CONTROLS_BUTTON_L1:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL1,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL1,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL1,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_L2:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_R2:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonR2,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonR2,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonR2,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_L3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL3,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL3,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL3,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_R3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonR3,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonR3,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonR3,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_R1:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonR1,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonR1,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonR1,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_L2_BUTTON_L3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonL3,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonL3,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonL3,true,BTN_NONE);
						}
							break;
						case SETTING_CONTROLS_BUTTON_L2_BUTTON_R2:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonR2,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonR2,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonR2,true,BTN_NONE);
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_BUTTON_R3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonR3,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonR3,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2_ButtonR3,true,BTN_QUICKLOAD);
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_BUTTON_R3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonR2_ButtonR3,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonR2_ButtonR3,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonR2_ButtonR3,true,BTN_QUICKSAVE);
						}
							break;
						case SETTING_CONTROLS_ANALOG_R_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.AnalogR_Up,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.AnalogR_Up,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.AnalogR_Up,true,BTN_CHEATENABLE);
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogR_Up_Type = !Settings.AnalogR_Up_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_R_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.AnalogR_Down,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.AnalogR_Down,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.AnalogR_Down,true,BTN_NONE);
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogR_Down_Type = !Settings.AnalogR_Down_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_R_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.AnalogR_Left,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.AnalogR_Left,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.AnalogR_Left,true,BTN_DECREMENTSAVE);
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogR_Left_Type = !Settings.AnalogR_Left_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_R_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.AnalogR_Right,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.AnalogR_Right,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.AnalogR_Right,true,BTN_INCREMENTSAVE);
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogR_Right_Type = !Settings.AnalogR_Right_Type;
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_ANALOG_R_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Right,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Right,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Right,true,BTN_INCREMENTCHEAT);
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_ANALOG_R_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Left,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Left,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Left,true,BTN_DECREMENTCHEAT);
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_ANALOG_R_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Up,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Up,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Up,true,BTN_NONE);
						}
						break;
						case SETTING_CONTROLS_BUTTON_L2_ANALOG_R_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Down,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Down,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Down,true,BTN_NONE);
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_ANALOG_R_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Right,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Right,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonL2_AnalogR_Right,true,BTN_NONE);
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_ANALOG_R_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Left,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Left,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Left,true,BTN_NONE);
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_ANALOG_R_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Up,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Up,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Up,true,BTN_NONE);
						}
						break;
						case SETTING_CONTROLS_BUTTON_R2_ANALOG_R_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Down,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Down,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonR2_AnalogR_Down,true,BTN_NONE);
						}
						break;
						case SETTING_CONTROLS_BUTTON_R3_BUTTON_L3:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.ButtonR3_ButtonL3,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.ButtonR3_ButtonL3,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.ButtonR3_ButtonL3,true,BTN_EXITTOMENU);
						}
						break;

						case SETTING_CONTROLS_ANALOG_L_UP:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.AnalogL_Up,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.AnalogL_Up,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.AnalogL_Up,true,BTN_UP);
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogL_Up_Type = !Settings.AnalogL_Up_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_L_DOWN:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.AnalogL_Down,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.AnalogL_Down,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.AnalogL_Down,true,BTN_DOWN);
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogL_Down_Type = !Settings.AnalogL_Down_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_L_LEFT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.AnalogL_Left,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.AnalogL_Left,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.AnalogL_Left,true,BTN_LEFT);
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogL_Left_Type = !Settings.AnalogL_Left_Type;
						}
						break;
						case SETTING_CONTROLS_ANALOG_L_RIGHT:
						if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0, CTRL_LSTICK))
						{
							Input_MapButton(&Settings.AnalogL_Right,false,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Input_MapButton(&Settings.AnalogL_Right,true,NULL);
							if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
							{
								sys_timer_usleep(SETTINGS_DELAY);
							}
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Input_MapButton(&Settings.AnalogL_Right,true,BTN_RIGHT);
						}
						if(CellInput->WasButtonPressed(0, CTRL_SELECT))
						{
							Settings.AnalogL_Right_Type = !Settings.AnalogL_Right_Type;
						}
						break;
						case SETTING_CONTROLS_DEFAULT_ALL:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS) | CellInput->IsButtonPressed(0, CTRL_START))
						{
							Emulator_Implementation_ButtonMappingSettings(MAP_BUTTONS_OPTION_DEFAULT);
						}
							break;
				default:
					break;
			} // end of switch
	}

	float yPos = 0.09;
	float ySpacing = 0.04;

	cellDbgFontPuts		(0.09f,	0.05f,	1.00f,	GREEN,	"GENERAL");
	cellDbgFontPuts		(0.27f,	0.05f,	1.00f,	GREEN,	"FCEU");
	cellDbgFontPuts		(0.45f,	0.05f,	1.00f,	GREEN,	"PATH");
	cellDbgFontPuts		(0.63f, 0.05f,	1.00f, RED,	"CONTROLS"); 
	Graphics->FlushDbgFont();

//PAGE 1
if((currently_selected_controller_setting-FIRST_CONTROLS_SETTING) < NUM_ENTRY_PER_SETTINGS_PAGE)
{
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_DPAD_UP ? YELLOW : WHITE,	"D-Pad Up");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.DPad_Up == BTN_UP ? GREEN : ORANGE, Input_PrintMappedButton(Settings.DPad_Up));

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_DPAD_DOWN ? YELLOW : WHITE,	"D-Pad Down");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.DPad_Down == BTN_DOWN ? GREEN : ORANGE, Input_PrintMappedButton(Settings.DPad_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_DPAD_LEFT ? YELLOW : WHITE,	"D-Pad Left");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.DPad_Left == BTN_LEFT ? GREEN : ORANGE, Input_PrintMappedButton(Settings.DPad_Left));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_DPAD_RIGHT ? YELLOW : WHITE,	"D-Pad Right");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.DPad_Right == BTN_RIGHT ? GREEN : ORANGE, Input_PrintMappedButton(Settings.DPad_Right));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_CIRCLE ? YELLOW : WHITE,	"Circle button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonCircle == BTN_A ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonCircle));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_CROSS ? YELLOW : WHITE,	"Cross button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonCross == BTN_B ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonCross));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_TRIANGLE ? YELLOW : WHITE,	"Triangle button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonTriangle == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonTriangle));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_SQUARE ? YELLOW : WHITE,	"Square button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonSquare == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonSquare));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_SELECT ? YELLOW : WHITE,	"Select button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonSelect == BTN_SELECT ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonSelect));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_START ? YELLOW : WHITE,	"Start button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonStart == BTN_START ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonStart));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L1 ? YELLOW : WHITE,	"L1 button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL1 == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL1));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R1 ? YELLOW : WHITE,	"R1 button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR1 == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR1));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2 ? YELLOW : WHITE,	"L2 button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL2 == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL2));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2 ? YELLOW : WHITE,	"R2 button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR2 == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR2));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L3 ? YELLOW : WHITE,	"L3 button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL3 == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R3 ? YELLOW : WHITE,	"R3 button");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR3 == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_BUTTON_L3 ? YELLOW : WHITE,	"Button combo: L2 & L3");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL2_ButtonL3 == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL2_ButtonL3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_BUTTON_R2 ? YELLOW : WHITE,	"Button combo: L2 & R2");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL2_ButtonR2 == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL2_ButtonR2));
	Graphics->FlushDbgFont();

}

//PAGE 2
if((currently_selected_controller_setting >= SETTING_CONTROLS_BUTTON_L2_BUTTON_R3) && (currently_selected_controller_setting < SETTING_CONTROLS_ANALOG_L_RIGHT))
{
	yPos = 0.09;

	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_BUTTON_R3 ? YELLOW : WHITE,	"Button combo: L2 & R3");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL2_ButtonR3 == BTN_QUICKLOAD ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL2_ButtonR3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_ANALOG_R_RIGHT ? YELLOW : WHITE,	"Button combo: L2 & RStick Right");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL2_AnalogR_Right == BTN_INCREMENTCHEAT ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL2_AnalogR_Right));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_ANALOG_R_LEFT ? YELLOW : WHITE,	"Button combo: L2 & RStick Left");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL2_AnalogR_Left == BTN_DECREMENTCHEAT ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL2_AnalogR_Left));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_ANALOG_R_UP ? YELLOW : WHITE,	"Button combo: L2 & RStick Up");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL2_AnalogR_Up == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL2_AnalogR_Up));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_L2_ANALOG_R_DOWN ? YELLOW : WHITE,	"Button combo: L2 & RStick Down");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonL2_AnalogR_Down == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonL2_AnalogR_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_ANALOG_R_RIGHT ? YELLOW : WHITE,	"Button combo: R2 & RStick Right");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR2_AnalogR_Right == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR2_AnalogR_Right));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_ANALOG_R_LEFT ? YELLOW : WHITE,	"Button combo: R2 & RStick Left");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR2_AnalogR_Left == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR2_AnalogR_Left));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_ANALOG_R_UP ? YELLOW : WHITE,	"Button combo: R2 & RStick Up");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR2_AnalogR_Up == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR2_AnalogR_Up));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_ANALOG_R_DOWN ? YELLOW : WHITE,	"Button combo: R2 & RStick Down");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR2_AnalogR_Down == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR2_AnalogR_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R2_BUTTON_R3 ? YELLOW : WHITE,	"Button combo: R2 & R3");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR2_ButtonR3 == BTN_QUICKSAVE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR2_ButtonR3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_BUTTON_R3_BUTTON_L3 ? YELLOW : WHITE,	"Button combo: R3 & L3");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.ButtonR3_ButtonL3 == BTN_EXITTOMENU ? GREEN : ORANGE, Input_PrintMappedButton(Settings.ButtonR3_ButtonL3));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_R_UP ? YELLOW : WHITE,	"Right Stick - Up %s", Settings.AnalogR_Up_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.AnalogR_Up == BTN_CHEATENABLE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.AnalogR_Up));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_R_DOWN ? YELLOW : WHITE,	"Right Stick - Down %s", Settings.AnalogR_Down_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.AnalogR_Down == BTN_NONE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.AnalogR_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_R_LEFT ? YELLOW : WHITE,	"Right Stick - Left %s", Settings.AnalogR_Left_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.AnalogR_Left == BTN_DECREMENTSAVE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.AnalogR_Left));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_R_RIGHT ? YELLOW : WHITE,	"Right Stick - Right %s", Settings.AnalogR_Right_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.AnalogR_Right == BTN_INCREMENTSAVE ? GREEN : ORANGE, Input_PrintMappedButton(Settings.AnalogR_Right));
	Graphics->FlushDbgFont();
//Left analog stick
	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_L_UP ? YELLOW : WHITE,	"Left Stick - Up %s", Settings.AnalogL_Up_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.AnalogL_Up == BTN_UP ? GREEN : ORANGE, Input_PrintMappedButton(Settings.AnalogL_Up));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_L_DOWN ? YELLOW : WHITE,	"Left Stick - Down %s", Settings.AnalogL_Down_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.AnalogL_Down == BTN_DOWN ? GREEN : ORANGE, Input_PrintMappedButton(Settings.AnalogL_Down));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_L_LEFT ? YELLOW : WHITE,	"Left Stick - Left %s", Settings.AnalogL_Left_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.AnalogL_Left == BTN_LEFT ? GREEN : ORANGE, Input_PrintMappedButton(Settings.AnalogL_Left));
	Graphics->FlushDbgFont();

}

//PAGE 3
if(currently_selected_controller_setting >= SETTING_CONTROLS_ANALOG_L_RIGHT)
{
	yPos = 0.09;

	cellDbgFontPrintf		(0.09f,	yPos,	1.00f,	currently_selected_controller_setting == SETTING_CONTROLS_ANALOG_L_RIGHT ? YELLOW : WHITE,	"Left Stick - Right %s", Settings.AnalogL_Right_Type ? "(IsPressed)" : "(WasPressed)");
	cellDbgFontPuts		(0.5f,	yPos,	1.00f,	Settings.AnalogL_Right == BTN_RIGHT ? GREEN : ORANGE, Input_PrintMappedButton(Settings.AnalogL_Right));
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, 1.00f, currently_selected_controller_setting == SETTING_CONTROLS_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");
}

	DisplayHelpMessage(currently_selected_controller_setting);

	cellDbgFontPuts(0.09f, 0.89f, 1.00f, YELLOW,
	"UP/DOWN - select  L2+R2 - resume game   X/LEFT/RIGHT - change");
	cellDbgFontPuts(0.09f, 0.93f, 1.00f, YELLOW,
	"START - default   L1/CIRCLE - go back");
	Graphics->FlushDbgFont();
}

void do_path_settings()
{
	if(CellInput->UpdateDevice(0) == CELL_OK)
	{
			// back to ROM menu if CIRCLE is pressed
			if (CellInput->WasButtonPressed(0, CTRL_L1) | CellInput->WasButtonPressed(0, CTRL_CIRCLE))
			{
				menuStack.pop();
				return;
			}

			if (CellInput->WasButtonPressed(0, CTRL_R1))
			{
				menuStack.push(do_controls_settings);
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
			{
				currently_selected_path_setting++;
				if (currently_selected_path_setting >= MAX_NO_OF_PATH_SETTINGS)
				{
					currently_selected_path_setting = FIRST_PATH_SETTING;
				}

				if(CellInput->IsButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))
				{
					sys_timer_usleep(SETTINGS_DELAY);
				}
			}

			if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
			{
				// if a rom is loaded then resume it
				if (Emulator_IsROMLoaded())
				{
					MenuStop();
					Emulator_StartROMRunning();
				}
				return;
			}

			if (CellInput->WasButtonHeld(0, CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
			{
					currently_selected_path_setting--;
					if (currently_selected_path_setting < FIRST_PATH_SETTING)
					{
						currently_selected_path_setting = MAX_NO_OF_PATH_SETTINGS-1;
					}
					if(CellInput->IsButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
			}
					switch(currently_selected_path_setting)
					{
						case SETTING_PATH_DEFAULT_ROM_DIRECTORY:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							tmpBrowser = NULL;
							menuStack.push(do_pathChoice);
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.PS3PathROMDirectory = "/";
						}
							break;
						case SETTING_PATH_SAVESTATES_DIRECTORY:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							tmpBrowser = NULL;
							menuStack.push(do_pathChoice);
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.PS3PathSaveStates = USRDIR;
						}
							break;
						case SETTING_PATH_SRAM_DIRECTORY:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							tmpBrowser = NULL;
							menuStack.push(do_pathChoice);
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.PS3PathSRAM = USRDIR;
						}
							break;
						case SETTING_PATH_CHEATS:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							tmpBrowser = NULL;
							menuStack.push(do_pathChoice);
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.PS3PathCheats = USRDIR;
						}
							break;
						case SETTING_PATH_BASE_DIRECTORY:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							tmpBrowser = NULL;
							menuStack.push(do_pathChoice);
						}
						if(CellInput->IsButtonPressed(0, CTRL_START))
						{
							Settings.PS3PathBaseDirectory = USRDIR;
						}
							break;
						case SETTING_PATH_DEFAULT_ALL:
						if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0, CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_CROSS))
						{
							Settings.PS3PathROMDirectory = "/";
							Settings.PS3PathSaveStates = USRDIR;
							Settings.PS3PathSRAM = USRDIR;
							Settings.PS3PathCheats = USRDIR;
							Settings.PS3PathBaseDirectory = USRDIR;
						}
							break;
					break;
				default:
					break;
			} // end of switch
	}

	float yPos = 0.09;
	float ySpacing = 0.04;

	cellDbgFontPuts		(0.09f,	0.05f,	1.00f,	GREEN,	"GENERAL");
	cellDbgFontPuts		(0.27f,	0.05f,	1.00f,	GREEN,	"FCEU");
	cellDbgFontPuts		(0.45f,	0.05f,	1.00f,	RED,	"PATH");
	cellDbgFontPuts		(0.63f, 0.05f,	1.00f, GREEN,	"CONTROLS"); 
	Graphics->FlushDbgFont();

	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_path_setting == SETTING_PATH_DEFAULT_ROM_DIRECTORY ? YELLOW : WHITE,	"Startup ROM Directory");
	cellDbgFontPuts		(0.5f,	yPos,	FONT_SIZE,	!(strcmp(Settings.PS3PathROMDirectory.c_str(),"/")) ? GREEN : ORANGE, Settings.PS3PathROMDirectory.c_str());

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_path_setting == SETTING_PATH_SAVESTATES_DIRECTORY ? YELLOW : WHITE,	"Savestate Directory");
	cellDbgFontPuts		(0.5f,	yPos,	FONT_SIZE,	!(strcmp(Settings.PS3PathSaveStates.c_str(),USRDIR)) ? GREEN : ORANGE, Settings.PS3PathSaveStates.c_str());
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_path_setting == SETTING_PATH_SRAM_DIRECTORY ? YELLOW : WHITE,	"SRAM directory");
	cellDbgFontPuts		(0.5f,	yPos,	FONT_SIZE,	!(strcmp(Settings.PS3PathSRAM.c_str(),USRDIR)) ? GREEN : ORANGE, Settings.PS3PathSRAM.c_str());

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_path_setting == SETTING_PATH_CHEATS ? YELLOW : WHITE,	"Cheats directory");
	cellDbgFontPuts		(0.5f,	yPos,	FONT_SIZE,	!(strcmp(Settings.PS3PathCheats.c_str(),USRDIR)) ? GREEN : ORANGE, Settings.PS3PathCheats.c_str());

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_path_setting == SETTING_PATH_BASE_DIRECTORY ? YELLOW : WHITE,	"Base directory");
	cellDbgFontPuts		(0.5f,	yPos,	FONT_SIZE,	!(strcmp(Settings.PS3PathBaseDirectory.c_str(),USRDIR)) ? GREEN : ORANGE, Settings.PS3PathBaseDirectory.c_str());

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, FONT_SIZE, currently_selected_path_setting == SETTING_PATH_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");

	DisplayHelpMessage(currently_selected_path_setting);

	cellDbgFontPuts(0.09f, 0.89f, 1.00f, YELLOW,
	"UP/DOWN - select  L2+R2 - resume game   X/LEFT/RIGHT - change");
	cellDbgFontPuts(0.09f, 0.93f, 1.00f, YELLOW,
	"START - default   L1/CIRCLE - go back   R1 - go forward");
	Graphics->FlushDbgFont();
}

void do_fceu_settings()
{
	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		// back to ROM menu if CIRCLE is pressed
		if (CellInput->WasButtonPressed(0, CTRL_L1) | CellInput->WasButtonPressed(0, CTRL_CIRCLE))
		{
			menuStack.pop();
			return;
		}
		
		if (CellInput->WasButtonPressed(0, CTRL_R1))
		{
			menuStack.push(do_path_settings);
			return;
		}

		if (CellInput->WasButtonHeld(0, CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
		{
			currently_selected_fceu_setting++;
			if (currently_selected_fceu_setting >= MAX_NO_OF_FCEU_SETTINGS)
			{
				currently_selected_fceu_setting = FIRST_FCEU_SETTING;
			}
			if(CellInput->IsButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))
			{
				sys_timer_usleep(SETTINGS_DELAY);
			}
		}

		if (CellInput->WasButtonHeld(0, CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
		{
				currently_selected_fceu_setting--;
				if (currently_selected_fceu_setting < FIRST_FCEU_SETTING)
				{
					currently_selected_fceu_setting = MAX_NO_OF_FCEU_SETTINGS-1;
				}
				if(CellInput->IsButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))
				{
					sys_timer_usleep(SETTINGS_DELAY);
				}
		}

		if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
		{
			// if a rom is loaded then resume it
			if (Emulator_IsROMLoaded())
			{
				MenuStop();

				Emulator_StartROMRunning();
			}

			return;
		}

		switch(currently_selected_fceu_setting)
		{
			case SETTING_FCEU_DISABLE_SPRITE_LIMIT:
				if(CellInput->WasButtonPressed(0, CTRL_LEFT) || CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) || CellInput->WasButtonPressed(0, CTRL_RIGHT) || CellInput->WasAnalogPressedRight(0,CTRL_LSTICK))
				{
					Settings.FCEUDisableSpriteLimitation = !Settings.FCEUDisableSpriteLimitation;
					FCEUI_DisableSpriteLimitation(Settings.FCEUDisableSpriteLimitation);
				}
				if(CellInput->IsButtonPressed(0, CTRL_START))
				{
					Settings.FCEUDisableSpriteLimitation = false;
					FCEUI_DisableSpriteLimitation(Settings.FCEUDisableSpriteLimitation);
				}
				break;
			case SETTING_FCEU_GAME_GENIE:
				if(CellInput->WasButtonPressed(0, CTRL_LEFT) || CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) || CellInput->WasButtonPressed(0, CTRL_RIGHT) || CellInput->WasAnalogPressedRight(0,CTRL_LSTICK))
				{
					Settings.FCEUGameGenie = !Settings.FCEUGameGenie;
					FCEUI_SetGameGenie(Settings.FCEUGameGenie);
				}
				if(CellInput->IsButtonPressed(0, CTRL_START))
				{
					Settings.FCEUGameGenie = false;
					FCEUI_SetGameGenie(Settings.FCEUGameGenie);
				}
				break;
			case SETTING_FCEU_CONTROL_STYLE:
				if(CellInput->WasButtonPressed(0, CTRL_LEFT) || CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) || CellInput->WasButtonPressed(0, CTRL_RIGHT) || CellInput->WasAnalogPressedRight(0,CTRL_LSTICK))
				{
					control_style = (ControlStyle)(((control_style) + 1) % 2);
					Settings.FCEUControlstyle = control_style;
					if(control_style == CONTROL_STYLE_ORIGINAL)
					{
						Settings.ButtonCircle = BTN_A;
						Settings.ButtonCross = BTN_B;
					}
					else
					{
						Settings.ButtonSquare = BTN_B;
						Settings.ButtonCross = BTN_A;
					}
				}
				if(CellInput->IsButtonPressed(0, CTRL_START))
				{
					control_style = CONTROL_STYLE_ORIGINAL;
					Settings.FCEUControlstyle = control_style;
					Settings.ButtonCircle = BTN_A;
					Settings.ButtonCross = BTN_B;
				}
				
				break;
			case SETTING_FCEU_DEFAULT_ALL:
				if(CellInput->WasButtonPressed(0, CTRL_LEFT) || CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) || CellInput->WasButtonPressed(0, CTRL_RIGHT) || CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->IsButtonPressed(0, CTRL_START))
				{
					Settings.FCEUDisableSpriteLimitation = false;
					Settings.FCEUGameGenie = false;
					control_style = CONTROL_STYLE_ORIGINAL;
					Settings.FCEUControlstyle = CONTROL_STYLE_ORIGINAL;
					FCEUI_DisableSpriteLimitation(Settings.FCEUDisableSpriteLimitation);
					FCEUI_SetGameGenie(Settings.FCEUGameGenie);
				}
				break;
			default:
				break;
		} // end of switch

	}

	cellDbgFontPuts		(0.09f,	0.05f,	1.00f,	GREEN,	"GENERAL");
	cellDbgFontPuts		(0.27f,	0.05f,	1.00f,	RED,	"FCEU");
	cellDbgFontPuts		(0.45f,	0.05f,	1.00f,	GREEN,	"PATH");
	cellDbgFontPuts		(0.63f, 0.05f,	1.00f, GREEN,	"CONTROLS"); 

	float yPos = 0.09;
	float ySpacing = 0.04;

	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_fceu_setting == SETTING_FCEU_DISABLE_SPRITE_LIMIT ? YELLOW : WHITE,	"Sprite Limit:");
	cellDbgFontPrintf	(0.5f,	yPos,	FONT_SIZE,	Settings.FCEUDisableSpriteLimitation == true ? ORANGE : GREEN, Settings.FCEUDisableSpriteLimitation == true ? "OFF" : "ON");

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_fceu_setting == SETTING_FCEU_GAME_GENIE ? YELLOW : WHITE,	"Game Genie:");
	cellDbgFontPrintf	(0.5f,	yPos,	FONT_SIZE,	Settings.FCEUGameGenie == true ? ORANGE : GREEN, Settings.FCEUGameGenie == true ? "ON" : "OFF");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FONT_SIZE, currently_selected_fceu_setting == SETTING_FCEU_CONTROL_STYLE ? YELLOW : WHITE, "Control Style");
	cellDbgFontPrintf(0.5f, yPos, FONT_SIZE,
			control_style == CONTROL_STYLE_ORIGINAL ? GREEN : ORANGE,
			"%s", control_style == CONTROL_STYLE_ORIGINAL ? "Original (X->B, O->A)" : "Better (X->A, []->B)");
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, FONT_SIZE, currently_selected_fceu_setting == SETTING_FCEU_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");

	DisplayHelpMessage(currently_selected_fceu_setting);

	cellDbgFontPuts(0.09f, 0.89f, 1.00f, YELLOW,
	"UP/DOWN - select  L2+R2 - resume game   X/LEFT/RIGHT - change");
	cellDbgFontPuts(0.09f, 0.93f, 1.00f, YELLOW,
	"START - default   L1/CIRCLE - go back   R1 - go forward");
	Graphics->FlushDbgFont();
}

// void do_settings()
// // called from ROM menu by pressing the SELECT button
// // return to ROM menu by pressing the CIRCLE button
void do_general_settings()
{
	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		// back to ROM menu if CIRCLE is pressed
		if (CellInput->WasButtonPressed(0, CTRL_CIRCLE) || CellInput->WasButtonPressed(0, CTRL_L1))
		{
			menuStack.pop();
			return;
		}

		if (CellInput->WasButtonPressed(0, CTRL_R1))
		{
			menuStack.push(do_fceu_settings);
			return;
		}

		if (CellInput->WasButtonHeld(0, CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))	// down to next setting
		{
			currently_selected_setting++;
			if (currently_selected_setting >= MAX_NO_OF_SETTINGS)
			{
				currently_selected_setting = FIRST_GENERAL_SETTING;
			}
			if(CellInput->IsButtonPressed(0,CTRL_DOWN) | CellInput->IsAnalogPressedDown(0, CTRL_LSTICK))
			{
				sys_timer_usleep(SETTINGS_DELAY);
			}
			
		}

		if (CellInput->WasButtonHeld(0, CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))	// up to previous setting
		{
				currently_selected_setting--;
				if (currently_selected_setting < FIRST_GENERAL_SETTING)
				{
					currently_selected_setting = MAX_NO_OF_SETTINGS-1;
				}
				if(CellInput->IsButtonPressed(0,CTRL_UP) | CellInput->IsAnalogPressedUp(0, CTRL_LSTICK))
				{
					sys_timer_usleep(SETTINGS_DELAY);
				}

		}

		if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
		{
			// if a rom is loaded then resume it
			if (Emulator_IsROMLoaded())
			{
				MenuStop();

				Emulator_StartROMRunning();
			}

			return;
		}

		switch(currently_selected_setting)
		{
			case SETTING_CURRENT_SAVE_STATE_SLOT:
				if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
				{
					Emulator_DecrementCurrentSaveStateSlot();
					if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
				}
				if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
				{
					Emulator_IncrementCurrentSaveStateSlot();
					if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
				}
				break;
			case SETTING_CHANGE_RESOLUTION:
				if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
				{
					Graphics->NextResolution();
					if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
				}
				if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
				{
					Graphics->PreviousResolution();
					if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
					{
						sys_timer_usleep(SETTINGS_DELAY);
					}
				}
				if(CellInput->WasButtonPressed(0, CTRL_CROSS))
				{
					if (Graphics->GetCurrentResolution() == CELL_VIDEO_OUT_RESOLUTION_576)
					{
						if(Graphics->CheckResolution(CELL_VIDEO_OUT_RESOLUTION_576))
						{
							Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
							Graphics->SwitchResolution(Graphics->GetCurrentResolution(), Settings.PS3PALTemporalMode60Hz);
						}
					}
					else
					{
						Graphics->SetPAL60Hz(false);
						Graphics->SwitchResolution(Graphics->GetCurrentResolution(), false);
					}
				}
				if(CellInput->IsButtonPressed(0, CTRL_START))
				{
					Graphics->SwitchResolution(Graphics->GetInitialResolution(), Settings.PS3PALTemporalMode60Hz);
				}
			   break;
			case SETTING_PAL60_MODE:
			   if(CellInput->WasButtonHeld(0, CTRL_RIGHT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonHeld(0,CTRL_CROSS) | CellInput->IsButtonPressed(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
			   {
				   if (Graphics->GetCurrentResolution() == CELL_VIDEO_OUT_RESOLUTION_576)
				   {
					   if(Graphics->CheckResolution(CELL_VIDEO_OUT_RESOLUTION_576))
					   {
						   Settings.PS3PALTemporalMode60Hz = !Settings.PS3PALTemporalMode60Hz;
						   Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
						   Graphics->SwitchResolution(Graphics->GetCurrentResolution(), Settings.PS3PALTemporalMode60Hz);
					   }
				   }
			   }
			   break;
			case SETTING_KEEP_ASPECT_RATIO:
				if(CellInput->WasButtonPressed(0, CTRL_LEFT) || CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) || CellInput->WasButtonPressed(0, CTRL_RIGHT) || CellInput->WasAnalogPressedRight(0,CTRL_LSTICK))
				{
					Settings.PS3KeepAspect = !Settings.PS3KeepAspect;
					Graphics->SetAspectRatio(Settings.PS3KeepAspect ? SCREEN_4_3_ASPECT_RATIO : SCREEN_16_9_ASPECT_RATIO);
				}
				if(CellInput->IsButtonPressed(0, CTRL_START))
				{
					Settings.PS3KeepAspect = true;
					Graphics->SetAspectRatio(SCREEN_4_3_ASPECT_RATIO);
				}
				break;
			case SETTING_HW_TEXTURE_FILTER:
				if(CellInput->WasButtonPressed(0, CTRL_LEFT) || CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) || CellInput->WasButtonPressed(0, CTRL_RIGHT) || CellInput->WasAnalogPressedRight(0,CTRL_LSTICK))
				{
					Settings.PS3Smooth = !Settings.PS3Smooth;
					Graphics->SetSmooth(Settings.PS3Smooth);
				}
				if(CellInput->IsButtonPressed(0, CTRL_START))
				{
					Settings.PS3Smooth = true;
					Graphics->SetSmooth(true);
				}
				break;
			case SETTING_HW_OVERSCAN_AMOUNT:
				if(CellInput->WasButtonHeld(0, CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
				{
					if(Settings.PS3OverscanAmount > -40)
					{
						Settings.PS3OverscanAmount--;
						Settings.PS3OverscanEnabled = true;
						Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
						{
							sys_timer_usleep(SETTINGS_DELAY);
						}
					}
					if(Settings.PS3OverscanAmount == 0)
					{
						Settings.PS3OverscanEnabled = false;
						Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						if(CellInput->IsButtonPressed(0,CTRL_LEFT) | CellInput->IsAnalogPressedLeft(0,CTRL_LSTICK))
						{
							sys_timer_usleep(SETTINGS_DELAY);
						}
					}
				}
				if(CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
				{
					if((Settings.PS3OverscanAmount < 40))
					{
						Settings.PS3OverscanAmount++;
						Settings.PS3OverscanEnabled = true;
						Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
						{
							sys_timer_usleep(SETTINGS_DELAY);
						}
					}
					if(Settings.PS3OverscanAmount == 0)
					{
						Settings.PS3OverscanEnabled = false;
						Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
						if(CellInput->IsButtonPressed(0,CTRL_RIGHT) | CellInput->IsAnalogPressedRight(0,CTRL_LSTICK))
						{
							sys_timer_usleep(SETTINGS_DELAY);
						}
					}
				}
				if(CellInput->IsButtonPressed(0, CTRL_START))
				{
					Settings.PS3OverscanAmount = 0;
					Settings.PS3OverscanEnabled = false;
					Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
				}
				break;
				case SETTING_RSOUND_ENABLED:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0,CTRL_CROSS))
					{
						Settings.RSoundEnabled = !Settings.RSoundEnabled;
						Emulator_ToggleSound();
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.RSoundEnabled = false;
						Emulator_ToggleSound();
					}
					break;
				case SETTING_RSOUND_SERVER_IP_ADDRESS:
					if(CellInput->WasButtonPressed(0, CTRL_LEFT) | CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) | CellInput->WasButtonPressed(0, CTRL_RIGHT) | CellInput->WasButtonPressed(0, CTRL_CROSS) | CellInput->WasAnalogPressedRight(0,CTRL_LSTICK))
					{
						Emulator_OSKStart(L"Enter the IP address for the RSound Server. Example (below):", L"192.168.1.1");
						Settings.RSoundServerIPAddress = Emulator_OSKOutputString();
					}
					if(CellInput->IsButtonPressed(0, CTRL_START))
					{
						Settings.RSoundServerIPAddress = "0.0.0.0";
					}
					break;
			case SETTING_SHADER:
				if (CellInput->WasButtonPressed(0, CTRL_CROSS))
				{
					tmpBrowser = NULL;
					menuStack.push(do_shaderChoice);
				}
				break;
			case SETTING_DEFAULT_ALL:
				if(CellInput->WasButtonPressed(0, CTRL_LEFT) || CellInput->WasAnalogPressedLeft(0,CTRL_LSTICK) || CellInput->WasButtonPressed(0, CTRL_RIGHT) || CellInput->WasAnalogPressedRight(0,CTRL_LSTICK) | CellInput->IsButtonPressed(0, CTRL_START))
				{
					Settings.PS3KeepAspect = true;
					Settings.PS3Smooth = true;
					Settings.PS3OverscanAmount = 0;
					Settings.PS3OverscanEnabled = false;
					Graphics->SetAspectRatio(SCREEN_4_3_ASPECT_RATIO);
					Graphics->SetSmooth(Settings.PS3Smooth);
					Graphics->SetOverscan(Settings.PS3OverscanEnabled, (float)Settings.PS3OverscanAmount/100);
					Settings.ButtonCircle = BTN_A;
					Settings.ButtonCross = BTN_B;
					Settings.PS3PALTemporalMode60Hz = false;
					Graphics->SetPAL60Hz(Settings.PS3PALTemporalMode60Hz);
				}
				break;
			default:
				break;
		} // end of switch

	}

	cellDbgFontPuts		(0.09f,	0.05f,	1.00f,	RED,	"GENERAL");
	cellDbgFontPuts		(0.27f,	0.05f,	1.00f,	GREEN,	"FCEU");
	cellDbgFontPuts		(0.45f,	0.05f,	1.00f,	GREEN,	"PATH");
	cellDbgFontPuts		(0.63f, 0.05f,	1.00f, GREEN,	"CONTROLS"); 

	float yPos = 0.09;
	float ySpacing = 0.04;

	cellDbgFontPuts(0.09f, yPos, FONT_SIZE, currently_selected_setting == SETTING_CURRENT_SAVE_STATE_SLOT ? YELLOW : WHITE, "Current save state slot");
	cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Emulator_CurrentSaveStateSlot() == 0 ? GREEN : ORANGE, "%d", Emulator_CurrentSaveStateSlot());

	yPos += ySpacing;

	cellDbgFontPuts(0.09f, yPos, FONT_SIZE, currently_selected_setting == SETTING_CHANGE_RESOLUTION ? YELLOW : WHITE, "Resolution");

	switch(Graphics->GetCurrentResolution())
	{
		case CELL_VIDEO_OUT_RESOLUTION_480:
			cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_480 ? GREEN : ORANGE, "720x480 (480p)");
			Graphics->FlushDbgFont();
			break;
		case CELL_VIDEO_OUT_RESOLUTION_720:
			cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_720 ? GREEN : ORANGE, "1280x720 (720p)");
			Graphics->FlushDbgFont();
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1080:
			cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1080 ? GREEN : ORANGE, "1920x1080 (1080p)");
			Graphics->FlushDbgFont();
			break;
		case CELL_VIDEO_OUT_RESOLUTION_576:
			cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_576 ? GREEN : ORANGE, "720x576 (576p)");
			Graphics->FlushDbgFont();
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1600x1080:
			cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1600x1080 ? GREEN : ORANGE, "1600x1080");
			Graphics->FlushDbgFont();
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1440x1080:
			cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1440x1080 ? GREEN : ORANGE, "1440x1080");
			Graphics->FlushDbgFont();
			break;
		case CELL_VIDEO_OUT_RESOLUTION_1280x1080:
			cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_1280x1080 ? GREEN : ORANGE, "1280x1080");
			Graphics->FlushDbgFont();
			break;
		case CELL_VIDEO_OUT_RESOLUTION_960x1080:
			cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Graphics->GetInitialResolution() == CELL_VIDEO_OUT_RESOLUTION_960x1080 ? GREEN : ORANGE, "960x1080");
			Graphics->FlushDbgFont();
			break;
	}
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_setting == SETTING_PAL60_MODE ? YELLOW : WHITE,	"PAL60 Mode (576p only)");
	cellDbgFontPrintf	(0.5f,	yPos,	FONT_SIZE,	Settings.PS3PALTemporalMode60Hz == true ? ORANGE : GREEN, Settings.PS3PALTemporalMode60Hz == true ? "ON" : "OFF");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FONT_SIZE, currently_selected_setting == SETTING_SHADER ? YELLOW : WHITE, "Selected Shader: ");
	cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, GREEN, "%s", Graphics->GetFragmentShaderPath().substr(Graphics->GetFragmentShaderPath().find_last_of('/')).c_str());
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FONT_SIZE, currently_selected_setting == SETTING_KEEP_ASPECT_RATIO ? YELLOW : WHITE, "Aspect Ratio");
	cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Settings.PS3KeepAspect == true ? GREEN : ORANGE, "%s", Settings.PS3KeepAspect == true ? "Scaled" : "Stretched");
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FONT_SIZE, currently_selected_setting == SETTING_HW_TEXTURE_FILTER ? YELLOW : WHITE, "Hardware Filtering");
	cellDbgFontPrintf(0.5f, yPos, FONT_SIZE, Settings.PS3Smooth == true ? GREEN : ORANGE, "%s", Settings.PS3Smooth == true ? "Linear interpolation" : "Point filtering");
	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts		(0.09f,	yPos,	FONT_SIZE,	currently_selected_setting == SETTING_HW_OVERSCAN_AMOUNT ? YELLOW : WHITE,	"Overscan");
	cellDbgFontPrintf	(0.5f,	yPos,	FONT_SIZE,	Settings.PS3OverscanAmount == 0 ? GREEN : ORANGE, "%f", (float)Settings.PS3OverscanAmount/100);

	Graphics->FlushDbgFont();

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FONT_SIZE, currently_selected_setting == SETTING_RSOUND_ENABLED ? YELLOW : WHITE, "Sound");
	cellDbgFontPuts(0.5f, yPos, FONT_SIZE, Settings.RSoundEnabled == false ? GREEN : ORANGE, Settings.RSoundEnabled == true ? "RSound" : "Normal");

	yPos += ySpacing;
	cellDbgFontPuts(0.09f, yPos, FONT_SIZE, currently_selected_setting == SETTING_RSOUND_SERVER_IP_ADDRESS ? YELLOW : WHITE, "RSound Server IP Address");
	cellDbgFontPuts(0.5f, yPos, FONT_SIZE, strcmp(Settings.RSoundServerIPAddress,"0.0.0.0") ? ORANGE : GREEN, Settings.RSoundServerIPAddress);
	Graphics->FlushDbgFont();



	yPos += ySpacing;
	cellDbgFontPrintf(0.09f, yPos, FONT_SIZE, currently_selected_setting == SETTING_DEFAULT_ALL ? YELLOW : GREEN, "DEFAULT");
	Graphics->FlushDbgFont();

	DisplayHelpMessage(currently_selected_setting);

	cellDbgFontPuts(0.09f, 0.89f, 1.00f, YELLOW,
	"UP/DOWN - select  L2+R2 - resume game   X/LEFT/RIGHT - change");
	cellDbgFontPuts(0.09f, 0.93f, 1.00f, YELLOW,
	"START - default   L1/CIRCLE - go back   R1 - go forward");

	Graphics->FlushDbgFont();
}


void do_ROMMenu ()
{
	string rom_path;

	if (CellInput->UpdateDevice(0) == CELL_PAD_OK)
	{
		UpdateBrowser(browser);

		if (CellInput->WasButtonPressed(0,CTRL_SELECT))
		{
			menuStack.push(do_general_settings);
		}

		if (CellInput->WasButtonPressed(0,CTRL_CROSS))
		{
			if(browser->IsCurrentADirectory())
			{
				browser->PushDirectory(	browser->GetCurrentDirectoryInfo().dir + "/" + browser->GetCurrentEntry()->d_name,
										CELL_FS_TYPE_REGULAR | CELL_FS_TYPE_DIRECTORY,
										"nes|NES|zip|ZIP");
			}
			else if (browser->IsCurrentAFile())
			{
				//load game (standard controls), go back to main loop
				rom_path = browser->GetCurrentDirectoryInfo().dir + "/" + browser->GetCurrentEntry()->d_name;

				MenuStop();

				// switch emulator to emulate mode
				Emulator_StartROMRunning();

				//FIXME: 1x dirty const char* to char* casts... menu sucks.
				Emulator_RequestLoadROM((char*)rom_path.c_str(), true);

				return;
			}
		}
		if (CellInput->IsButtonPressed(0,CTRL_L2) && CellInput->IsButtonPressed(0,CTRL_R2))
		{
			// if a rom is loaded then resume it
			if (Emulator_IsROMLoaded())
			{
				MenuStop();

				Emulator_StartROMRunning();
			}

			return;
		}
	}

	if(browser->IsCurrentADirectory())
	{
		if(!strcmp(browser->GetCurrentEntry()->d_name,"app_home") || !strcmp(browser->GetCurrentEntry()->d_name,"host_root"))
		{
			cellDbgFontPrintf(0.09f, 0.90f, 0.91f, RED, "%s","WARNING - Do not open this directory, or you might have to restart!");
		}
		else if(!strcmp(browser->GetCurrentEntry()->d_name,".."))
		{
			cellDbgFontPrintf(0.09f, 0.90f, 0.91f, LIGHTBLUE, "%s","INFO - Press X to go back to the previous directory.");
		}
		else
		{
			cellDbgFontPrintf(0.09f, 0.90f, 0.91f, LIGHTBLUE, "%s","INFO - Press X to enter the directory.");
		}
	}
	if(browser->IsCurrentAFile())
	{
		cellDbgFontPrintf(0.09f, 0.90f, 0.91f, LIGHTBLUE, "%s", "INFO - Press X to load the game. ");
	}
	cellDbgFontPuts	(0.09f,	0.05f,	1.00f,	RED,	"FILE BROWSER");
	cellDbgFontPrintf(0.7f, 0.05f, 0.82f, WHITE, "FCEU PS3 v%s", EMULATOR_VERSION);
	cellDbgFontPrintf(0.09f, 0.09f, 1.00f, YELLOW, "PATH: %s", browser->GetCurrentDirectoryInfo().dir.c_str());
	cellDbgFontPuts(0.09f, 0.93f, 1.00f, YELLOW,
	"L2 + R2 - resume game           SELECT - Settings screen");
	Graphics->FlushDbgFont();

	RenderBrowser(browser);
}


void MenuMainLoop()
{
	// create file browser if null
	if (browser == NULL)
	{
		browser = new FileBrowser(Settings.PS3PathROMDirectory.c_str());
		browser->SetEntryWrap(false);
	}


	// FIXME: could always just return to last menu item... don't pop on resume kinda thing
	if (menuStack.empty())
	{
		menuStack.push(do_ROMMenu);
	}

	// menu loop
	menuRunning = true;
	while (!menuStack.empty() && menuRunning)
	{
		Graphics->Clear();

		menuStack.top()();

		Graphics->Swap();

		cellSysutilCheckCallback();

#ifdef EMUDEBUG
		if (CellConsole_IsInitialized())
		{
			cellConsolePoll();
		}
#endif
	}
}



