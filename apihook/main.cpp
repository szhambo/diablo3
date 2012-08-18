////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2004 Necr0Potenc3
// release date: January 25th, 2004
// dcavalcanti@bigfoot.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
////////////////////////////////////////////////////////////////////////////////

#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <Shlwapi.h>
#include <string>
#include <algorithm>
#include "Logger.h"
#include "resource.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Shlwapi.lib")

typedef int (WINAPI* t_WSASend)(SOCKET,LPWSABUF,DWORD,LPDWORD,DWORD,LPWSAOVERLAPPED,LPWSAOVERLAPPED_COMPLETION_ROUTINE );

t_WSASend o_WSASend;

int WINAPI hook_WSASend(SOCKET s,LPWSABUF lpBuffers,DWORD dwBufferCount,LPDWORD lpNumberOfBytesSent,DWORD dwFlags,LPWSAOVERLAPPED lpOverlapped,LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);


void *DetourFunction(BYTE *src, const BYTE *dst, const int len) // credits to gamedeception
{
	BYTE *jmp = (BYTE*)malloc(len+5);
	DWORD dwback;
	VirtualProtect(src, len, PAGE_READWRITE, &dwback);
	memcpy(jmp, src, len); jmp += len;
	jmp[0] = 0xE9;
	*(DWORD*)(jmp+1) = (DWORD)(src+len - jmp) - 5;
	src[0] = 0xE9;
	*(DWORD*)(src+1) = (DWORD)(dst - src) - 5;
	VirtualProtect(src, len, dwback, &dwback);
	return (jmp-len);
}

char realLoc[256] = {0};
char fakeLoc[256] = {0};
char *locs[] = {"enUS", "esMX", "ptBR", "koKR", "zhCN", "zhTW", "deDE", "esES", "frFR", "itIT", "plPL", "ruRU", "enGB", "jaJP"};
char lpDllPath[MAX_PATH];
char lpIniPath[MAX_PATH];

BOOL CALLBACK MainDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
	switch(wMsg)
	{
		case WM_INITDIALOG:
		{
			HWND cboreal = GetDlgItem(hDlg, IDC_COMBOREAL);
			HWND cbofake = GetDlgItem(hDlg, IDC_COMBOFAKE);

			for(int i = 0; i < 14; i++)
			{
				SendMessage(cboreal, CB_ADDSTRING, 0, (LPARAM)locs[i]);
				SendMessage(cbofake, CB_ADDSTRING, 0, (LPARAM)locs[i]);
			}

			SendMessage(cboreal, CB_SETCURSEL, 5, 0); // default zhTW
			SendMessage(cbofake, CB_SETCURSEL, 2, 0); // default ptBR

			SetFocus(GetDlgItem(hDlg, IDC_BUTTONOK));
		} break;
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))	// msg is kept in wParam
			{
				case IDC_BUTTONOK:
				{
					GetDlgItemText(hDlg, IDC_COMBOREAL, realLoc, sizeof(realLoc));
					GetDlgItemText(hDlg, IDC_COMBOFAKE, fakeLoc, sizeof(fakeLoc));
					LogPrint("Save location to Ini File: %s\r\n", lpIniPath);
					::WritePrivateProfileString( "Language", "realLoc", realLoc, lpIniPath );
					::WritePrivateProfileString( "Language", "fakeLoc", fakeLoc, lpIniPath );

					EndDialog(hDlg, 0);
				}
			}
		} break;
	}

	return FALSE;
}


BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			DisableThreadLibraryCalls(hModule); //keeps it from being re-called
//			OpenLog(); LogPrint("Starting Logger\r\n");
			if ( GetModuleFileName(GetModuleHandle("apihook.dll"), lpDllPath, sizeof(lpDllPath)) > 0 ) {	// 获取当前apihook.dll文件路径.
				if ( ::PathRemoveFileSpec(lpDllPath) == TRUE ) {
					strcpy_s(lpIniPath, sizeof(lpIniPath), lpDllPath);
					strcat_s(lpIniPath, sizeof(lpIniPath), "\\apihook.ini");	// 生成apihook.ini文件路径
					LogPrint("Ini File: %s\r\n", lpIniPath);
				}
			}

			char realLoc_ini[256] = {0};
			char fakeLoc_ini[256] = {0};
			if ( strlen(lpIniPath) > 0 ) {
				::GetPrivateProfileString( "Language", "realLoc", "", realLoc_ini, sizeof(realLoc_ini), lpIniPath );
				::GetPrivateProfileString( "Language", "fakeLoc", "", fakeLoc_ini, sizeof(fakeLoc_ini), lpIniPath );
				if ( strlen(realLoc_ini) > 0 && strlen(fakeLoc_ini) > 0 ) {
					strcpy_s( realLoc, realLoc_ini);
					strcpy_s( fakeLoc, fakeLoc_ini);
					LogPrint("realLoc_ini: %s, fakeLoc_ini: %s\r\n", realLoc_ini, fakeLoc_ini);
				}else {
					DialogBox(hModule, MAKEINTRESOURCE(IDD_DIALOGBAR), 0, MainDlgProc);
				}
			}

			LogPrint("realLoc: %s, fakeLoc: %s\r\n", realLoc, fakeLoc);

			o_WSASend  = (t_WSASend)DetourFunction((PBYTE)GetProcAddress(GetModuleHandle("ws2_32.dll"), "WSASend"), (PBYTE)hook_WSASend, 5);
			return true;
		} break;
	case DLL_THREAD_ATTACH: break;
	case DLL_THREAD_DETACH: break;
	case DLL_PROCESS_DETACH:
		{		
			LogPrint("Closing Logger");
//			CloseLog();
		} break;
	}
    return TRUE;
}

int WINAPI hook_WSASend(SOCKET s,LPWSABUF lpBuffers,DWORD dwBufferCount,LPDWORD lpNumberOfBytesSent,DWORD dwFlags,LPWSAOVERLAPPED lpOverlapped,LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	//for(DWORD i = 0; i < dwBufferCount; i++)
	//{
	//	LogPrint("Buffer: %d\r\n", i);
	//	LogDump((unsigned char*)lpBuffers->buf, lpBuffers->len);
	//}

	if(lpBuffers->len > 25 && !memcmp(lpBuffers->buf + 21, realLoc, 4))
	{
		LogDump((unsigned char*)lpBuffers->buf, lpBuffers->len);
		LogPrint("\r\nDetected locale %s, changing to %s. New Buffer:\r\n", realLoc, fakeLoc);
		memcpy(lpBuffers->buf + 21, fakeLoc, 4);
		LogDump((unsigned char*)lpBuffers->buf, lpBuffers->len);
	}
 	else if(lpBuffers->len > 25)
	{
		LogPrint("Text @ pos %c%c%c%c\r\n", lpBuffers->buf[21], lpBuffers->buf[22], lpBuffers->buf[23], lpBuffers->buf[24]);
	}

	return o_WSASend(s,lpBuffers,dwBufferCount,lpNumberOfBytesSent,dwFlags,lpOverlapped,lpCompletionRoutine);
}
