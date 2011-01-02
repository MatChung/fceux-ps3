/*
 * emulator_zipio.h
 *
 *  Created on: Oct 25, 2010
 *      Author: halsafar
 */

/****************************************************************************
 * FCE Ultra
 * Nintendo Wii/Gamecube Port
 *
 * Tantric September 2008
 *
 * gcunzip.h
 *
 * Unzip routines
 ****************************************************************************/

#ifndef EMULATOR_ZIPIO_H_
#define EMULATOR_ZIPIO_H_



int IsZipFile (char *buffer);
char * GetFirstZipFilename();
size_t UnZipBuffer (unsigned char *outbuffer);
int SzParse(char * filepath);
size_t SzExtractFile(int i, unsigned char *buffer);
void SzClose();


#endif /* EMULATOR_ZIPIO_H_ */
