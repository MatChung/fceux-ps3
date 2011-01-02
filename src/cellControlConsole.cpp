#include <stdio.h>
#include <stdlib.h>
#include <sys/timer.h>

#include "cellControlConsole.h"
#include <cell/control_console.h>

// is it running
bool console_initialized = false;


bool CellConsole_IsInitialized()
{
	return console_initialized;
}


// Force exit function
CellConsoleInputProcessorResult consoleExitProgram(unsigned int uiConnection,
												   const char *pcInput,
												   void *pvPrivateData,
												   int iContinuation)
{
	cellConsolePrintf(uiConnection, "Bye!\n");
	exit(0);

	// Never reached
	return CELL_CONSOLE_INPUT_PROCESSED;
}

void ControlConsole_DeInit()
{
	// FIXME: it appears there is no need to deinit anything here
}

int ControlConsole_Init()
{
	int32_t ret;

	// E Initialize the console
	cellConsoleInit();

	// For PS3 buids, you can use network or DECI3 to communicate with the
	// console.  If you want to use DECI3, use the deci3_console_terminal
	// sample to communicate with the console.

	// For Linux and Windows builds, network is the only option so it should
	// always be enabled.

#if defined(CONSOLE_USE_NETWORK) || !defined(__PPU__)
	// The control console code offers a convenience function to initialize
	// the network.  This basically initializes winsock on a windows build,
	// or initializes the network for a PPU build
	ret = cellConsoleNetworkInitialize();
	if (ret != CELL_CONSOLE_OK)
	{
		return ret;
	}

	// E Now make the console listen on the network for requests.
	ret = cellConsoleNetworkServerInit(9001);
	if (ret != CELL_CONSOLE_OK)
	{
		return ret;
	}
#endif // CONSOLE_USE_NETWORK

	// E For the PS3, we also want to listen in using DECI3
#ifdef __PPU__
	ret = cellConsoleDeci3ServerInit();
	if (ret != CELL_CONSOLE_OK)
	{
		return ret;
	}
#endif

	// add force exit function
	cellConsoleInputProcessorAdd("exit", "Exits your application", "", 0,
								 consoleExitProgram);

	console_initialized = true;

	return 0;
}

