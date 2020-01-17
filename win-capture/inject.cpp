#include "inject.h"
#include <tchar.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")  

// SE_DEBUG_NAME
BOOL SetPrivilege(LPCTSTR name, BOOL on)
{
	HANDLE  token;
	LUID    luid;
	TOKEN_PRIVILEGES tp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
	{
		return FALSE;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
	{
		CloseHandle(token);
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = (on ? SE_PRIVILEGE_ENABLED : 0);

	if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(tp), NULL, NULL))
	{
		CloseHandle(token);
		return FALSE;
	}

	CloseHandle(token);
	return TRUE;
}

DWORD GetProcessIDByName(LPCTSTR processName)
{
	DWORD pid = 0;
	HANDLE hSnapShot = INVALID_HANDLE_VALUE;
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	if (!Process32First(hSnapShot, &pe))
	{
		CloseHandle(hSnapShot);
		return pid;
	}

	do
	{
		if (!_tcsicmp(processName, (LPCTSTR)pe.szExeFile))
		{
			pid = pe.th32ProcessID;
			break;
		}
	} while (Process32Next(hSnapShot, &pe));

	CloseHandle(hSnapShot);
	return pid;
}

BOOL InjectDLLByRemoteThread(DWORD pid, LPCTSTR dllPath)
{
	HANDLE hProcess = INVALID_HANDLE_VALUE;
	HANDLE hThread = INVALID_HANDLE_VALUE;
	LPVOID buf = NULL;
	DWORD bufSize = (DWORD)(_tcslen(dllPath) + 1) * sizeof(TCHAR);
	LPTHREAD_START_ROUTINE threadProc = NULL;

	if (!PathFileExists(dllPath))
	{
		return FALSE;
	}

	if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)))
	{
		return FALSE;
	}

	buf = VirtualAllocEx(hProcess, NULL, bufSize, MEM_COMMIT, PAGE_READWRITE);
	if (buf == NULL)
	{
		CloseHandle(hProcess);
		return FALSE;
	}

	if (!WriteProcessMemory(hProcess, buf, (LPVOID)dllPath, bufSize, NULL))
	{
		VirtualFreeEx(hProcess, buf, bufSize, MEM_DECOMMIT);
		CloseHandle(hProcess);
		return FALSE;
	}

	threadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "LoadLibraryW");
	if (threadProc == NULL)
	{
		VirtualFreeEx(hProcess, buf, bufSize, MEM_DECOMMIT);
		CloseHandle(hProcess);
		return FALSE;
	}

	hThread = CreateRemoteThread(hProcess, NULL, 0, threadProc, buf, 0, NULL);
	if (hThread == NULL)
	{
		VirtualFreeEx(hProcess, buf, bufSize, MEM_DECOMMIT);
		CloseHandle(hProcess);
		return FALSE;
	}

	WaitForSingleObject(hThread, INFINITE);
	VirtualFreeEx(hProcess, buf, bufSize, MEM_DECOMMIT);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return TRUE;
}

BOOL EjectDLLByRemoteThread(DWORD pid, LPCTSTR dllName)
{
	BOOL bMore = FALSE, bFound = FALSE;
	HANDLE hSnapshot, hProcess, hThread;
	HMODULE hModule = NULL;
	MODULEENTRY32 me = { sizeof(me) };
	LPTHREAD_START_ROUTINE threadProc;

	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	bMore = Module32First(hSnapshot, &me);
	for (; bMore; bMore = Module32Next(hSnapshot, &me))
	{
		if (!_tcsicmp(me.szModule, dllName) || !_tcsicmp(me.szExePath, dllName))
		{
			bFound = TRUE;
			break;
		}
	}

	if (!bFound)
	{
		CloseHandle(hSnapshot);
		return FALSE;
	}

	if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)))
	{
		CloseHandle(hSnapshot);
		return FALSE;
	}

	threadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"Kernel32.dll"), "FreeLibrary");
	if (threadProc == NULL)
	{
		CloseHandle(hProcess);
		CloseHandle(hSnapshot);
		return FALSE;
	}

	hThread = CreateRemoteThread(hProcess, NULL, 0, threadProc, me.modBaseAddr, 0, NULL);
	if (hThread == NULL)
	{
		CloseHandle(hProcess);
		CloseHandle(hSnapshot);
		return FALSE;
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	CloseHandle(hProcess);
	CloseHandle(hSnapshot);
	return TRUE;
}
