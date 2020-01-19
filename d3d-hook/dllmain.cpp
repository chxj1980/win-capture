#include <Windows.h>
#include <TCHAR.H> 
#include "Logger.h"
#include "D3D11Hook.h"
#include "HookEvent.h"

static char   dllPath[2048];
static BOOL   stopLoop = false;
static HANDLE hThread = NULL;

struct CaptureInfo
{
	char type[16];
	HANDLE handle;
};

DWORD __stdcall Attach(LPVOID)
{
	HookEvent::instance().init();

	char pathname[2048];
	sprintf_s(pathname, 2048, "%s%s", dllPath, "d3d-hook-log.txt");
	Logger::instance().init(pathname);

	D3D11Hook::Attach();

	CaptureInfo captureInfo;	
	char *buf = NULL;
	HANDLE hMapFile = NULL;
	TCHAR bufName[2048] = { 0 };
	swprintf_s(bufName, 2048, L"d3d-hook-%lu", GetCurrentProcessId());

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

	bool notify = false;
	while (!stopLoop)
	{		
		if (!notify)
		{
			D3D11TextureInfo textureInfo;
			if (buf != NULL && D3D11Hook::instance().GetTextureInfo(&textureInfo))
			{
				sprintf_s(captureInfo.type, 16, "%s", "d3d11");
				captureInfo.handle = textureInfo.handle;
				memcpy(buf, &captureInfo, sizeof(CaptureInfo));

				HookEvent::instance().notify(HookEvent::HOOK_D3D_INIT);
				notify = true;
			}			
		}

		Sleep(100);
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
	HookEvent::instance().exit();
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