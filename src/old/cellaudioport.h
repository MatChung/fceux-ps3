#ifndef __CELLAUDIOPORT_H
#define __CELLAUDIOPORT_H

#include <stdint.h>

#define AUDIO_BLOCKS 8 // 8 or 16. Guess what we choose? :)
#define AUDIO_CHANNELS 2 // All hail glorious stereo!
#define AUDIO_OUT_RATE (48000.0)

// Specific to application
typedef int32_t audio_input_t;

bool cellAudioPortInit(uint64_t samplerate, uint64_t buffersize);

// Write data to buffer. Make sure that samples < buffersize.
void cellAudioPortWrite(const audio_input_t* buffer, uint64_t samples);

// How many samples can we write to buffer without blocking?
uint64_t cellAudioPortWriteAvail();
void cellAudioPortExit();

#endif
