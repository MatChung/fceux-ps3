#include "emulator_input.h"

char * Input_PrintMappedButton(int mappedbutton)
{
	switch(mappedbutton)
	{
		case BTN_A:
			return "Button A";
			break;
		case BTN_B:
			return "Button B";
			break;
		case BTN_SELECT:
			return "Button Select";
			break;
		case BTN_START:
			return "Button Start";
			break;
		case BTN_LEFT:
			return "D-Pad Left";
			break;
		case BTN_RIGHT:
			return "D-Pad Right";
			break;
		case BTN_UP:
			return "D-Pad Up";
			break;
		case BTN_DOWN:
			return "D-Pad Down";
			break;
		case BTN_QUICKSAVE:
			return "Save State";
			break;
		case BTN_QUICKLOAD:
			return "Load State";
			break;
		case BTN_INCREMENTSAVE:
			return "Increment state position";
			break;
		case BTN_DECREMENTSAVE:
			return "Decrement state position";
			break;
		case BTN_INCREMENTCHEAT:
			return "Increment cheat position";
			break;
		case BTN_DECREMENTCHEAT:
			return "Decrement cheat position";
			break;
		case BTN_CHEATENABLE:
			return "Enable selected cheat";
			break;
		case BTN_CHEATDISABLE:
			return "Disable selected cheat";
			break;
		case BTN_EXITTOMENU:
			return "Exit to menu";
			break;
		case BTN_NONE:
			return "None";
			break;
/*
		case BTN_END_RECORDING_MOVIE:
			return "End recording movie";
			break;
		case BTN_LOAD_MOVIE:
			return "Load movie";
			break;
*/
		case BTN_FASTFORWARD:
			return "Fast forward";
			break;
		case BTN_INCREMENTTURBO:
			return "Increment Fast-forward speed";
			break;
		case BTN_DECREMENTTURBO:
			return "Decrement Fast-forward speed";
			break;	
/*
		case BTN_SWAPJOYPADS:
			return "Swap Joypads";
			break;
*/
		default:
			return "Unknown";
			break;

	}
}

//bool next: true is next, false is previous
int Input_GetAdjacentButtonmap(int buttonmap, bool next)
{
	switch(buttonmap)
	{
		case BTN_UP:
			return next ? BTN_DOWN : BTN_NONE;
			break;
		case BTN_DOWN:
			return next ? BTN_LEFT : BTN_UP;
			break;
		case BTN_LEFT:
			return next ? BTN_RIGHT : BTN_DOWN;
			break;
		case BTN_RIGHT:
			return next ?  BTN_A : BTN_LEFT;
			break;
		case BTN_A:
			return next ? BTN_B : BTN_RIGHT;
			break;
		case BTN_B:
			return next ? BTN_SELECT : BTN_A;
			break;
		case BTN_SELECT:
			return next ? BTN_START : BTN_B;
			break;
		case BTN_START:
			return next ? BTN_QUICKSAVE : BTN_SELECT;
			break;
		case BTN_QUICKSAVE:
			return next ? BTN_QUICKLOAD : BTN_START;
			break;
		case BTN_QUICKLOAD:
			return next ? BTN_INCREMENTCHEAT : BTN_QUICKSAVE;
			break;
		case BTN_INCREMENTCHEAT:
			return next ? BTN_DECREMENTCHEAT : BTN_QUICKLOAD;
			break;
		case BTN_DECREMENTCHEAT:
			return next ? BTN_EXITTOMENU : BTN_INCREMENTCHEAT;
			break;
		case BTN_EXITTOMENU:
			return next ? BTN_CHEATENABLE : BTN_DECREMENTCHEAT;
			break;
		case BTN_CHEATENABLE:
			return next ? BTN_CHEATDISABLE : BTN_EXITTOMENU;
			break;
		case BTN_CHEATDISABLE:
			return next ? BTN_DECREMENTSAVE : BTN_CHEATENABLE;
			break;
		case BTN_DECREMENTSAVE:
			return next ? BTN_INCREMENTSAVE : BTN_CHEATDISABLE;
			break;
		case BTN_INCREMENTSAVE:
			return next ? BTN_FASTFORWARD : BTN_DECREMENTSAVE;
			break;
		case BTN_FASTFORWARD:
			return next ? BTN_INCREMENTTURBO : BTN_INCREMENTSAVE;
			break;
		case BTN_DECREMENTTURBO:
			return next ? BTN_INCREMENTTURBO : BTN_FASTFORWARD;
		case BTN_INCREMENTTURBO:
			return next ? BTN_NONE : BTN_DECREMENTTURBO;
/*
		case BTN_RESET:
			return next ? BTN_SOFTRESET : BTN_INCREMENTTURBO;
			break;
		case BTN_SOFTRESET:
			return next ? BTN_PAUSE : BTN_RESET;
		case BTN_PAUSE:
			return next ? BTN_BEGIN_RECORDING_MOVIE : BTN_SOFTRESET;
			break;
		case BTN_BEGIN_RECORDING_MOVIE:
			return next ? BTN_END_RECORDING_MOVIE : BTN_PAUSE;
			break;
		case BTN_END_RECORDING_MOVIE:
			return next ? BTN_LOAD_MOVIE : BTN_BEGIN_RECORDING_MOVIE;
			break;
		case BTN_LOAD_MOVIE:
			return next ? BTN_SWAPJOYPADS : BTN_END_RECORDING_MOVIE;
			break;	
		case BTN_SWAPJOYPADS:
			return next ? BTN_NONE : BTN_LOAD_MOVIE;
*/
		case BTN_NONE:
			return next ? BTN_UP : BTN_INCREMENTTURBO;
			break;
		default:
			return BTN_NONE;
			break;
	}
}

void Input_MapButton(int* buttonmap, bool next, int defaultbutton)
{
	if(defaultbutton == NULL)
	{
		*buttonmap = Input_GetAdjacentButtonmap(*buttonmap, next);
	}
	else
	{
		*buttonmap = defaultbutton;
	}
	//FIXME: Do something with this, or remove it
	/*
	if(*buttonmap == (BTN_LEFT | BTN_RIGHT | BTN_DOWN | BTN_UP | BTN_A | BTN_B | BTN_START | BTN_SELECT))
	{
	}
	*/
	MenuResetControlStyle();
}
