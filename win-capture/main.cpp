#include <stdio.h>
#include <TCHAR.H> 
#include "inject.h"
#include "HookEvent.h"

struct CaptureInfo
{
	int type = 0;
	int a = 0;
	int b = 0;
};

int main(int argc, char **argv)
{
	HookEvent::instance().init();

	char buf1[1024];
	if (GetModuleFileNameA(NULL, buf1, sizeof(buf1)))
	{
		char *ptr = strrchr(buf1, '\\');
		if (ptr)
		{
			*ptr = '\0';
			SetCurrentDirectoryA(buf1);
		}
	}

	char buf2[2048];
	sprintf_s(buf2, 2048, "%s\\..\\d3d-hook\\d3d-hook.dll", buf1);

	TCHAR dllPath[2048];
	int len = MultiByteToWideChar(CP_ACP, 0, buf2, (int)strlen(buf2) + 1, NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, buf2, (int)strlen(buf2) + 1, dllPath, len);

	if (!SetPrivilege(SE_DEBUG_NAME, TRUE))
	{
		printf("SetPrivilege() failed.\n");
	}

	DWORD pid = GetProcessIDByName(L"JX3ClientX64.exe");
	if (!pid)
	{
		printf("GetProcessIDByName() failed.\n");
		return 0;
	}

	if (!InjectDLLByRemoteThread(pid, dllPath))
	{
		printf("InjectDLLByRemoteThread() failed.\n");
		return 0;
	}

	printf("Inject d3d-hook.dll succeed.\n");

	Sleep(2000);
	if (!HookEvent::instance().wait(HookEvent::HOOK_D3D_INIT, 10000))
	{
		printf("Wait for hook timeout.\n");
	}

	struct CaptureInfo captureInfo;
	HANDLE  hMapFile = NULL;
	char* buf = NULL;
	TCHAR bufName[1024] = { 0 };
	swprintf_s(bufName, 1024, L"d3d-hook-%lu", pid);
	
	hMapFile = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, bufName);
	if (NULL == hMapFile)
	{
		printf("OpenFileMapping() failed.\n");
	}

	buf = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CaptureInfo));
	if (NULL == buf)
	{
		printf("OpenFileMapping() failed.\n");
	}

	if (buf != NULL)
	{
		memcpy(&captureInfo, buf, sizeof(CaptureInfo));
		printf("map: %d %d %d\n", captureInfo.type, captureInfo.a, captureInfo.b);
	}

	getchar();

	if (buf != NULL)
	{
		UnmapViewOfFile(buf);
	}

	if (hMapFile != NULL)
	{
		CloseHandle(hMapFile);
	}

	if (!EjectDLLByRemoteThread(pid, L"d3d-hook.dll"))
	{
		printf("EjectDLLByRemoteThread() failed.\n");
		return 0;
	}

	HookEvent::instance().exit();

	printf("Eject d3d-hook.dll succeed.\n");

	getchar();
	return 0;
}