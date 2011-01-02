#include <string.h>
#include "menu.h"

enum
{
	BTN_QUICKSAVE = 150,
	BTN_QUICKLOAD,
	BTN_INCREMENTSAVE,
	BTN_DECREMENTSAVE,
	BTN_INCREMENTCHEAT,
	BTN_DECREMENTCHEAT,
	BTN_EXITTOMENU,
	BTN_CHEATENABLE,
	BTN_CHEATDISABLE,
	BTN_LOAD_MOVIE,	
	BTN_SOFTRESET,
	BTN_RESET,
	BTN_PAUSE,
	BTN_BEGIN_RECORDING_MOVIE,
	BTN_END_RECORDING_MOVIE,
	BTN_FASTFORWARD,
	BTN_INCREMENTTURBO,
	BTN_DECREMENTTURBO,
	BTN_SWAPJOYPADS,
	BTN_NONE
};
/* Input bitmasks */
#define BTN_A		1
#define BTN_B		2
#define BTN_UP		0x10
#define BTN_DOWN	0x20
#define BTN_LEFT	0x40
#define BTN_RIGHT	0x80
#define BTN_SELECT	4
#define BTN_START	8

char * Input_PrintMappedButton(int mappedbutton);
int Input_GetAdjacentButtonmap(int buttonmap, bool next);
void Input_MapButton(int* buttonmap, bool next, int defaultbutton);
