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





#ifndef __AUDIO_STREAM_H
#define __AUDIO_STREAM_H

#include <stddef.h>
#include "../threads/thread_fifo.hpp"
#include "../threads/cond.hpp"
#include "../threads/thread.hpp"
#include "../threads/scoped_lock.hpp"
#include "resampler.hpp"
#include "quadratic_resampler.hpp"
#include <algorithm>
#include <limits>
#include <string.h>
#include <stdlib.h>

namespace Audio {
   
   namespace Internal {
      
#define AUDIO_CHANNELS (2)

      // Hard-wired for 2 channels of audio!
      template <class C>
      inline void array_to_float(float * __restrict__ out, unsigned, const C * __restrict__ in, size_t frames)
      {
         size_t samples = frames * AUDIO_CHANNELS;
         if (std::numeric_limits<C>::min() == 0) // Unsigned
         {
            for (size_t i = 0; i < samples; i++)
               out[i] = ((float)in[i]/(std::numeric_limits<C>::max())) * 2.0f - 1.0f;
         }
         else
         {
            for (size_t i = 0; i < samples; i++)
               out[i] = (float)in[i]/(std::numeric_limits<C>::max() + 1);
         }
      }      

      template <class C>
      inline void float_to_array(C * __restrict__ out, const float * __restrict__ in, size_t samples)
      {
         size_t max = std::numeric_limits<C>::max();
         if (std::numeric_limits<C>::min() == 0) // Unsigned
         {
            for (size_t i = 0; i < samples; i++)
               out[i] = static_cast<C>(0.5 * (in[i] + 1.0) * max);
         }
         else
         {
            for (size_t i = 0; i < samples; i++)
               out[i] = static_cast<C>(in[i] * max);
         }
      }

      // Special case for floats, we just copy.
      inline void array_to_float(float * __restrict__ out, unsigned, const float * __restrict__ in, size_t frames)
      {
         size_t samples = frames * AUDIO_CHANNELS;
         std::copy(in, in + samples, out);
      }

      inline void float_to_array(float * __restrict__ out, const float * __restrict__ in, size_t samples)
      {
         array_to_float(out, AUDIO_CHANNELS, in, samples / AUDIO_CHANNELS);
      }
   }


template<class T>
class Stream
{
   public:

      Stream() : m_fn(Internal::array_to_float<T>), m_callback(NULL), m_saved_callback(NULL) {}

      // Returns number of samples you can write without blocking.
      virtual size_t write_avail() = 0;
      // Writes 'samples' samples to buffer. Will block until everything is written. Writing more data than write_avail() returns with an empty buffer is not safe.
      virtual size_t write(const T* in, size_t samples) = 0;
      // Notifies interface that you will not be writing more data to buffer until unpause();
      virtual void pause() 
      { 
         Threads::ScopedLock foo(lock);
         m_saved_callback = m_callback; 
         m_callback = NULL;
      }
      // Notifies interface that you would like to start writing data again.
      virtual void unpause() 
      {
         Threads::ScopedLock foo(lock);
         if (!m_callback)
            m_callback = m_saved_callback;
      }
      // Returns false if initialization failed. It is possible that that a pause()/unpause() sequence will remedy this.
      virtual bool alive() const { return true; }

      // Sets a callback for converting T frames of audio to floating point [-1.0, 1.0] frames with out_channels channels. out_channels is not guaranteed to be equal to input channels. It is called on from a thread, so make sure code is thread safe. This can be changed on the fly (or set to NULL to use internal converter) to allow for using self defined DSP effects, etc.
      // It is possible to avoid calling this function if:
      // Audio input channels == 2 and
      // Sample format is trivially convertable to floating point.
      virtual void set_float_conv_func(void (*fn)(float * out, unsigned out_channels, const T* in, size_t frames))
      {
         Threads::ScopedLock foo(lock);
         if (fn == NULL)
            m_fn = Internal::array_to_float<T>;
         else
            m_fn = fn;
      }

      // If this returns true, set_float_conv_func() must be set before starting callback or write().
      virtual bool need_float_conv() const { return false; }

      // By giving this a function pointer different than NULL, callback interface is activated. write() and write_avail() are no-ops. The callback will call this function sporadically. You can return a number of frames less than desired, but this will usually mean the driver itself will fill the rest with silence. cb_data is userdefined callback data. This can be NULL. After activating callback, by calling this again with NULL for callback argument, callbacks will be disabled and you can use normal, blocking write() and write_avail() again.
      virtual void set_audio_callback(ssize_t (*cb)(T*, size_t frames, void *data), void *cb_data)
      {
         Threads::ScopedLock foo(lock);
         m_callback = cb;
         data = cb_data;
      }

      virtual ~Stream() {}
      
      
   protected:
      inline void convert_frames(float *out, unsigned out_channels, const T* in, size_t frames)
      {
         Threads::ScopedLock foo(lock);
         m_fn(out, out_channels, in, frames);
      }

      inline bool callback_active()
      {
         Threads::ScopedLock foo(lock);
         return m_callback;
      }

      inline ssize_t callback(T* out, size_t frames)
      {
         Threads::ScopedLock foo(lock);
         if (!m_callback)
            return -1;

         return m_callback(out, frames, data);
      }

   private:
      void (*m_fn)(float *, unsigned, const T*, size_t);
      ssize_t (*m_callback)(T*, size_t, void*);
      ssize_t (*m_saved_callback)(T*, size_t, void*);
      Threads::Mutex lock;
      void *data;
};

}

#endif
