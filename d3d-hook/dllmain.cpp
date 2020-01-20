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

	HANDLE handle = NULL;
	bool notify = false;
	int timestamp = 0;
	hook::D3D11TextureInfo textureInfo;
	
	while (!stopLoop)
	{		
		bool ret = hook::D3D11Hook::instance().GetTextureInfo(&textureInfo);
		if (ret)
		{
			if (!notify)
			{
				hook::HookInfo info;
				sprintf_s(info.type, 16, "%s", "d3d11");
				info.handle = textureInfo.handle;
				info.format = textureInfo.format;
				info.width = textureInfo.width;
				info.height = textureInfo.height;
				info.lock = textureInfo.lock;

				handle = hook::SetHookInfo(&info, GetCurrentProcessId());
				hook::HookEvent::instance().notify(hook::HOOK_EVENT_D3D_INIT);
				notify = true;
			}
			else
			{
				if (timestamp != textureInfo.timestamp)
				{
					timestamp = textureInfo.timestamp;
					hook::HookEvent::instance().notify(hook::HOOK_EVENT_D3D_PRESENT);
				}
			}
		}
		
		Sleep(10);
	}

	if (notify)
	{
		hook::CloseHookInfo(handle);
		hook::D3D11Hook::instance().Exit();
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