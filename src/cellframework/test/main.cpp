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





#include <sys/spu_initialize.h>
#include <sysutil/sysutil_sysparam.h>
#include "TestGFX.hpp"
#include "TestAudio.hpp"
#include <input/cellinput.h>

// Needed for PS3. Defines process priorities (?!) and stack space. Shouldn't need to be changed, but stack space (second argument) can be changed to a maximum of 1 MiB. 
SYS_PROCESS_PARAM(1001, 0x10000);
static volatile bool main_loop_active = true;

// This is called when calling cellSysutilCheckCallback()
static void sysutil_callback(uint64_t status, uint64_t param, void *userdata)
{
   (void)param;
   (void)userdata;

   switch (status)
   {
      case CELL_SYSUTIL_REQUEST_EXITGAME:
         main_loop_active = false;
         break;
      default:
         break;
   }
}

int main()
{
   // Generic PS3 stuff. Init SPUs and register callback.
   sys_spu_initialize(6, 1);
   cellSysutilRegisterCallback(0, sysutil_callback, NULL);

   // Our test video class. Inherits from a more general graphics class that does boilerplate init.
   // PS3 has very limited stack space. Use heap when you can!
   std::auto_ptr<TestGFX> gfx(new TestGFX);
   gfx->Init(); // We do a generic init here.

   // Our audio class.
   std::auto_ptr<TestAudio> audio(new TestAudio);
   audio->start(); // Start audio thread. Let it play something cool in the background for now.

   // Our input class.
   std::auto_ptr<CellInputFacade> input(new CellInputFacade);
   input->Init();

   // A very simple main loop. Just display a rotating quad. :)
   while (main_loop_active)
   {
      // In all main loops we need to check if user has pressed "Quit Game". If so, we can exit loop
      cellSysutilCheckCallback();

      gfx->clear();
      if (input->IsButtonPressed(0, CTRL_SQUARE))
      {
         gfx->rotate(-5);
      }
      else if (input->IsButtonPressed(0, CTRL_CIRCLE))
      {
         gfx->rotate(5);
      }
      gfx->render();
      gfx->swap();
   }

   audio->stop(); // Stop audio thread.
   input->Deinit();
}

