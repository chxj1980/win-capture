#include <stdio.h>
#include "inject.h"

int main(int argc, char **argv)
{
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

	Sleep(10000);

	if (!EjectDLLByRemoteThread(pid, L"d3d-hook.dll"))
	{
		printf("EjectDLLByRemoteThread() failed.\n");
		return 0;
	}

	printf("Eject d3d-hook.dll succeed.\n");

	getchar();
	return 0;
}