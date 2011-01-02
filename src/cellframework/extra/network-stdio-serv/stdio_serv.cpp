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






#include "../../network/TCPSocket.hpp"
#include "../../threads/thread.hpp"
#include "IOHandler.hpp"
#include <iostream>

using namespace Threads;
using namespace Network;

static void thread_entry(TCPSocket *sock)
{
   IOHandler handler(sock);
   handler.run();
}

static void start_thread(TCPSocket& sock)
{
   TCPSocket *tmp = new TCPSocket;
   tmp->grab(sock); // Emulates a move constructor.

   Thread thr(thread_entry, tmp);
   thr.detach();
}

int main()
{
   std::cout << "=================================" << std::endl;
   std::cout << " STIOSRV: Networked stdio server" << std::endl;
   std::cout << "=================================" << std::endl;
   TCPServerSocket serv(9001);

   if (!serv.alive())
   {
      std::cerr << "Failed to start server. :( Port is taken? :D" << std::endl;
      return 1;
   }

   while(serv.alive())
   {
      TCPSocket sock = serv.accept();
      sock.nodelay(true);
      if (sock.alive())
      {
         start_thread(sock);
      }
   }
}

