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
 * utils.h
 *
 *  Created on: Nov 16, 2010
 *      Author: halsafar
 */

#ifndef UTILS_H_
#define UTILS_H_

// Platform dependant
#ifdef __CELLOS_LV2__
#include <sys/sys_time.h>
static uint64_t get_usec()
{
   return sys_time_get_system_time();
}
#else
#include <time.h>
static uint64_t get_usec()
{
   struct timespec tv;
   clock_gettime(CLOCK_MONOTONIC, &tv);
   return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_nsec / 1000;
}
#endif

#endif /* UTILS_H_ */
