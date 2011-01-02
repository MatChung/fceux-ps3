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
 * IGraphics.h
 *
 *  Created on: Oct 26, 2010
 *      Author: halsafar
 */

#ifndef __IGRAPHICS_H__
#define __IGRAPHICS_H__

template <class FLOAT_TYPE=float, class INT_TYPE=int>
class IGraphics
{
public:
	virtual ~IGraphics() {}

	virtual void Init() = 0;
	virtual void Deinit() = 0;

	virtual void InitDbgFont() = 0;
	virtual void DeinitDbgFont() = 0;

	virtual FLOAT_TYPE GetDeviceAspectRatio() = 0;
	virtual INT_TYPE GetResolutionWidth() = 0;
	virtual INT_TYPE GetResolutionHeight() = 0;
private:
};

#endif
