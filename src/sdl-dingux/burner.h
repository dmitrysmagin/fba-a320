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
#include <ctype.h>

#include "tchar.h"

#define BZIP_MAX (8)								// Maximum zip files to search through
#define DIRS_MAX (8)								// Maximum number of directories to search

#include "burn.h"

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

// dat.cpp
#define APP_TITLE "FB Alpha"
#define APP_DESCRIPTION "Emulator for MC68000/Z80 based arcade games"
char* DecorateGameName(unsigned int nBurnDrv);
int write_datfile(int nDatType, FILE* fDat);
int create_datfile(TCHAR* szFilename, int nDatType);

// fba_player.cpp
extern char szAppBurnVer[16];

// config.cpp
typedef struct
{
	int option_sound_enable;
	int option_rescale;
	int option_rotate;
	int option_samplerate;
	int option_showfps;
	int option_frameskip;
	int option_68kcore;
	int option_z80core;
	int option_sense;
	int option_useswap;
	char option_frontend[MAX_PATH];
	int option_create_lists;
} CFG_OPTIONS;

typedef struct
{
	int up;
	int down;
	int left;
	int right;
	int fire1;
	int fire2;
	int fire3;
	int fire4;
	int fire5;
	int fire6;
	int coin1;
	int coin2;
	int start1;
	int start2;
	int pause;
	int quit;
	int qsave;
	int qload;
} CFG_KEYMAP;

extern CFG_OPTIONS config_options;
extern CFG_KEYMAP config_keymap;

int ConfigAppLoad();
int ConfigAppSave();
int ConfigGameLoad();
int ConfigGameSave();

// drv.cpp
extern char szAppRomPaths[DIRS_MAX][MAX_PATH];
int DrvInitCallback(); // needed for StatedLoad/StatedSave

// paths.cpp
extern char szAppHomePath[MAX_PATH];
extern char szAppSavePath[MAX_PATH];
extern char szAppConfigPath[MAX_PATH];
void BurnPathsInit();

// state.cpp
int BurnStateLoadEmbed(FILE* fp, int nOffset, int bAll, int (*pLoadGame)());
int BurnStateLoad(const char * szName, int bAll, int (*pLoadGame)());
int BurnStateSaveEmbed(FILE* fp, int nOffset, int bAll);
int BurnStateSave(const char * szName, int bAll);
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


