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





#ifndef __IOHANDLER_HPP
#define __IOHANDLER_HPP

#include "../../network/TCPSocket.hpp"
#include <stdint.h>
#include <string>
#include <stdio.h>

class IOHandler
{
   public:
      IOHandler(Network::TCPSocket*);
      ~IOHandler();

      void run();

   private:
      FILE *m_file;
      Network::TCPSocket *m_sock;
      typedef enum { ERROR, FOPEN, FCLOSE, FWRITE, FREAD, FTELL, FSEEK, UNGETC, FEOF } Command;
      Command get_command();
      void execute_command(Command);
      static const unsigned max_proto_len = 16;
      std::string m_path;

      bool fopen_exec();
      bool fclose_exec();
      bool fread_exec();
      bool fwrite_exec();
      bool ftell_exec();
      bool fseek_exec();
      bool ungetc_exec();
      bool feof_exec();

      size_t read_all(void *data, size_t bytes);
      size_t write_all(const void *data, size_t bytes);

      bool get_argument(std::string&);
      bool get_argument(int64_t&);
      bool return_argument(int64_t);

      template<class T, class D>
      bool get_argument(T& t, D& d)
      {
         if (!get_argument(t))
            return false;
         return get_argument(d);
      }

      template <class T, class D, class R>
      bool get_argument(T& t, D& d, R& r)
      {
         if (!get_argument(t))
            return false;
         return get_argument(d, r);
      }
};

#endif
