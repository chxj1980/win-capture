#include <stdio.h>
#include <tchar.h> 
#include <thread>
#include <memory>
#include "inject.h"
#include "HookEvent.h"
#include "HookInfo.h"
#include "nvenc.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

static void *nvenc = nullptr;
static bool stopLoop = false;
static std::shared_ptr<std::thread> encodeThread = nullptr;

void StartEncoding(hook::HookInfo* info)
{
	if (nvenc_info.is_supported())
	{
		nvenc = nvenc_info.create();
	}

	if (nvenc != nullptr)
	{
		encoder_config nvenc_config;
		nvenc_config.codec = "h264";
		nvenc_config.format = (DXGI_FORMAT)info->format;
		nvenc_config.width = info->width;
		nvenc_config.height = info->height;
		nvenc_config.framerate = 30;
		nvenc_config.gop = nvenc_config.framerate * 10;
		nvenc_config.bitrate = 4000000;

		if (!nvenc_info.init(nvenc, &nvenc_config))
		{
			nvenc_info.destroy(&nvenc);
			nvenc = nullptr;
		}
		else
		{
			stopLoop = false;
			encodeThread.reset(new std::thread([=] {
				uint32_t bufferSize = info->width * info->height * 4;
				std::shared_ptr<uint8_t> buffer(new uint8_t[bufferSize]);
				int frameSize = 0;

				while (!stopLoop)
				{					
					if (hook::HookEvent::instance().wait(hook::HOOK_EVENT_D3D_PRESENT, 10))
					{
						frameSize = nvenc_info.encode_handle(nvenc, info->handle, info->lock, 0,
																buffer.get(), bufferSize);
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}));
		}
	}
}

void StopEncodnig()
{
	if (encodeThread != nullptr)
	{
		stopLoop = true;
		encodeThread->join();
		encodeThread = nullptr;
		nvenc_info.destroy(&nvenc);
		nvenc = nullptr;
	}
}

int main(int argc, char **argv)
{
	DWORD pid = GetProcessIDByName(L"JX3ClientX64.exe");
	if (!pid)
	{
		printf("GetProcessIDByName() failed.\n");
		return 0;
	}

	EjectDLLByRemoteThread(pid, L"d3d-hook.dll");

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

	if (!InjectDLLByRemoteThread(pid, dllPath))
	{
		printf("InjectDLLByRemoteThread() failed.\n");
		return 0;
	}

	printf("Inject d3d-hook.dll succeed.\n");

	hook::HookEvent::instance().init();

	if (!hook::HookEvent::instance().wait(hook::HOOK_EVENT_D3D_INIT, 20000))
	{
		printf("Wait for hook timeout.\n");
	}
	else
	{
		hook::HookInfo info;
		if (hook::GetHookInfo(&info, pid))
		{
			printf("hook info: type:%s, handle:%llx \n", info.type, (uintptr_t)info.handle);
			//StartEncoding(&info);
		}
	}

	printf("\nPlease press enter to continue ...\n");
	getchar();

	//StopEncodnig();

	hook::HookEvent::instance().exit();

	if (!EjectDLLByRemoteThread(pid, L"d3d-hook.dll"))
	{
		printf("EjectDLLByRemoteThread() failed.\n");
		return 0;
	}

	printf("Eject d3d-hook.dll succeed.\n");

	getchar();
	return 0;
}
