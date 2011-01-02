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





#ifndef __PROFILER_H
#define __PROFILER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Small performance checking library.
// Call perf_start() and perf_stop() with same idents several times to measure call count, average call time, etc. It's not very accurate for small functions, and might give very inaccurate results.

// Starts timer for ident. Cannot be called again until perf_stop() is called.
void perf_start(const char *ident);
// Stops timer for ident. Cannot be called unless timer is started with perf_start().
void perf_stop(const char *ident);

// Receives last timer in usec.
uint64_t perf_last_time(const char *ident);
// Receives accumulated timer for a certain ident in usec.
uint64_t perf_total_time(const char *ident);
// Receives number of times p
uint64_t perf_count(const char *ident);
uint64_t perf_avg_time(const char *ident);
// Dumps profiling data to a file
void perf_dump_file(const char *path);
// Dumps profiling data on a TCP socket.
void perf_dump_tcp(const char *host, uint16_t port);

#ifdef PROFILE
#define PERF_START(x) do {\
   perf_start(x);\
} while(0)

#define PERF_STOP(x) do {\
   perf_stop(x);\
} while(0)
#else
#define PERF_START(x)
#define PERF_STOP(x)
#endif

#ifdef __cplusplus
}
#endif

// Defines a RAII class that can use the profiling tool more effectively in C++. Just use Perf::Scoped(__func__); at top of your function or method call.
#ifdef __cplusplus
namespace Perf
{
   class Scoped
   {
      public:
         Scoped(const char *ident) : m_ident(ident)
         {
            perf_start(m_ident);
         }

         ~Scoped()
         {
            perf_stop(m_ident);
         }

      private:
         const char *m_ident;
   };
}
#endif


#endif
