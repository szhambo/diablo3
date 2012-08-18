////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Necr0Potenc3
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

#include <windows.h>
#include <stdio.h>
#include <Psapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <algorithm>
#include "inject.h"

using namespace std;

#pragma comment(lib, "Psapi.lib")

BOOL GetProcessModules( DWORD dwPID, vector<MODULEENTRY32> &vModulesList )
{
	vModulesList.clear();

	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
	if( hModuleSnap == INVALID_HANDLE_VALUE )
	{
		return( FALSE );
	}

	// Set the size of the structure before using it.
	me32.dwSize = sizeof( MODULEENTRY32 );

	// Retrieve information about the first module,
	// and exit if unsuccessful
	if( !Module32First( hModuleSnap, &me32 ) )
	{
		CloseHandle( hModuleSnap );     // Must clean up the snapshot object!
		return( FALSE );
	}

	// Now walk the module list of the process,
	// and display information about each module
	do
	{
		vModulesList.push_back(me32);
	} while( Module32Next( hModuleSnap, &me32 ) );

	// Don't forget to clean up the snapshot object.
	CloseHandle( hModuleSnap );
	return( TRUE );
}

vector<MODULEENTRY32>::iterator FindModuleByExePath(vector<MODULEENTRY32> &vModulesList, LPTSTR szExePath)
{
	vector<MODULEENTRY32>::iterator vIter;
	for (vIter = vModulesList.begin(); vIter != vModulesList.end(); vIter++) {
		if (!_stricmp(szExePath, vIter->szExePath)) {
			return vIter;
		}
	}
	return vModulesList.end();
}

bool IsModuleExist(DWORD dwPID, LPTSTR szExePath)
{
	vector<MODULEENTRY32> vModulesList;
	if ( GetProcessModules( dwPID, vModulesList ) ) {
		if ( FindModuleByExePath(vModulesList, szExePath) != vModulesList.end() ) {
			return true;
		}
	}
	return false;
}

int main(int args, char *argv[])
{
	printf("Trying to acquire Debug Privileges\n\n");

	HANDLE TokenHandle;
	OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &TokenHandle);

	LUID Luid;
	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Luid); // "SeDebugPrivilege"

	TOKEN_PRIVILEGES NewState;
	NewState.PrivilegeCount = 1;
	NewState.Privileges[0].Luid = Luid;
	NewState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if(AdjustTokenPrivileges(TokenHandle, FALSE, &NewState, NULL, NULL, NULL) == 0)
	{
		printf("Failed to acquire Debug Privileges\n\n");
		system("pause");
		return 0;
	}

	HWND hWnd = NULL;
	DWORD dwProcessId = 0;
	char path[MAX_PATH];
	vector<DWORD> dwPIDs;
	string d3path;
	
	char lpBuffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, lpBuffer);
	strcat_s(lpBuffer, "\\apihook.dll");

	printf("Searching for Diablo III window...\n\n");

	dwPIDs.clear();
	while(true) {
		hWnd = FindWindow(NULL, "Diablo III");
		if(hWnd == NULL) hWnd = FindWindow(NULL, "°µºÚÆÆ‰ÄÉñIII");
		if(hWnd == NULL) hWnd = FindWindow(NULL, "ºÚ°µÆÆ»µÉñIII");

		if(hWnd != NULL) {
			GetWindowThreadProcessId(hWnd, &dwProcessId);
			if ( find(dwPIDs.begin(), dwPIDs.end(), dwProcessId) ==  dwPIDs.end() ) {
				if ( d3path.empty() ) {
					HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
					GetProcessImageFileNameA(proc, path, sizeof(path));
					CloseHandle(proc);
					d3path = path;
				} else if( d3path.find("Diablo III.exe") != string::npos ) {
					if ( ! IsModuleExist(dwProcessId, lpBuffer) ) {

						printf("Found Process of diablo 3, PID %d\n", dwProcessId);

						printf("Injecting PID %d with %s\n", dwProcessId, lpBuffer);

						DLLInject(dwProcessId, lpBuffer);

						Sleep(1000);

						if ( IsModuleExist(dwProcessId, lpBuffer) ) {
							printf("Found module in (PID %d), Injecting finished!\n\n", dwProcessId);
							dwPIDs.push_back(dwProcessId);
							while(true) {
							}
						}
					}
				} else if ( ! d3path.empty() ) {
					printf("Found Process of diablo 3, PID %d, but not Diablo III.exe\n\n", dwProcessId);
				} else {
					printf("Found Process of diablo 3, PID %d, can not get path of Diablo III.exe\n\n", dwProcessId);
				}
			}
		}

		Sleep(50);
	}

	return 0;
}
