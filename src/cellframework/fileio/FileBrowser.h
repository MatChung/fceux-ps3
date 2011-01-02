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
 * FileBrowser.h
 *
 *  Created on: Oct 29, 2010
 *      Author: halsafar
 */

#ifndef FILEBROWSER_H_
#define FILEBROWSER_H_

#define MAXJOLIET 255

#include <string>
#include <vector>
#include <stack>

#include <sys/types.h>

#include <cell/cell_fs.h>

using namespace std;


//FIXME: shouldnt need this, ps3 has its own CellFsDirEnt and CellFsDirectoryEntry
//		-- the latter should be switched to eventually
typedef struct
{
        string dir;
        string extensions;
        int types;
        uint32_t numEntries;
        int size;
} DirectoryInfo;

/*
typedef struct
{
        size_t length; // file length
        time_t mtime; // file modified time
        int isdir; // 0 - file, 1 - directory
        CellFsDirent dirent;
        char filename[MAXJOLIET + 1]; // full filename
        char displayname[MAXJOLIET + 1]; // name for browser display
        int filenum; // file # (for 7z support)
        int icon; // icon to display
} BrowserEntry;
*/


typedef CellFsDirent DirectoryEntry;


struct less_than_key
{
	// yeah sucks, not using const
    inline bool operator() (DirectoryEntry* a, DirectoryEntry* b)
    {
    	// dir compare to file, let us always make the dir less than
    	if ((a->d_type == CELL_FS_TYPE_DIRECTORY && b->d_type == CELL_FS_TYPE_REGULAR))
    	{
    		return true;
    	}
    	else if (a->d_type == CELL_FS_TYPE_REGULAR && b->d_type == CELL_FS_TYPE_DIRECTORY)
    	{
    		return false;
    	}

    	// FIXME: add a way to customize sorting someday
    	// 	also add a ignore filename, sort by extension

    	// use this to ignore extension
    	if (a->d_type == CELL_FS_TYPE_REGULAR && b->d_type == CELL_FS_TYPE_REGULAR)
    	{
			char *pIndex1 = strrchr(a->d_name, '.');
			char *pIndex2 = strrchr(b->d_name, '.');

			// null the dots
			if (pIndex1 != NULL)
			{
				*pIndex1 = '\0';
			}

			if (pIndex2 != NULL)
			{
				*pIndex2 = '\0';
			}

			// compare
			int retVal = strcasecmp(a->d_name, b->d_name);

			// restore the dot
			if (pIndex1 != NULL)
			{
				*pIndex1 = '.';
			}

			if (pIndex2 != NULL)
			{
				*pIndex2 = '.';
			}
			return retVal < 0;
    	}

    	// both dirs at this points btw
    	return strcasecmp(a->d_name, b->d_name) < 0;
    }
};


class FileBrowser
{
public:
	FileBrowser(string startDir);
	FileBrowser(string startDir, string extensions);
	FileBrowser(string startDir, int types, string extensions);
	~FileBrowser();

	void Destroy();

	DirectoryEntry* GetCurrentEntry();
	uint32_t GetCurrentEntryIndex();
	static string GetExtension(string filename);

	bool IsCurrentAFile();
	bool IsCurrentADirectory();

	// Set wrapping on or off
	// Wrapping: if the entry that _currentSelected points
	// to exceeds the maximum amount of entries in that
	// directory, it will go back to either the first entry
	// or the last
	void SetEntryWrap(bool wrapvalue);

	void IncrementEntry();
	void DecrementEntry();
	void GotoEntry(uint32_t i);

	DirectoryInfo GetCurrentDirectoryInfo();

	void PushDirectory(string path, int types, string extensions);
	void PopDirectory();
	uint32_t DirectoryStackCount();

	DirectoryEntry* operator[](uint32_t i)
	{
		return _cur[i];
	}
private:
	// currently select browser entry
	uint32_t _currentSelected;

	// current file descriptor, used for reading entries
	int _fd;

	// wrap boolean variable
	// if true -
	// IncrementEntry will set _currentSelected to 0 if _currentSelected is bigger than _cur.size()
	// DecrementEntry will set _currentSelected to _cur.size() - 1 if _currentSelected is bigger than or equal to cur.size()
	// if false -
	// IncrementEntry will simply increment the entry without any checks 
	// DecrementEntry will simply decrement the entry without any checks
	// It's up to the application to properly safeguard against trying to access an entry that exceeds the bounds
	// of the entry list
	bool m_wrap;

	// info of the current directory
	DirectoryInfo _dir;

	// current file listing
	vector<DirectoryEntry*> _cur;

	// dir stack for ez traversal
	stack<DirectoryInfo> _dirStack;

	bool ParseDirectory(string path, int types, string extensions);
	void DeleteCurrentEntrySet();
};


#endif /* FILEBROWSER_H_ */
