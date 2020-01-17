#ifndef INJECT_HELPER_H
#define INJECT_HELPER_H

#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>

DWORD GetProcessIDByName(LPCTSTR processName);

BOOL  SetPrivilege(LPCTSTR name, BOOL on);

BOOL  InjectDLLByRemoteThread(DWORD pid, LPCTSTR dllPath);

BOOL  EjectDLLByRemoteThread(DWORD pid, LPCTSTR dllName);

#endif