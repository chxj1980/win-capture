#include "HookInfo.h"
#include <stdio.h>
#include <tchar.h> 

HANDLE hook::SetHookInfo(struct HookInfo *info, unsigned long pid)
{
	char *buf = NULL;
	HANDLE hMapFile = NULL;
	TCHAR name[2048] = { 0 };
	swprintf_s(name, 2048, L"d3d-hook-%lu", pid);

	hMapFile = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(HookInfo), name);
	if (hMapFile == NULL)
	{
		return NULL;
	}

	buf = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HookInfo));
	if (buf == NULL)
	{
		return NULL;
	}

	memcpy(buf, info, sizeof(HookInfo));

	if (buf != NULL)
	{
		UnmapViewOfFile(buf);
	}

	//if (hMapFile != NULL)
	//{
	//	CloseHandle(hMapFile);
	//}

	return hMapFile;
}

void hook::CloseHookInfo(HANDLE handle)
{
	if (handle != NULL)
	{
		CloseHandle(handle);
	}
}

bool hook::GetHookInfo(struct HookInfo *info, unsigned long pid)
{
	HANDLE  hMapFile = NULL;
	char* buf = NULL;
	TCHAR name[1024] = { 0 };
	swprintf_s(name, 1024, L"d3d-hook-%lu", pid);

	hMapFile = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
	if (NULL == hMapFile)
	{
		return false;
	}

	buf = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(HookInfo));
	if (NULL == buf)
	{
		return false;
	}

	if (buf != NULL)
	{
		memcpy(info, buf, sizeof(HookInfo));
		//printf("hook info: type:%s, handle:%llx \n", info->type, (uintptr_t)info->handle);
	}

	if (buf != NULL)
	{
		UnmapViewOfFile(buf);
	}

	if (hMapFile != NULL)
	{
		CloseHandle(hMapFile);
	}

	return true;
}

