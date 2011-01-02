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





#include "IOHandler.hpp"
#include <vector>
#include <stdlib.h>
#include <iostream>

#include <string.h>
#include <errno.h>

IOHandler::IOHandler(Network::TCPSocket *sock) : m_file(NULL), m_sock(sock) {}

IOHandler::~IOHandler()
{
   if (m_file)
   {
      std::cout << "-- CLOSE: " << m_path << std::endl;
      fclose(m_file);
   }
   delete m_sock;
}

IOHandler::Command IOHandler::get_command()
{
   char buf[max_proto_len + 1] = {0};

   ssize_t ret = m_sock->recv(buf, max_proto_len);

   if (ret <= 0)
   {
      //std::cerr << "Received no data. " << ret << " " << strerror(errno) << std::endl;
      return ERROR;
   }

   std::cerr << "Got command: " << buf << std::endl;

   const char *first = strstr(buf, "CMD_");
   if (first == NULL)
      return ERROR;

   first += strlen("CMD_");

   if (!strcmp(first, "FOPEN"))
      return FOPEN;
   if (!strcmp(first, "FCLOSE"))
      return FCLOSE;
   if (!strcmp(first, "FWRITE"))
      return FWRITE;
   if (!strcmp(first, "FREAD"))
      return FREAD;
   if (!strcmp(first, "FTELL"))
      return FTELL;
   if (!strcmp(first, "FSEEK"))
      return FSEEK;
   if (!strcmp(first, "UNGETC"))
      return UNGETC;
   if (!strcmp(first, "FEOF"))
      return FEOF;

   return ERROR;
}

bool IOHandler::get_argument(int64_t& val)
{
   std::cerr << "get_argument (int)" << std::endl;
   char buf[max_proto_len + 1] = {0};
   ssize_t ret = m_sock->recv(buf, max_proto_len);
   if (ret <= 0)
      return false;

   long len = strtol(buf, NULL, 10);
   std::cerr << "LEN: " << len << std::endl;
   std::vector<char> arg_buf(len + 1);

   ret = m_sock->recv(&arg_buf[0], len);
   if (ret <= 0)
      return false;

   val = (int64_t)strtoll(&arg_buf[0], NULL, 10);
   std::cerr << "VAL: " << val << std::endl;
   return true;
}

bool IOHandler::get_argument(std::string& str)
{
   std::cerr << "get_argument (string)" << std::endl;
   char buf[max_proto_len + 1] = {0};
   ssize_t ret = m_sock->recv(buf, max_proto_len);
   if (ret <= 0)
      return false;

   long len = strtol(buf, NULL, 10);
   std::vector<char> arg_buf(len + 1);

   ret = m_sock->recv(&arg_buf[0], len);
   if (ret <= 0)
      return false;

   str = &arg_buf[0];
   std::cerr << "STR: " << str << std::endl;

   return true;
}

bool IOHandler::return_argument(int64_t ret)
{
   std::cerr << "Return argument!: " << ret << std::endl;
   char buf[max_proto_len + 1] = {0};

   snprintf(buf, max_proto_len + 1, "%0*lld", max_proto_len, (long long)ret);
   ssize_t rc = m_sock->send(buf, max_proto_len);
   if (rc <= 0)
      return false;
   return true;
}

bool IOHandler::fopen_exec()
{
   std::cerr << "Running fopen_exec" << std::endl;

   std::string path, mode;
   if (!get_argument(path, mode))
      return false;

   std::cout << "-- OPEN: " << path << std::endl;
   m_file = fopen(path.c_str(), mode.c_str());

   if (m_file == NULL)
      std::cout << "-- FAIL OPEN: " << path << std::endl;

   m_path = path;
   return return_argument(m_file != NULL ? 0 : -1);
}

bool IOHandler::fclose_exec()
{
   if (m_file == NULL)
      return false;

   std::cout << "-- CLOSE: " << m_path << std::endl;
   fclose(m_file);
   return true;
}

bool IOHandler::fwrite_exec()
{
   if (!m_file)
      return false;

   int64_t size;
   if (!get_argument(size))
      return false;

   std::vector<char> buf(size);
   int64_t rc;
   if ((rc = read_all(&buf[0], size)) == 0)
      return false;

   rc = fwrite(&buf[0], 1, rc, m_file);
   return return_argument(rc);
}

bool IOHandler::fread_exec()
{
   if (!m_file)
      return false;

   int64_t size;
   if (!get_argument(size))
      return false;

   std::vector<char> buf(size);
   int64_t rc = fread(&buf[0], 1, size, m_file);

   if (!return_argument(rc))
      return false;

   if (write_all(&buf[0], rc) < rc)
      return false;
   return true;
}

bool IOHandler::ftell_exec()
{
   if (!m_file)
      return false;

   int64_t ret = ftell(m_file);
   if (!return_argument(ret))
      return false;
   return true;
}

bool IOHandler::fseek_exec()
{
   if (!m_file)
      return false;

   int64_t offset, whence;
   if (!get_argument(offset, whence))
      return false;

   return return_argument(fseek(m_file, offset, whence));
}

bool IOHandler::ungetc_exec()
{
   if (!m_file)
      return false;

   int64_t c;
   if (!get_argument(c))
      return false;

   return return_argument(ungetc(c, m_file));
}

bool IOHandler::feof_exec()
{
   if (!m_file)
      return false;

   return return_argument(feof(m_file));
}

void IOHandler::run()
{
   for(;;)
   {
      Command cmd = get_command();

      if (cmd == ERROR)
      {
         //std::cerr << "Failed to get command." << std::endl;
         break;
      }

      bool ret = false;
      switch (cmd)
      {
         case FOPEN:
            ret = fopen_exec();
            break;
         case FCLOSE:
            ret = fclose_exec();
            m_file = NULL;
            break;
         case FWRITE:
            ret = fwrite_exec();
            break;
         case FREAD:
            ret = fread_exec();
            break;
         case FTELL:
            ret = ftell_exec();
            break;
         case FSEEK:
            ret = fseek_exec();
            break;
         case UNGETC:
            ret = ungetc_exec();
            break;
         default:
            break;
      }
      if (!ret)
      {
         std::cerr << "Command failed ..." << std::endl;
         break;
      }
   }
}

#define MAX_PACKET_SIZE (1024 << 10)
size_t IOHandler::write_all(const void *data, size_t bytes)
{
   size_t written = 0;
   while (written < bytes)
   {
      size_t write_amt = bytes - written > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : bytes - written;

      ssize_t rc = m_sock->send((const char*)data + written, write_amt);
      if (rc <= 0)
         return written;

      written += rc;
   }
   return written;
}

size_t IOHandler::read_all(void *data, size_t bytes)
{
   size_t has_read = 0;
   while (has_read < bytes)
   {
      size_t read_amt = bytes - has_read > MAX_PACKET_SIZE ? MAX_PACKET_SIZE : bytes - has_read;

      ssize_t rc = m_sock->recv((char*)data + has_read, read_amt);
      if (rc <= 0)
         return has_read;

      has_read += rc;
   }
   return has_read;
}


