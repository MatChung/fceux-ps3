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





#ifndef __THREAD_FIFO_H
#define __THREAD_FIFO_H

#include <deque>
#include <stddef.h>
#include "fifo.hpp"
#include "scoped_lock.hpp"
#include "mutex.hpp"

namespace Threads {

template<class T, class Container = std::deque<T> >
class ThreadFifo : public FifoBuffer<T, Container>
{
   public:
      ThreadFifo(size_t num_elems);
      size_t read_avail();
      size_t write_avail();
      size_t write(const T* in, size_t num_elems);
      size_t read(T* in, size_t num_elems);
      void clear();
      void resize(size_t num_elems);
   private:
      Mutex m_lock;
};

template<class T, class Container>
ThreadFifo<T, Container>::ThreadFifo(size_t num_elems) : FifoBuffer<T, Container>(num_elems)
{}

template<class T, class Container>
size_t ThreadFifo<T, Container>::read_avail()
{
   ScopedLock lock(m_lock);
   return FifoBuffer<T, Container>::read_avail();
}

template<class T, class Container>
size_t ThreadFifo<T, Container>::write_avail()
{
   ScopedLock lock(m_lock);
   return FifoBuffer<T, Container>::write_avail();
}

template<class T, class Container>
size_t ThreadFifo<T, Container>::write(const T* in, size_t num_elems)
{
   ScopedLock lock(m_lock);
   return FifoBuffer<T, Container>::write(in, num_elems);
}

template<class T, class Container>
size_t ThreadFifo<T, Container>::read(T* in, size_t num_elems)
{
   ScopedLock lock(m_lock);
   return FifoBuffer<T, Container>::read(in, num_elems);
}

template<class T, class Container>
void ThreadFifo<T, Container>::clear()
{
   ScopedLock lock(m_lock);
   FifoBuffer<T, Container>::clear();
}

template<class T, class Container>
void ThreadFifo<T, Container>::resize(size_t num_elems)
{
   ScopedLock lock(m_lock);
   FifoBuffer<T, Container>::resize(num_elems);
}
}

#endif
