#include <Windows.h>
#include "Logger.h"
#include "D3D11Hook.h"

static BOOL   stopLoop     = false;
static HANDLE threadHandle = false;
static char   dllPath[2048];

struct CaptureInfo
{
	int type;
	int mapID;
	int mapSize;
};


DWORD __stdcall Attach(LPVOID)
{
	char pathname[2048];
	sprintf_s(pathname, 2048, "%s%s", dllPath, "hook-log.txt");
	Logger::instance().init(pathname);

	D3D11Hook::Attach();

	while (!stopLoop)
	{
		Sleep(10);
	}

	Logger::instance().exit();
	return 0;
}

void Detach()
{
	D3D11Hook::Detach();
}

BOOL __stdcall DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		memset(dllPath, 0, 2048);
		GetModuleFileNameA(hModule, dllPath, 2048);
		for (size_t i = strlen(dllPath); i > 0; i--) { if (dllPath[i] == '\\') { dllPath[i + 1] = 0; break; } }
		DisableThreadLibraryCalls(hModule);
		threadHandle = CreateThread(NULL, 0, Attach, NULL, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		stopLoop = true;
		WaitForSingleObject(threadHandle, 1000);
		CloseHandle(threadHandle);	
		Detach();
		break;
	}
	return TRUE;
}