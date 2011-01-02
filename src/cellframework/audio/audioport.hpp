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





#ifndef __AUDIO_PORT_HPP
#define __AUDIO_PORT_HPP

#include "stream.hpp"
#include <cell/audio.h>
#include <cell/sysmodule.h>
#include "../utility/ref_counted.hpp"

namespace Audio {

#define AUDIO_CHANNELS (2)
#define AUDIO_OUT_RATE (48000.0)
#define AUDIO_BLOCKS (8)

   namespace Internal {

      class AudioPortRef : public ref_counted<AudioPortRef>
      {
         public:
            AudioPortRef()
            {
               if (ref() == 0)
               {
                  cellSysmoduleLoadModule(CELL_SYSMODULE_AUDIO);
                  cellAudioInit();
               }
               ref()++;
            }

            ~AudioPortRef()
            {
               if (ref() == 1)
               {
                  cellSysmoduleUnloadModule(CELL_SYSMODULE_AUDIO);
                  cellAudioQuit();
               }
               ref()--;
            }
      };
   }

template<class T, class ResamplerCore = QuadraticResampler>
class AudioPort : public Stream<T>, public Internal::AudioPortRef
{
   public:
      AudioPort(unsigned channels, unsigned samplerate, size_t num_samples = 8092) : fifo(Threads::ThreadFifo<T>(num_samples)), quit_thread(false), input_rate(samplerate), fifo_size(num_samples), m_chan(channels)
      {
         CellAudioPortParam params;

         params.nChannel = AUDIO_CHANNELS;
         params.nBlock = AUDIO_BLOCKS;
         params.attr = 0;

         cellAudioPortOpen(&params, &audio_port);

         cellAudioPortStart(audio_port);
         unsigned tmp_chans = m_chan < 2 ? 2 : m_chan;
         tmp = (T*)memalign(128, CELL_AUDIO_BLOCK_SAMPLES * tmp_chans * sizeof(T));
         out_buffer = (float*)memalign(128, CELL_AUDIO_BLOCK_SAMPLES * AUDIO_CHANNELS * sizeof(float));
         thread = new Threads::Thread(&AudioPort<T>::event_loop, this);
      }

      size_t write_avail()
      {
         return fifo.write_avail();
      }

      size_t write(const T* in, size_t samples)
      {
         while (samples > 0)
         {
            size_t write_amount = samples > fifo_size ? fifo_size : samples;

            size_t written = 0;
            for(;;)
            {
               size_t wrote = fifo.write(in, write_amount - written);
               written += wrote;
               in += wrote;
               samples -= wrote;

               if (written == write_amount)
                  break;
               cond.wait();
            }
         }

         return samples;
      }

      ~AudioPort()
      {
         quit_thread = true;
         thread->join();
         delete thread;
         free(tmp);
         free(out_buffer);

         cellAudioPortStop(audio_port);
         cellAudioPortClose(audio_port);
      }

      // Callback for resampler
      ssize_t operator()(float **data)
      {
         ssize_t has_read = 0;
         if (this->callback_active())
         {
            has_read = callback(tmp, CELL_AUDIO_BLOCK_SAMPLES);
            if (has_read < 0)
               has_read = 0;
            has_read *= m_chan;
         }
         else
         {
            has_read = fifo.read(tmp, CELL_AUDIO_BLOCK_SAMPLES * m_chan);
         }

         if (has_read < CELL_AUDIO_BLOCK_SAMPLES * m_chan)
         {
            memset(tmp + has_read, 0, (CELL_AUDIO_BLOCK_SAMPLES * m_chan - has_read) * sizeof(T));
         }

         convert_frames(resampler_buf, AUDIO_CHANNELS, tmp, CELL_AUDIO_BLOCK_SAMPLES);
         *data = resampler_buf;
         return CELL_AUDIO_BLOCK_SAMPLES * AUDIO_CHANNELS;
      }

      bool need_float_conv() const
      {
         return m_chan != AUDIO_CHANNELS;
      }

   private:
      Threads::ThreadFifo<T> fifo;
      Threads::Cond cond;
      volatile bool quit_thread;
      float resampler_buf[CELL_AUDIO_BLOCK_SAMPLES * AUDIO_CHANNELS];
      Threads::Thread *thread;
      uint64_t input_rate;
      uint32_t audio_port;
      size_t fifo_size;
      static int ref_count;
      T* tmp;
      float *out_buffer;
      unsigned m_chan;
      
      void event_loop()
      {
         sys_event_queue_t id;
         sys_ipc_key_t key;
         sys_event_t event;

         cellAudioCreateNotifyEventQueue(&id, &key);
         cellAudioSetNotifyEventQueue(key);
         
         Resampler *re = new ResamplerCore(*this, AUDIO_OUT_RATE/input_rate, AUDIO_CHANNELS);

         while (!quit_thread)
         {
            sys_event_queue_receive(id, &event, SYS_NO_TIMEOUT);
            re->pull(out_buffer, CELL_AUDIO_BLOCK_SAMPLES * AUDIO_CHANNELS);
            cellAudioAdd2chData(audio_port, out_buffer, CELL_AUDIO_BLOCK_SAMPLES, 1.0);
            cond.wake();
         }

         cellAudioRemoveNotifyEventQueue(key);
         delete re;
      }
};

}

#endif
