#ifndef _HOOK_INFO_H_
#define _HOOK_INFO_H_

#include <Windows.h>

namespace hook
{

struct HookInfo
{
	char type[16];
	int format;
	int width;
	int height;
	int lock;
	HANDLE handle;
};

HANDLE SetHookInfo(struct HookInfo *info, unsigned long pid);
void CloseHookInfo(HANDLE handle);

bool GetHookInfo(struct HookInfo *info, unsigned long pid);

}

#endif

