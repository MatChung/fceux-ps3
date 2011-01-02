/******************************************************************************* 
 *  -- Cellframework -  Open framework to abstract the common tasks related to
 *                      PS3 application development.
 *
 *  Copyright (C) 2010
 *       Hans-Kristian Arntzen
 *       Stephen A. Damm
 *       Daniel De Matteis
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ********************************************************************************/





/*
 * OSKUtil.cpp
 *
 *  Created on: Oct 31, 2010
 *      Author: Squarepusher2
 */

#include "OSKUtil.h"

#include "sysutil/sysutil_oskdialog.h"
#include "sys/memory.h"
#include <assert.h>

/* mFlags */
#define OSK_IN_USE	(0x00000001)

OSKUtil::OSKUtil()
{
	mFlags = 0;
	memset(result_text_buffer, 0, sizeof(*result_text_buffer));
	memset(result_text_buffer_char, 0, 256);
	m_oskdialog_memorycontainer =  1024*1024*7;
}

OSKUtil::OSKUtil(std::string& msg, std::string& init)
{
	m_msg = msg;
	m_init = init;
	mFlags = 0;
	m_oskdialog_memorycontainer = 1024*1024*7;
}

//Overloaded function - you can specify the size of the memory container with this
//instead of the default 7MB
OSKUtil::OSKUtil(std::string& msg, std::string& init, unsigned int memorycontainer)
{
	m_msg = msg;
	m_init = init;
	mFlags = 0;
	m_oskdialog_memorycontainer = memorycontainer;
}


OSKUtil::~OSKUtil()
{
}

void OSKUtil::str_to_utf16(uint16_t*& buf, const std::string& out) {}

bool OSKUtil::Start(const wchar_t* msg, const wchar_t* init)
{
	int ret;
	if (mFlags & OSK_IN_USE)
	{
		return (true);
	}
	ret = sys_memory_container_create(&containerid, m_oskdialog_memorycontainer);
	if(ret < 0)
	{
		return (false);
	}

	inputFieldInfo.message = (uint16_t*)msg;			//Text to be displayed, as guide message, at upper left on the OSK
	inputFieldInfo.init_text = (uint16_t*)init;			//Initial text
	inputFieldInfo.limit_length = CELL_OSKDIALOG_STRING_SIZE;	//Length limitation for input text
	CreateActivationParameters();
	if(!EnableKeyLayout())
	{
		return (false);
	}

	ret = cellOskDialogLoadAsync(containerid, &dialogParam, &inputFieldInfo);
	if(ret < 0)
	{
		return (false);
	}
	mFlags |= OSK_IN_USE;
	return (true);
}

bool OSKUtil::Abort()
{
	int ret;

	if ((mFlags & OSK_IN_USE) == 0)
	{
		return (false);
	}

	ret = cellOskDialogAbort();
	if (ret < 0)
	{
		return (false);
	}
	return (true);
}

void OSKUtil::Stop()
{
	int ret;

	outputInfo.result = CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK;	// Result onscreen keyboard dialog termination
	outputInfo.numCharsResultString = 16;				// Specify number of characters for returned text
	outputInfo.pResultString = (uint16_t *)result_text_buffer;	// Buffer storing returned text

	ret = cellOskDialogUnloadAsync(&outputInfo);

	if (outputInfo.result == CELL_OSKDIALOG_INPUT_FIELD_RESULT_OK)
	{
		int num=wcstombs(result_text_buffer_char, result_text_buffer, 256);
		result_text_buffer_char[num]=0;
	}

	mFlags &= ~OSK_IN_USE;
}

void OSKUtil::Close()
{
	int ret;

	ret = sys_memory_container_destroy(containerid);

}

const char * OSKUtil::OutputString()
{
	return result_text_buffer_char;
}

void OSKUtil::CreateActivationParameters()
{
	// Initial display psition of the OSK (On-Screen Keyboard) dialog [x, y]
	pos.x = 0.0;
	pos.y = 0.0;

	// Set standard position
	int32_t LayoutMode = CELL_OSKDIALOG_LAYOUTMODE_X_ALIGN_CENTER | CELL_OSKDIALOG_LAYOUTMODE_Y_ALIGN_TOP;
	cellOskDialogSetLayoutMode(LayoutMode);

	//Select panels to be used using flags
	// NOTE: We don't need CELL_OSKDIALOG_PANELMODE_JAPANESE_KATAKANA and CELL_OSKDIALOG_PANELMODE_JAPANESE obviously (and Korean), so I'm going to
	// leave that all out	
	dialogParam.allowOskPanelFlg =
						CELL_OSKDIALOG_PANELMODE_ALPHABET |
						CELL_OSKDIALOG_PANELMODE_NUMERAL |
						CELL_OSKDIALOG_PANELMODE_NUMERAL_FULL_WIDTH |
						CELL_OSKDIALOG_PANELMODE_ENGLISH;
	// Panel to display first
	dialogParam.firstViewPanel = CELL_OSKDIALOG_PANELMODE_ENGLISH;
	// Initial display position of the onscreen keyboard dialog
	dialogParam.controlPoint = pos;
	dialogParam.prohibitFlgs = 0;
}

void OSKUtil::ProhibitFlags(int prohibitedFlag)
{
	// Prohibited operation flag(s) (ex. CELL_OSKDIALOG_NO_SPACE)
	// dialogParam.prohibitFlgs = 0;
	dialogParam.prohibitFlgs = prohibitedFlag;
}

bool OSKUtil::EnableKeyLayout()
{
	int ret;
	ret = cellOskDialogSetKeyLayoutOption(CELL_OSKDIALOG_10KEY_PANEL | CELL_OSKDIALOG_FULLKEY_PANEL);
	if (ret < 0)
	{
		return (false);
	}
	return (true);
}

uint64_t OSKUtil::getString(std::string& out) {}
