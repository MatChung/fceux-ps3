/*
 * emulator_fileio.cpp
 *
 *  Created on: Oct 25, 2010
 *      Author: halsafar
 */

#include "fceu/emufile.h"
#include "fceu/types.h"
#include "fceu/fceu.h"
#include "fceu/file.h"
#include "fceu/driver.h"
#include "fceu/fds.h"

#include "emulator_implementation.h"

#include "emulator_zipio.h"

FILE * file;

// the nes rom, 4mb
unsigned char * nesrom = NULL;

//
// LoadFile - Can be used in a threaded scenario so while loading graphics can update
// 			- Probably not needed on PS3.  Loading always seems instant.
//
size_t LoadFile (char * rbuffer, const char *filepath, size_t length, bool silent)
{
	FCEU_PrintError("LoadFile(%p, %c, %d, %b)", rbuffer, filepath, length, silent);

        char zipbuffer[2048];
        size_t size = 0, offset = 0, readsize = 0;
        int retry = 1;
        int device;

        // open the file
        while(retry)
        {
                file = fopen (filepath, "rb");

                if(!file)
                {
        			FCEU_PrintError("1: LoadFile %s failed", filepath);
        			return 0;
                }

                /*if(length > 0 && length <= 2048) // do a partial read (eg: to check file header)
                {
                        size = fread (rbuffer, 1, length, file);
                }
                else // load whole file*/
                {
                        readsize = fread (zipbuffer, 1, 32, file);

                        if(!readsize)
                        {
                                fclose (file);
                    			FCEU_PrintError("2: LoadFile %s failed", filepath);
                    			return 0;
                        }

                        if (IsZipFile (zipbuffer))
                        {
                                size = UnZipBuffer ((unsigned char *)rbuffer); // unzip
                        }
                        else
                        {
                            fseek(file,0,SEEK_END);
                            size = ftell(file);
                            fseek(file,0,SEEK_SET);
							//fseeko(file,0,SEEK_END);
							//size = ftello(file);
							//fseeko(file,0,SEEK_SET);

							while(!feof(file))
							{
									FCEU_printf("Loading...%d/%d", offset, size);
									readsize = fread (rbuffer + offset, 1, 4096, file); // read in next chunk

									if(readsize <= 0)
											break; // reading finished (or failed)

									offset += readsize;
							}
							size = offset;
                        }
                }
                retry = 0;
                fclose (file);
        }

        // go back to checking if devices were inserted/removed
        return size;
}


bool LoadRom(const char* fname, int size)
{
	bool biosError = false;
	bool rom_loaded = false;

	ResetGameLoaded();

	FCEUI_CloseGame();
	GameInfo = new FCEUGI();
	memset(GameInfo, 0, sizeof(FCEUGI));

	GameInfo->filename = strdup(fname);
	GameInfo->archiveCount = 0;

	// Set some default values
#define AUDIO_OUT_RATE (48300) // We want slightly more than 48kHz to make sure we're blocking on audio rather than video.
	GameInfo->soundchan = 2;
	GameInfo->soundrate = AUDIO_OUT_RATE;
	GameInfo->name=0;
	GameInfo->type=GIT_CART;
	GameInfo->vidsys=GIV_USER;
	GameInfo->input[0]=GameInfo->input[1]=SI_UNSET;
	GameInfo->inputfc=SIFC_UNSET;
	GameInfo->cspecial=SIS_NONE;

	//Set internal sound information
	FCEUI_Sound(AUDIO_OUT_RATE);
	FCEUI_SetSoundVolume(100); // 0-100
	FCEUI_SetLowPass(0);

	FCEUFILE * fceufp = new FCEUFILE();
	fceufp->size = size;
	fceufp->filename = fname;
	fceufp->mode = FCEUFILE::READ; // read only
	EMUFILE_MEMFILE *fceumem = new EMUFILE_MEMFILE(nesrom, size);
	fceufp->stream = fceumem;

	rom_loaded = iNESLoad(fname, fceufp, 1);

	if(!rom_loaded)
	{
			rom_loaded = UNIFLoad(fname, fceufp);
	}

	if(!rom_loaded)
	{
			rom_loaded = NSFLoad(fname, fceufp);
	}

	if(!rom_loaded)
	{
	/*		// read FDS BIOS into FDSBIOS - should be 8192 bytes
			if (FDSBIOS[1] == 0)
			{
					size_t biosSize = 0;
					char * tmpbuffer = (char *) memalign(32, 64 * 1024);

					char filepath[1024];

					sprintf (filepath, "%s%s/disksys.rom", pathPrefix[GCSettings.LoadMethod], APPFOLDER);
					biosSize = LoadFile(tmpbuffer, filepath, 0, SILENT);
					if(biosSize == 0 && strlen(appPath) > 0)
					{
							sprintf (filepath, "%s/disksys.rom", appPath);
							biosSize = LoadFile(tmpbuffer, filepath, 0, SILENT);
					}

					if (biosSize == 8192)
					{
							memcpy(FDSBIOS, tmpbuffer, 8192);
					}
					else
					{
							biosError = true;

							if (biosSize > 0)
									ErrorPrompt("FDS BIOS file is invalid!");
							else
									ErrorPrompt("FDS BIOS file not found!");
					}
					free(tmpbuffer);
			}
			if (FDSBIOS[1] != 0)
			{
					rom_loaded = FDSLoad(current_rom, fceufp);
			}*/
	}

	delete fceufp;

	if (rom_loaded)
	{
		GetFileBase(GameInfo->filename);

		FCEU_ResetVidSys();

		if(GameInfo->type!=GIT_NSF)
		{
			if(FSettings.GameGenie)
			{
				OpenGenie();
			}
		}
		PowerNES();

		if(GameInfo->type!=GIT_NSF)
		{
			FCEU_LoadGamePalette();
			FCEU_LoadGameCheats(0);
			FCEUI_DisableAllCheats();
		}

		FCEU_ResetPalette();
		FCEU_ResetMessages();

		return 1;
	}
	else
	{
		delete GameInfo;
		GameInfo = 0;

		if(!biosError)
			FCEU_printf("Invalid game file: %s!",fname);
		rom_loaded = false;
		return 0;
	}
}
