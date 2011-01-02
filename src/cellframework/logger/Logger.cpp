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
 * Logger.cpp
 *
 *  Created on: Nov 2, 2010
 *      Author: halsafar
 */

#include "Logger.h"

// statics
std::auto_ptr<ILogger> Logger::_netLogger;

ILogger& Logger::GetLogger()
{
	// ideally parse some conf file here to setup the logger
	// for now just return this
	// FIXME: really take some time to fix this class
#ifdef CELL_DEBUG
	if (_netLogger.get() == NULL)
	{
		_netLogger.reset(new NetLogger(PS3_DEBUG_IP, PS3_DEBUG_PORT));
	}
#endif

	return *_netLogger;
}

