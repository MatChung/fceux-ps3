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





#ifndef __OSKUTIL_H__
#define __OSKUTIL_H__

#include <sysutil/sysutil_oskdialog.h>
#include <sysutil/sysutil_common.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

class OSKUtil
{
   public:
      OSKUtil();
      OSKUtil(std::string& msg, std::string& init);
      OSKUtil(std::string& msg, std::string& init, unsigned int memorycontainer);
      ~OSKUtil();

      uint64_t getString(std::string& out);
      const char * OutputString();
      bool Start(const wchar_t* msg, const wchar_t* init);
      bool Abort();
      void Stop();
      void Close();
      void ProhibitFlags(int prohibitedFlag); 
   protected:
      void str_to_utf16(uint16_t*& buf, const std::string& str);
   private:
      unsigned int m_oskdialog_memorycontainer;
      uint32_t mFlags;
      wchar_t result_text_buffer[CELL_OSKDIALOG_STRING_SIZE + 1];
      uint16_t* msg_buf;
      uint16_t* init_buf;
      char result_text_buffer_char[256 + 1];
      std::string m_msg;
      std::string m_init;
      sys_memory_container_t containerid;
      CellOskDialogPoint pos;
      CellOskDialogInputFieldInfo inputFieldInfo;
      CellOskDialogCallbackReturnParam outputInfo;
      // Onscreen keyboard dialog utility activation parameters
      CellOskDialogParam dialogParam;

      //Functions
      void CreateActivationParameters();
      bool EnableKeyLayout();
};

enum{
	MODE_IDLE = 0,
	MODE_OPEN,
	MODE_RUNNING,
	MODE_CLOSE,
	MODE_ENTERED,
	MODE_CANCELED,
	SET_ABOR_TIMER,
	CHANGE_PANEL_MODE,
	MODE_EXIT_OSK,
	START_DIALOG_TYPE,
	START_SEPARATE_TYPE_1,
	START_SEPARATE_TYPE_2,
};

#endif
