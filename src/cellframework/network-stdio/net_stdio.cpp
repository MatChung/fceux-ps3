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





#include "net_stdio.h"
#include <string>
#include "../network/TCPSocket.hpp"
#include <map>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

using namespace std;
using namespace Network;

namespace IO {

   namespace Global {
      // This will leak memory, but we have to do this (I think) to "rely" on behavior that happens after exit(). (No global destructor fucking us up! :D)
      static string *host = new string;
      static uint16_t *port = new uint16_t;
      static string *pre_path = new string;
      static int *strip = new int(0);
      static bool *enabled = new bool(false);
   }


   class FileStreamer
   {
      public:
         FileStreamer(const string& host, const uint16_t port, const string& mode, const string& in_path = "/") : m_sock(TCPSocket(host, port)), m_path(in_path)
         {
            m_sock.nodelay(true);
            // omg, broken windows paths. :v
            for (std::string::iterator itr = m_path.begin(); itr != m_path.end(); ++itr)
            {
               if (*itr == '\\')
                  *itr = '/';
            }

            const char *c_path = m_path.c_str();
            const char *strip_path = c_path;
            const char *use_path;

            for (int i = 0; i < *Global::strip && strip_path != NULL; i++)
            {
               strip_path++;
               strip_path = strchr(strip_path, '/');
            }

            if (*Global::strip > 0 && strip_path != NULL)
               use_path = strip_path + 1;
            else
               use_path = c_path;

            std::string real_path;
            real_path += *Global::pre_path;
            real_path += use_path;

            m_alive = open_file(real_path, mode);
         }

         ~FileStreamer()
         {
            send_command(FCLOSE);
         }

         size_t write(const void *data, size_t bytes)
         {
            if (!send_command(FWRITE))
               return 0;

            if (!send_argument(bytes))
               return 0;

            write_all(data, bytes);

            int64_t ret;
            if (!get_return(ret))
               return 0;
            return (size_t)ret;
         }

         size_t read(void *data, size_t bytes)
         {
            if (!send_command(FREAD))
               return 0;

            if (!send_argument(bytes))
               return 0;

            int64_t ret;
            if (!get_return(ret))
               return 0;

            size_t has_read = read_all(data, ret);
            return has_read;
         }

         int seek(long offset, int whence)
         {
            if (!send_command(FSEEK))
               return -1;

            if (!send_argument(offset, whence))
               return -1;

            int64_t ret;
            if (!get_return(ret))
               return -1;

            return ret;
         }

         int ungetc(int c)
         {
            if (!send_command(UNGETC))
               return -1;

            if (!send_argument(c))
               return -1;

            int64_t ret;
            if (!get_return(ret))
               return -1;

            return ret;
         }

         long tell()
         {
            if (!send_command(FTELL))
               return -1;

            int64_t ret;
            if (!get_return(ret))
               return -1;

            return ret;
         }

         int eof()
         {
            if (!send_command(FEOF))
               return -1;

            int64_t ret;
            if (!get_return(ret))
               return -1;

            return ret;
         }

         bool alive() const
         {
            return m_sock.alive() && m_alive;
         }

      private:
         TCPSocket m_sock;
         string m_path;
         bool m_alive;
         typedef enum { ERROR, FOPEN, FCLOSE, FWRITE, FREAD, FTELL, FSEEK, UNGETC, FEOF } Command;
         static const unsigned max_proto_len = 16;

         bool open_file(const string& path, const string& mode)
         {
            if (!send_command(FOPEN))
               return false;

            if (!send_argument(path, mode))
               return false;

            int64_t ret;
            if (!get_return(ret))
               return false;

            return ret == 0;
         }

         bool send_command(Command cmd)
         {
            std::string str;
            switch (cmd)
            {
               case FOPEN:
                  str = "CMD_FOPEN";
                  break;
               case FCLOSE:
                  str = "CMD_FCLOSE";
                  break;
               case FWRITE:
                  str = "CMD_FWRITE";
                  break;
               case FREAD:
                  str = "CMD_FREAD";
                  break;
               case FTELL:
                  str = "CMD_FTELL";
                  break;
               case FSEEK:
                  str = "CMD_FSEEK";
                  break;
               case UNGETC:
                  str = "CMD_UNGETC";
                  break;
               case FEOF:
                  str = "CMD_FEOF";
                  break;
               default:
                  str = "CMD_ERROR";
            }

            char buf[max_proto_len] = {0};
            strncpy(buf, str.c_str(), max_proto_len);
            ssize_t ret = m_sock.send(buf, max_proto_len);
            if (ret != max_proto_len)
               return false;
            return true;
         }

         bool send_argument(int64_t arg)
         {
            char buf[max_proto_len + 1] = {0};
            char len_buf[max_proto_len + 1] = {0};
            snprintf(buf, max_proto_len + 1, "%0*u", max_proto_len, max_proto_len);
            snprintf(len_buf, max_proto_len + 1, "%0*lld", max_proto_len, (long long)arg);

            if (m_sock.send(buf, max_proto_len) != max_proto_len ||
                  m_sock.send(len_buf, max_proto_len) != max_proto_len)
               return false;
            return true;
         }

         bool send_argument(const std::string& arg)
         {
            char buf[max_proto_len + 1] = {0};
            snprintf(buf, max_proto_len + 1, "%0*lld", max_proto_len, (long long)arg.size());

            if (m_sock.send(buf, max_proto_len) != max_proto_len ||
                  m_sock.send(&arg[0], arg.size()) != arg.size())
               return false;
            return true;
         }

         bool get_return(int64_t& ret)
         {
            char buf[max_proto_len + 1] = {0};

            if (m_sock.recv(buf, max_proto_len) != max_proto_len)
               return false;

            ret = strtoll(buf, NULL, 10);
            return true;
         }

#define MAX_PACKET_SIZE (1024 << 10)
         size_t write_all(const void *data, size_t bytes)
         {
            size_t written = 0;
            while (written < bytes)
            {
               size_t write_amt = bytes - written > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : bytes - written;
               ssize_t rc = m_sock.send((const char*)data + written, write_amt);
               if (rc <= 0)
                  return written;

               written += rc;
            }
            return written;
         }

         size_t read_all(void *data, size_t bytes)
         {
            size_t has_read = 0;
            while (has_read < bytes)
            {
               size_t read_amt = bytes - has_read > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : bytes - has_read;
               ssize_t rc = m_sock.recv((char*)data + has_read, read_amt);
               if (rc <= 0)
                  return has_read;

               has_read += rc;
            }
            return has_read;
         }

         template <class T, class D>
         bool send_argument(const T& t, const D& d)
         {
            return send_argument(t) && send_argument(d);
         }

         template <class T, class D, class R>
         bool send_argument(const T& t, const D& d, const R& r)
         {
            return send_argument(t, d) && send_argument(r);
         }
   };

   namespace Global {
      static std::map<int, FileStreamer*> *g_map = new std::map<int, FileStreamer*>; 
      static std::map<const FileStreamer*, bool> *g_valid_map = new std::map<const FileStreamer*, bool>;
      static std::map<const FILE*, FILE*> *g_hook_map = new std::map<const FILE*, FILE*>;

      inline bool is_net_file(FILE* file)
      {
         return (*g_valid_map)[reinterpret_cast<const FileStreamer*>(file)];
      }

      inline void set_new_file(const FileStreamer* file)
      {
         (*g_valid_map)[file] = true;
      }

      inline void set_closed_file(const FileStreamer* file)
      {
         (*g_valid_map)[file] = false;
      }

      inline FILE* get_stdio_hook(FILE *stream)
      {
         return (*g_hook_map)[stream];
      }

      inline void add_stdio_hook(FILE *stream, const char *in_path)
      {
         if (stream == stdin || stream == stdout || stream == stderr)
         {
            if ((*g_hook_map)[stream] != NULL)
            {
               fclose((*g_hook_map)[stream]);
            }

            (*g_hook_map)[stream] = fopen(in_path, stream == stdin ? "r" : "w");
         }
      }

      inline void remove_stdio_hook(FILE *stream)
      {
         if (stream == stdin || stream == stdout || stream == stderr)
         {
            if ((*g_hook_map)[stream] != NULL)
            {
               fclose((*g_hook_map)[stream]);
               (*g_hook_map)[stream] = NULL;
            }
         }
      }
   }

}

void net_stdio_enable(int enable)
{
   *IO::Global::enabled = (bool)enable;
}

void net_stdio_set_target(const char *host, uint16_t port)
{
   *IO::Global::host = host;
   *IO::Global::port = port;
}

void net_stdio_set_paths(const char *path, int strip)
{
   *IO::Global::pre_path = path;
   *IO::Global::strip = strip;
}

void net_stdio_hook(FILE *file, const char *path)
{
   if (path != NULL)
      IO::Global::add_stdio_hook(file, path);
   else
      IO::Global::remove_stdio_hook(file);
}

extern "C" {
   FILE *__wrap_fopen(const char*, const char*);
   FILE *__real_fopen(const char*, const char*);

   size_t __wrap_fwrite(const void* , size_t, size_t, FILE*);
   size_t __real_fwrite(const void*, size_t, size_t, FILE*);

   size_t __wrap_fread(void*, size_t, size_t, FILE*);
   size_t __real_fread(void*, size_t, size_t, FILE*);

   int __wrap_fclose(FILE*);
   int __real_fclose(FILE*);

   int __wrap_feof(FILE*);
   int __real_feof(FILE*);

   int __wrap_fseek(FILE*, long, int);
   int __real_fseek(FILE*, long, int);

   int __wrap_fflush(FILE*);
   int __real_fflush(FILE*);

   void __wrap_rewind(FILE*);
   void __real_rewind(FILE*);

   long __wrap_ftell(FILE*);
   long __real_ftell(FILE*);

   int __wrap_fgetpos(FILE*, fpos_t*);
   int __real_fgetpos(FILE*, fpos_t*);

   int __wrap_fsetpos(FILE*, fpos_t*);
   int __real_fsetpos(FILE*, fpos_t*);

   int __wrap_ungetc(int, FILE*);
   int __real_ungetc(int, FILE*);

   int __wrap_setvbuf(FILE*, char*, int, size_t);
   int __real_setvbuf(FILE*, char*, int, size_t);

   void __wrap_setbuf(FILE*, char*);
   void __real_setbuf(FILE*, char*);
}

using namespace IO;
using namespace IO::Global;

FILE *__wrap_fopen(const char *in_path, const char* in_mode)
{
   if (!(*enabled))
      return __real_fopen(in_path, in_mode);

   FileStreamer *stream = new FileStreamer(*host, *port, in_mode, in_path);
   if (!stream->alive())
   {
      delete stream;
      return NULL;
   }
   set_new_file(stream);
   return reinterpret_cast<FILE*>(stream);
}

size_t __wrap_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
   FILE *wrap = get_stdio_hook(stream);
   if (wrap)
      fwrite(ptr, size, nmemb, wrap);

   if (is_net_file(stream))
   {
      FileStreamer *sock = reinterpret_cast<FileStreamer*>(stream);
      return sock->write(ptr, size * nmemb) / size;
   }
   else
      return __real_fwrite(ptr, size, nmemb, stream);
}

size_t __wrap_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
   FILE *wrap = get_stdio_hook(stream);
   if (wrap)
      fread(ptr, size, nmemb, wrap);

   if (is_net_file(stream))
   {
      FileStreamer *sock = reinterpret_cast<FileStreamer*>(stream);
      return sock->read(ptr, size * nmemb) / size;
   }
   else
      return __real_fread(ptr, size, nmemb, stream);
}

int __wrap_fclose(FILE *fp)
{
   if (get_stdio_hook(fp) != NULL)
      remove_stdio_hook(fp);

   if (is_net_file(fp))
   {
      FileStreamer *sock = reinterpret_cast<FileStreamer*>(fp);
      delete sock;
      set_closed_file(sock);
      return 0;
   }
   else
      return __real_fclose(fp);
}

int __wrap_feof(FILE *fp)
{
   if (is_net_file(fp))
   {
      FileStreamer *sock = reinterpret_cast<FileStreamer*>(fp);
      return sock->eof();
   }
   else
      return __real_feof(fp);
}

int __wrap_fseek(FILE *stream, long offset, int whence)
{
   if (is_net_file(stream))
   {
      FileStreamer *sock = reinterpret_cast<FileStreamer*>(stream);
      return sock->seek(offset, whence);
   }
   else
      return __real_fseek(stream, offset, whence);
}

int __wrap_fflush(FILE* stream)
{
   if (is_net_file(stream))
      return 0;
   else
      return __real_fflush(stream);
}

void __wrap_rewind(FILE* stream)
{
   fseek(stream, SEEK_SET, 0);
}

long __wrap_ftell(FILE* stream)
{
   if (is_net_file(stream))
   {
      FileStreamer *sock = reinterpret_cast<FileStreamer*>(stream);
      return sock->tell();
   }
   else
      return __real_ftell(stream);
}

int __wrap_fgetpos(FILE* stream, fpos_t* pos)
{
   if (!is_net_file(stream))
   {
      return __real_fgetpos(stream, pos);
   }
   else
      return -1;
}

int __wrap_fsetpos(FILE* stream, fpos_t* pos)
{
   if (!is_net_file(stream))
   {
      return __real_fsetpos(stream, pos);
   }
   else
      return -1;
}

int __wrap_ungetc(int c, FILE* stream)
{
   if (is_net_file(stream))
   {
      FileStreamer *sock = reinterpret_cast<FileStreamer*>(stream);
      return sock->ungetc(c);
   }
   else
      return __real_ungetc(c, stream);
}

int __wrap_setvbuf(FILE* stream, char* buf, int mode, size_t size)
{
   if (!is_net_file(stream))
      return __real_setvbuf(stream, buf, mode, size);
   else
      return 0;
}

void __wrap_setbuf(FILE* stream, char* buf)
{
   if (!is_net_file(stream))
      __real_setbuf(stream, buf);
}








