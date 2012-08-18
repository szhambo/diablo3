/******************************************************************************\
* 
* 
*  Copyright (C) 2000 Bruno 'Beosil' Heidelberger
* 
* 
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
* 
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
* 
* 	n0p3:
* 	Somewhere in 2004  -- got the code from Sniffy, changed it to pure C
* 
\******************************************************************************/


/* based on Beosil's log class */

#include <windows.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <Shlwapi.h>
#include "Logger.h"


static FILE *logFile = NULL;

void MBOut(const char *title, const char *msg, ...)
{
	char final[4096];
	va_list list;
	va_start(list, msg);
	vsprintf((char*)final, msg, list);
	va_end(list);

	MessageBox(NULL, final, title, 0);
	
	return;
}

void OpenLog(void)
{
	char LogPath[4096];
	time_t TmpTime = 0;
	struct tm *TimeInfo = NULL;

	/* get time stuff */
	time(&TmpTime);
	TimeInfo = localtime(&TmpTime);

	/* let those lab monkeys free! */
	char lpDllPath[MAX_PATH];
	if ( GetModuleFileName(GetModuleHandle("apihook.dll"), lpDllPath, sizeof(lpDllPath)) > 0 ) {	// 获取当前apihook.dll文件路径.
		if ( ::PathRemoveFileSpec(lpDllPath) == TRUE ) {
			sprintf(LogPath, "%s\\NetHook(%02d-%02d-%04d)(%02d-%02d-%02d).txt", lpDllPath, TimeInfo->tm_mday, TimeInfo->tm_mon + 1, TimeInfo->tm_year + 1900, TimeInfo->tm_hour, TimeInfo->tm_min, TimeInfo->tm_sec);
			logFile = fopen(LogPath, "wb");
		}
	}

	return;
}

void CloseLog(void)
{
	if(logFile) fclose(logFile);

	return;
}


void LogPrint(const char *strFormat, ...)
{
	if(!logFile) return;
	
	char final[4096];
	va_list args;

	if(!logFile) return;

	va_start(args, strFormat);
	memset(final, 0, sizeof(final));
	vsprintf(final, strFormat, args);
	va_end(args);

	fprintf(logFile, final);
	fflush(logFile);

	return;
}

void LogDump(unsigned char *pBuffer, int length)
{
	int actLine = 0, actRow = 0;

	if(!logFile) return;

	for(actLine = 0; actLine < (length / 16) + 1; actLine++)
	{
		fprintf(logFile, "%04x: ", actLine * 16);

		for(actRow = 0; actRow < 16; actRow++)
		{
			if(actLine * 16 + actRow < length) fprintf(logFile, "%02x ", (unsigned int)((unsigned char)pBuffer[actLine * 16 + actRow]));
			else fprintf(logFile, "-- ");
		}

		fprintf(logFile, ": ");

		for(actRow = 0; actRow < 16; actRow++)
		{
			if(actLine * 16 + actRow < length) fprintf(logFile, "%c", isprint(pBuffer[actLine * 16 + actRow]) ? pBuffer[actLine * 16 + actRow] : '.');
		}

		fprintf(logFile, "\r\n");
	}

	fprintf(logFile, "\r\n");

	fflush(logFile);

	return;
}
