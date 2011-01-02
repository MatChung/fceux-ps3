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





#include "TestAudio.hpp"
#include <vector>
#include <fastmath.h>

TestAudio::TestAudio() : audio(new Audio::AudioPort<int16_t>(2, 48000))
{}

void TestAudio::start()
{
   running = true;
   thread = new Threads::Thread(&TestAudio::audio_thread, this);
}

void TestAudio::stop()
{
   running = false;
   thread->join();
}

TestAudio::~TestAudio()
{
   delete thread;
   delete audio;
}

void TestAudio::audio_thread()
{
   uint64_t cnt = 0;
   std::vector<int16_t> buf(1024);
   while (running)
   {
      for (unsigned i = 0; i < buf.size(); i+=2)
      {
         buf[i] = (int16_t)(0x7FFE * sin(cnt * 0.01));
         buf[i+1] = (int16_t)(0x7FFE * sin(cnt * 0.01));
         cnt++;
      }
      audio->write(&buf[0], buf.size());
   }
}
