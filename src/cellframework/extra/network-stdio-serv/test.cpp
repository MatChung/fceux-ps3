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





#include "../../network-stdio/net_stdio.h"
#include <assert.h>

int main()
{
   char buffer[1024] = {0};
   net_stdio_enable(1);

   net_stdio_set_target("localhost", 9001);

#if 0
   FILE *file = fopen("/tmp/test.txt", "wb");
   assert(file);

   fprintf(file, "HAI U\n");
   printf("ftell: %ld\n", ftell(file));
   fprintf(file, "WHAT'S THIS?!\n");
   printf("ftell: %ld\n", ftell(file));
   fseek(file, SEEK_SET, 0);
   fprintf(file, "OMG");
   printf("ftell: %ld\n", ftell(file));

   fclose(file);

   file = fopen("/tmp/test.txt", "rb");
   assert(file);
   size_t ret = fread(buffer, 1, 1024, file);

   printf("ret: %zu\n", ret);
   puts(buffer);
   rewind(file);
   ret = fread(buffer, 1, 1024, file);

   printf("ret: %zu\n", ret);
   puts(buffer);

   fclose(file);

   // Copy file. Take a big one to test transfer performance.
   file = fopen("/tmp/test1.bin", "rb");
   assert(file);
   fseek(file, 0, SEEK_END);
   long len = ftell(file);
   rewind(file);

   char *buf = new char[len];
   printf("len: %ld\n", len);
   printf("fread: %zu\n", fread(buf, 1, len, file));
   fclose(file);

   file = fopen("/tmp/test2.bin", "wb");
   printf("fwrite: %zu\n", fwrite(buf, 1, len, file));
   fclose(file);

   delete buf;
#endif

   fprintf(stderr, ":V\n");
   fprintf(stdout, "hai\n");
   net_stdio_hook(stdout, "/tmp/stdout.txt");
   net_stdio_hook(stderr, "/tmp/stderr.txt");

   fprintf(stdout, ":D\n");
   net_stdio_hook(stdout, NULL);
   fprintf(stderr, ":D:D\n");
   fprintf(stdout, "wtf\n");
   net_stdio_hook(stdout, "/tmp/winrar.txt");
   fprintf(stdout, "lulz\n");

   return 0;
}
