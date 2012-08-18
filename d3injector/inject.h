DWORD GetProcessThreads(DWORD pid, DWORD *buf, int *pbytes);
void NumOut(char *format, ...);
bool ShowError(DWORD ercode);
DWORD GetAllSystemProcess(char str[][260], DWORD *pid, int *pbytes);
bool DLLInject(DWORD pid, const char *path);
bool WriteMemPatch(HANDLE hProcess, HANDLE hThread, const char *path);