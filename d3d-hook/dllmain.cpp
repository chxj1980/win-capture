#include <Windows.h>
#include <TCHAR.H> 
#include "Logger.h"
#include "D3D11Hook.h"
#include "HookEvent.h"
#include "HookInfo.h"

static char   dllPath[2048];
static BOOL   stopLoop = false;
static HANDLE hThread = NULL;


DWORD __stdcall Attach(LPVOID)
{
	char pathname[2048];
	sprintf_s(pathname, 2048, "%s%s", dllPath, "d3d-hook-log.txt");
	Logger::instance().init(pathname);

	hook::HookEvent::instance().init();
	hook::D3D11Hook::Attach();

	bool notify = false;
	while (!stopLoop)
	{		
		if (!notify)
		{
			hook::D3D11TextureInfo textureInfo;
			if (hook::D3D11Hook::instance().GetTextureInfo(&textureInfo))
			{
				hook::HookInfo info;
				sprintf_s(info.type, 16, "%s", "d3d11");
				info.handle = textureInfo.handle;
				hook::SetHookInfo(&info, GetCurrentProcessId());
				hook::HookEvent::instance().notify(hook::HOOK_EVENT_D3D_INIT);
				notify = true;
			}			
		}

		Sleep(100);
	}

	hook::HookEvent::instance().exit();
	Logger::instance().exit();
	return 0;
}

void Detach()
{
	hook::D3D11Hook::Detach();
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