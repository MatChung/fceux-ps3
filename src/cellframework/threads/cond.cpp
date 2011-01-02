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





#include "cond.hpp"
#include "scoped_lock.hpp"

namespace Threads {

Cond::Cond()
{
   pthread_cond_init(&m_cond, NULL);
}

Cond::~Cond()
{
   pthread_cond_destroy(&m_cond);
}

void Cond::wait()
{
   m_lock.lock();
   pthread_cond_wait(&m_cond, &m_lock.m_lock);
   m_lock.unlock();
}

void Cond::wake()
{
   pthread_cond_signal(&m_cond);
}

void Cond::wake_all()
{
   pthread_cond_broadcast(&m_cond);
}

}
