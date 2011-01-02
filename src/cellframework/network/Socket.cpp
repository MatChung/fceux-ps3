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





#include "Socket.hpp"
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/poll.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>

#ifndef __CELLOS_LV2__
#include <fcntl.h>
#endif

namespace Network {

   Socket::Socket(int socktype) : m_type(socktype), m_alive(true)
   {
      m_fd = socket(AF_INET, socktype, 0);
      if (m_fd < 0)
         alive(false);
   }

   void Socket::grab(Socket& in)
   {
      //std::cout << "TCPSocket::grab" << std::endl;
      int other_fd = in.fd();
      //std::cout << "FD:" << in.fd() << std::endl;
      fd(other_fd);

      //std::cout << "Blocking? " << in.is_blocking() << std::endl;
      blocking(in.is_blocking());
      in.fd(-1);
   }

   Socket::Socket(int socktype, int fd) : m_type(socktype), m_alive(true)
   {
      m_fd = fd;
      if (m_fd < 0)
         alive(false);
   }

   Socket::~Socket()
   {
      if (m_fd >= 0)
      {
#ifdef __CELLOS_LV2__
         socketclose(m_fd);
#else
         close(m_fd);
#endif
      }
   }

   bool Socket::alive() const
   {
      return m_alive;
   }

   void Socket::alive(bool is_alive)
   {
      m_alive = is_alive;
      //std::cout << "Setting alive: " << m_alive << std::endl;
   }

   bool Socket::is_blocking() const
   {
#ifdef __CELLOS_LV2__
      int i;
      socklen_t len = sizeof(i);
      getsockopt(m_fd, SOL_SOCKET, SO_NBIO, &i, &len);
      return !i;
#else
      return !(fcntl(m_fd, F_GETFL) & O_NONBLOCK);
#endif
   }

   void Socket::blocking(bool block)
   {
      //std::cout << "Socket::blocking: " << block << std::endl;
      if (!alive())
         return;

#ifdef __CELLOS_LV2__
      int i = !block;
      setsockopt(m_fd, SOL_SOCKET, SO_NBIO, &i, sizeof(i));
#else
      if (block)
         fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL) & (~O_NONBLOCK));
      else
         fcntl(m_fd, F_SETFL, fcntl(m_fd, F_GETFL) | O_NONBLOCK);
#endif
      //std::cout << "Socket::blocking: setting block" << std::endl;
      //std::cout << "Socket::blocking: is_blocking()?: " << is_blocking() << std::endl;
   }

   bool Socket::operator==(const Socket& in) const
   {
      return m_fd == in.m_fd && m_fd != -1;
   }

   int Socket::fd() const
   {
      return m_fd;
   }

   void Socket::fd(int sock)
   {
      m_fd = sock;
      if (m_fd < 0)
         alive(false);
      else
         alive(true);
   }

   // Sockets need to implement this properly.
   ssize_t Socket::recv(void *, size_t)
   {
      return -1;
   }

   // Sockets need to implement this properly.
   ssize_t Socket::send(const void*, size_t)
   {
      return -1;
   }

   ssize_t Socket::send(const std::string& msg)
   {
      return send(msg.c_str(), msg.size());
   }

   ssize_t Socket::send(const char *msg)
   {
      return send(msg, strlen(msg));
   }
}

