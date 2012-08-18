#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>
#include "inject.h"

//not really useful but I needed it
bool ShowError(DWORD ercode)
{
	void *msgbuf;
	int n = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						ercode,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR)&msgbuf,
						0,
						NULL);
	//the error of an error...
	//redundancy rlz =P
	if(!n)
		ShowError(GetLastError());
	else
		NumOut("Error id: %d msg: %s", ercode, msgbuf);

	LocalFree(msgbuf);
	return 0;
}

//debugging purposes
void NumOut(char *format, ...)
{
   char final[4096]; memset(final, 0, sizeof(final));
   va_list args;
   va_start(args, format);
   vsprintf(final, format, args);
   va_end(args);
   MessageBox(0, final, "NumOut:", 0);
}

//dumps all procs exe name in str
//all pids in pid
//pbytes in == number of bytes of the buffers
//pbytes out == items placed in the buffers
DWORD GetAllSystemProcess(char str[][260], DWORD *pid, int *pbytes)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	int tmppbytes = 0;
	PROCESSENTRY32 procs;
	memset(&procs, 0, sizeof(procs));
	procs.dwSize = sizeof(procs);
	if(!Process32First(snap, &procs))
	{
		CloseHandle(snap);
		return GetLastError();
	}
	else
	{
		memcpy(&str[tmppbytes][0], procs.szExeFile, sizeof(procs.szExeFile));
		pid[tmppbytes] = procs.th32ProcessID;
		tmppbytes++;
		if(tmppbytes == *pbytes)
		{
			CloseHandle(snap);
			return 0;
		}
	}

	while(Process32Next(snap, &procs))
	{
		memcpy(&str[tmppbytes][0], procs.szExeFile, sizeof(procs.szExeFile));
		pid[tmppbytes] = procs.th32ProcessID;
		tmppbytes++;
		if(tmppbytes == *pbytes)
		{
            CloseHandle(snap);
			return 0;
		}
	}

	*pbytes = tmppbytes;
	CloseHandle(snap);
	return 0;
}


//pid being the Process ID of a process to patch
//buf is an int array
//pbytes is how many items are in the array, recommended is 20
DWORD GetProcessThreads(DWORD pid, DWORD *tid, int *pbytes)
{
	//DWORD pid = 4092;
	// Take a snapshot of all processes in the system.
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
	int tmppbytes = 0;
  	THREADENTRY32 threads;
	memset(&threads, 0, sizeof(threads));
	threads.dwSize = sizeof(threads);
	if(!Thread32First(snap, &threads))
	{
		CloseHandle(snap);
		return GetLastError();
	}
	else
	{
		if(threads.th32OwnerProcessID == pid) //quite impossible
		{
			tid[tmppbytes] = threads.th32ThreadID;
			tmppbytes++;
			if(tmppbytes == *pbytes)
			{
				CloseHandle(snap);
				return 0;
			}
		}
	}
	
	while(Thread32Next(snap, &threads))
	{
		if(threads.th32OwnerProcessID == pid)
		{
			tid[tmppbytes] = threads.th32ThreadID;
			//tid[tmppbytes] = threads.th32OwnerProcessID;
			tmppbytes++;
			if(tmppbytes == *pbytes)
			{
				CloseHandle(snap);
				return 0;
			}
		}
	}

	*pbytes = tmppbytes;
	CloseHandle(snap);
	return 0;
}

bool DLLInject(DWORD pid, const char *path)
{
	//this will go through all TIDs of a process
	//and get one we can use
    DWORD tid[20];
	int inbytes = 20;
	GetProcessThreads(pid, &tid[0], &inbytes);

	if(pid == GetCurrentProcessId())
	{
        NumOut("Process will suspend itself and lock");
		return 0;
	}

	DWORD tiduse=0;
	for(int i=0; i<inbytes; i++)
	{
		HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid[i]);
		if(!thread)
		{
			NumOut("ops");
			CloseHandle(thread);
		}
		else
		{
			SuspendThread(thread);
			CONTEXT ctx;
			ctx.ContextFlags = CONTEXT_FULL;
			GetThreadContext(thread, &ctx);
			ResumeThread(thread);
			CloseHandle(thread);
			
			//lower eips will crash the system
			if(ctx.Eip > 0x400000)
			{
				tiduse = tid[i];
				break; //one thread is good, break the loop
			}
			//NumOut("%d %X", tid[i], ctx.Eip);
		}
	}

	//do we have a good tid?
	if(!tiduse)
	{
		NumOut("No usable tid found");
		return 0;
	}

	//found a good tid already, now patch
	//excelent patching method

	HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if(!proc)
		proc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, pid);

	HANDLE thread = OpenThread(THREAD_ALL_ACCESS, FALSE, tiduse);

	bool ret = WriteMemPatch(proc, thread, path);
	CloseHandle(proc);
	CloseHandle(thread);
	return ret;
}

bool WriteMemPatch(HANDLE hProcess, HANDLE hThread, const char *path)
{
	//the sequence bellow was taken from ClientStarter.cpp of uoinjection.sf.net
	// On NT4+ I allocate needed memory with VirtualAllocEx, and
	// on 9x I use free space in PE header, because VirtualAllocEx is
	// not implemented there. I can't use PE header space under all OSes
	// because XP does not allow writing there even after calling VirtualProtectEx.

	DWORD PatchBase=(DWORD)VirtualAllocEx(hProcess,0,
		4096,MEM_COMMIT,PAGE_EXECUTE_READWRITE);

	if(PatchBase==0)
		PatchBase=0x400400;

	BOOL AllOk=TRUE;	// Flag that everything went ok

#define pokeb(addr,byte)	\
	{	\
		BYTE b=(BYTE)(byte);	\
		AllOk&=WriteProcessMemory(hProcess,(void*)(addr),&b,1,0);	\
	}

#define poked(addr,dword)	\
	{	\
		DWORD dw=(DWORD)(dword);	\
		AllOk&=WriteProcessMemory(hProcess,(void*)(addr),&dw,4,0);	\
	}

	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;
	SuspendThread(hThread); //VERY important, if the registers change and we set old regs back == crash
	AllOk&=GetThreadContext(hThread,&ctx);
	
	AllOk&=WriteProcessMemory(hProcess, (void*)(PatchBase+0x100), path, strlen(path)+1, 0);

	pokeb(PatchBase  ,0x9c);				// pushfd
	pokeb(PatchBase+1,0x60);				// pushad
	pokeb(PatchBase+2,0x68);				// push "address of dll path"
	poked(PatchBase+3,PatchBase+0x100);

	pokeb(PatchBase+7,0xe8);				// call LoadLibraryA, the DWORD for a call or a jmp is ((to)-(from)-5)
	poked(PatchBase+8,(char*)(GetProcAddress(GetModuleHandle("kernel32"),"LoadLibraryA"))-(PatchBase+7+5));

	pokeb(PatchBase+12,0x61);				// popad
	pokeb(PatchBase+13,0x9d);				// popfd

	pokeb(PatchBase+14,0xe9);				// jmp OldEIP
	poked(PatchBase+15,ctx.Eip-(PatchBase+14+5));

	if(!AllOk)
		return FALSE;

	ctx.Eip=PatchBase;
	AllOk&=SetThreadContext(hThread,&ctx);
	ResumeThread(hThread);

	return AllOk!=FALSE;
}