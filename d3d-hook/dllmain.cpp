#include <Windows.h>
#include <TCHAR.H> 
#include "Logger.h"
#include "D3D11Hook.h"

static char   dllPath[2048];
static BOOL   stopLoop = false;
static HANDLE hThread = NULL;

struct CaptureInfo
{
	int type = 0;
	int a = 0;
	int b = 0;
};

DWORD __stdcall Attach(LPVOID)
{
	char pathname[2048];
	sprintf_s(pathname, 2048, "%s%s", dllPath, "d3d-hook-log.txt");
	Logger::instance().init(pathname);

	D3D11Hook::Attach();

	CaptureInfo captureInfo;	
	char *buf = NULL;
	HANDLE hMapFile = NULL;
	TCHAR bufName[2048] = { 0 };
	_stprintf_s(bufName, 2048, L"d3d-hook-%lu", GetCurrentProcessId());

	hMapFile = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(CaptureInfo), bufName);
	if (hMapFile == NULL)
	{
		LOG_ERROR("[Map] CreateFileMapping() failed.\n");
	}

	buf = (char*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(CaptureInfo));
	if (buf == NULL)
	{
		LOG_ERROR("[Map] CreateFileMapping() failed.\n");
	}

	while (!stopLoop)
	{		
		if (buf != NULL)
		{
			captureInfo.type = 1;
			captureInfo.a = 2;
			captureInfo.b = 3;
			memcpy(buf, &captureInfo, sizeof(CaptureInfo));
		}

		Sleep(10);
	}

	if (buf != NULL)
	{
		UnmapViewOfFile(buf);
	}

	if (hMapFile != NULL)
	{
		CloseHandle(hMapFile);
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
		for (size_t i = strlen(dllPath); i > 0; i--) 
		{ 
			if (dllPath[i] == '\\') 
			{ 
				dllPath[i + 1] = 0; break; 
			} 
		}

		DisableThreadLibraryCalls(hModule);
		hThread = CreateThread(NULL, 0, Attach, NULL, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		stopLoop = true;
		WaitForSingleObject(hThread, 1000);
		CloseHandle(hThread);
		Detach();
		break;
	}
	return TRUE;
}