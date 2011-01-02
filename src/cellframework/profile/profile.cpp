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





#include "profile.h"
#include "../network/TCPSocket.hpp"

#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <iostream>
#include <fstream>

using namespace std;

#include "../utility/util.h"

class TimeInfo
{
   public:
      void start()
      {
         m_start = get_usec();
      }

      void stop()
      {
         m_stop = get_usec();
         m_delta = m_stop - m_start;
         m_total += m_delta;
         m_cnt++;
      }

      uint64_t clock() const
      {
         return m_delta;
      }

      uint64_t total_clock() const
      {
         return m_total;
      }

      uint64_t count() const
      {
         return m_cnt;
      }

      uint64_t avg_usec() const
      {
         return m_total / m_cnt;
      }

   private:
      uint64_t m_start, m_stop, m_cnt, m_delta, m_total;
};

static map<string, TimeInfo> g_map;

void perf_start(const char *ident)
{
   g_map[ident].start();
}

void perf_stop(const char *ident)
{
   g_map[ident].stop();
}

uint64_t perf_last_time(const char *ident)
{
   return g_map[ident].clock();
}

uint64_t perf_total_time(const char *ident)
{
   return g_map[ident].total_clock();
}

uint64_t perf_avg_time(const char *ident)
{
   return g_map[ident].avg_usec();
}

uint64_t perf_count(const char *ident)
{
   return g_map[ident].count();
}

void perf_dump_file(const char *path)
{
   fstream out(path, ios::out);
   if (!out.is_open())
      return;

   for (map<string, TimeInfo>::iterator itr = g_map.begin(); itr != g_map.end(); ++itr)
   {
      out << ":: " << itr->first << " ::" << endl << "\t" << "Calls: " << itr->second.count() << "   Avg call time (usec): " << itr->second.avg_usec() << "  Total call time (usec): " << itr->second.total_clock() << endl;
   }
}

void perf_dump_tcp(const char *host, uint16_t port)
{
   Network::TCPSocket sock(host, port);
   if (!sock.alive())
      return;
   
   ostringstream stream;
   for (map<string, TimeInfo>::iterator itr = g_map.begin(); itr != g_map.end() && sock.alive(); ++itr)
   {
      stream << ":: " << itr->first << " ::" << endl << "\t" << "Calls: " << itr->second.count() << "   Avg call time (usec): " << itr->second.avg_usec() << "  Total call time (usec): " << itr->second.total_clock() << endl;
      sock.send(stream.str());
      stream.str("");
   }
}

