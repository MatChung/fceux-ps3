/*
 * emulator_fileio.h
 *
 *  Created on: Oct 25, 2010
 *      Author: halsafar
 */

#ifndef EMULATOR_FILEIO_H_
#define EMULATOR_FILEIO_H_

size_t LoadFile (char * rbuffer, const char *filepath, size_t length, bool silent);
bool LoadRom(const char* fname, int size);

extern FILE * file;

#endif /* EMULATOR_FILEIO_H_ */
