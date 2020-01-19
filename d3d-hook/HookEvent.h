#ifndef _HOOK_EVENT_H_
#define _HOOK_EVENT_H_

#include <Windows.h>
#include <string>
#include <map>
#include <mutex>

class HookEvent
{
public:
	static HookEvent& instance();
	~HookEvent();

	void init();
	void exit();
	bool wait(std::string event, int msec);
	bool notify(std::string event);

	static std::string HOOK_D3D_INIT;
	static std::string HOOK_D3D_EXIT;

private:
	HookEvent();

	bool m_isEnabled;
	std::mutex m_mutex;
	std::map<std::string, HANDLE> m_events;

};

#endif

