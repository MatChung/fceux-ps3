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





#ifndef __AUDIO_RSOUND_H
#define __AUDIO_RSOUND_H

#include "stream.hpp"
#include "rsound.h"
#include <string>
#include "../threads/thread.hpp"
#ifdef __CELLOS_LV2__
#include "../network/network.hpp"
#endif

namespace Audio {

template <class T>
class RSound : public Stream<T>, public Network::NetworkInterface
{
   public:
      RSound(std::string server, int channels, int samplerate, int buffersize = 8092, int latency = 64) : thread(NULL), thread_active(false), m_chan(channels), must_conv(false)
      {
         rsd_init(&rd);
         int format = type_to_format(T());
         rsd_set_param(rd, RSD_FORMAT, &format);
         rsd_set_param(rd, RSD_CHANNELS, &channels);
         rsd_set_param(rd, RSD_HOST, const_cast<char*>(server.c_str()));
         rsd_set_param(rd, RSD_SAMPLERATE, &samplerate);

         if (buffersize < 256)
            buffersize = 256;

         convbuf = new float[buffersize];
         reconvbuf = new T[buffersize];
         buffersize *= sizeof(T);
         rsd_set_param(rd, RSD_BUFSIZE, &buffersize);
         rsd_set_param(rd, RSD_LATENCY, &latency);
         m_latency = latency;

         int rc = rsd_start(rd);
         if (rc < 0) // Couldn't start, don't do anything after this, might implement some proper error handling here.
            runnable = false;
         else
            runnable = true;

         emptybuf = new uint8_t[buffersize];
         memset(emptybuf, 0, buffersize);
      }

      ~RSound()
      {
         stop_thread();
         delete[] emptybuf;
         delete[] convbuf;
         delete[] reconvbuf;
         rsd_stop(rd);
         rsd_free(rd);
      }

      size_t write(const T* in, size_t samples)
      {
         if (!runnable || this->callback_active() || samples == 0)
            return 0;

         const T *write_buf = in;
         if (must_conv)
         {
            convert_frames(convbuf, m_chan, in, samples / m_chan);
            Internal::float_to_array(reconvbuf, convbuf, samples);
            write_buf = reconvbuf;
         }

         rsd_delay_wait(rd);
         size_t rc = rsd_write(rd, write_buf, samples * sizeof(T))/sizeof(T);
         if (rc == 0)
         {
            runnable = false;
            return 0;
         }
         
         // Close to underrun, fill up buffer
         if (rsd_delay_ms(rd) < m_latency / 2)
         {
            size_t size = rsd_get_avail(rd);
            rsd_write(rd, emptybuf, size);
         }
         return rc;
      }

      bool alive() const
      {
         return runnable;
      }

      size_t write_avail()
      {
         if (!runnable || this->callback_active())
            return 0;

         // We'll block
         if (rsd_delay_ms(rd) > m_latency)
            return 0;

         return rsd_get_avail(rd) / sizeof(T);
      }

      void pause()
      {
         stop_thread();

         if (runnable)
         {
            runnable = false;
            rsd_pause(rd, 1);
         }
      }

      void set_float_conv_func(void (*fn)(float * out, unsigned out_channels, const T* in, size_t frames))
      {
         Stream<T>::set_float_conv_func(fn);
         if (fn == NULL)
            must_conv = false;
         else
            must_conv = true;
      }

      void set_audio_callback(ssize_t (*cb)(T*, size_t, void*), void *data)
      {
         Stream<T>::set_audio_callback(cb, data);

         if (this->callback_active() && thread == NULL && runnable)
         {
            thread_active = true;
            thread = new Threads::Thread(&RSound<T>::callback_thread, this);
         }
         else if (!this->callback_active() && thread != NULL && runnable)
         {
            thread_active = false;
            delete thread;
            thread = NULL;
         }
      }

      void unpause()
      {
         if (!runnable)
         {
            if (rsd_pause(rd, 0) < 0)
               runnable = false;
            else
               runnable = true;

            start_thread();
         }
      }

   private:
      bool runnable;
      rsound_t *rd;
      int m_latency;
      uint8_t *emptybuf;
      float *convbuf;
      T *reconvbuf;
      unsigned m_chan;
      Threads::Thread *thread;
      volatile bool thread_active;
      bool must_conv;

      int type_to_format(uint8_t) { return RSD_U8; }
      int type_to_format(int8_t) { return RSD_S8; }
      int type_to_format(int16_t) { return RSD_S16_NE; }
      int type_to_format(uint16_t) { return RSD_U16_NE; }

      void start_thread()
      {
         if (runnable && this->callback_active() && thread == NULL)
         {
            thread_active = true;
            thread = new Threads::Thread(&RSound<T>::callback_thread, this);
         }
      }

      void stop_thread()
      {
         if (runnable && this->callback_active() && thread != NULL)
         {
            thread_active = false;
            delete thread;
            thread = NULL;
         }
      }

      void callback_thread()
      {
         T* buf = new T[256 * m_chan]; // Just some arbitrary size
         while (thread_active)
         {
            ssize_t ret = callback(buf, 256);
            if (ret < 0)
               break;

            if (ret < 256)
            {
               memset(buf + ret * m_chan, 0, (256 - ret) * m_chan * sizeof(T));
            }

            const T *write_buf = buf;
            if (must_conv)
            {
               convert_frames(convbuf, m_chan, buf, 256);
               Internal::float_to_array(reconvbuf, convbuf, 256 * m_chan);
               write_buf = reconvbuf;
            }

            rsd_delay_wait(rd);
            if (rsd_write(rd, write_buf, 256 * m_chan * sizeof(T)) == 0)
               break;
         }
      }

};

}

#endif
