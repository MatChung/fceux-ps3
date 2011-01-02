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
 * GraphicsTypes.h
 *
 *  Created on: Nov 10, 2010
 *      Author: halsafar
 */

#ifndef GRAPHICSTYPES_H_
#define GRAPHICSTYPES_H_

typedef struct _Vertex
{
        float x;
        float y;
        float z;
} Vertex;

typedef struct _TextureCoord
{
        float u;
        float v;
} TextureCoord;

typedef struct _Quad
{
        Vertex v1;
        Vertex v2;
        Vertex v3;
        Vertex v4;
        TextureCoord t1;
        TextureCoord t2;
        TextureCoord t3;
        TextureCoord t4;
} Quad;

typedef struct _Rect
{
	float x;
	float y;
	float w;
	float h;
} Rect;


#endif /* GRAPHICSTYPES_H_ */
