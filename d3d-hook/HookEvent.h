#ifndef _HOOK_EVENT_H_
#define _HOOK_EVENT_H_

#include <Windows.h>
#include <string>
#include <map>
#include <mutex>

namespace hook
{

static const char* HOOK_EVENT_D3D_INIT    = "HOOK_EVENT_D3D_INIT";
static const char* HOOK_EVENT_D3D_EXIT    = "HOOK_EVENT_D3D_EXIT";
static const char* HOOK_EVENT_D3D_PRESENT = "HOOK_EVENT_D3D_PRESENT";

class HookEvent
{
public:
	static HookEvent& instance();
	~HookEvent();

	void init();
	void exit();
	bool wait(const char* event, int msec);
	bool notify(const char* event);

private:
	HookEvent();

	bool m_isEnabled;
	std::mutex m_mutex;
	std::map<const char*, HANDLE> m_events;
};

}

#endif

