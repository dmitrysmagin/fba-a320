/*
 * FinalBurn Alpha for Dingux/OpenDingux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _BURNER_H_
#define _BURNER_H_

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tchar.h"

#define BZIP_MAX (8)								// Maximum zip files to search through
#define DIRS_MAX (8)								// Maximum number of directories to search

#include "burn.h"

#ifndef MAX_PATH
#define	MAX_PATH 256
#endif

// drv.cpp
extern char szAppRomPaths[DIRS_MAX][MAX_PATH];
int DrvInitCallback(); // needed for StatedLoad/StatedSave

// paths.cpp
extern char szAppHomePath[MAX_PATH];
extern char szAppSavePath[MAX_PATH];
void BurnPathsInit();

// state.cpp
int BurnStateLoadEmbed(FILE* fp, int nOffset, int bAll, int (*pLoadGame)());
int BurnStateLoad(const char * szName, int bAll, int (*pLoadGame)());
int BurnStateSaveEmbed(FILE* fp, int nOffset, int bAll);
int BurnStateSave(const char * szName, int bAll);

// stated.cpp
extern int nSavestateSlot;
int StatedLoad(int nSlot);
int StatedSave(int nSlot);

// zipfn.cpp
struct ZipEntry { char* szName;	unsigned int nLen; unsigned int nCrc; };

int ZipOpen(char* szZip);
int ZipClose();
int ZipGetList(struct ZipEntry** pList, int* pnListCount);
int ZipLoadFile(unsigned char* Dest, int nLen, int* pnWrote, int nEntry);

// bzip.cpp
#define BZIP_STATUS_OK		(0)
#define BZIP_STATUS_BADDATA	(1)
#define BZIP_STATUS_ERROR	(2)

int BzipOpen(bool);
int BzipClose();
int BzipInit();
int BzipExit();
int BzipStatus();

#endif // _BURNER_H_


