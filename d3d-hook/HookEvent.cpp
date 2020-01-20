#include "HookEvent.h"

using namespace hook;

HookEvent::HookEvent()
	: m_isEnabled(false)
{

}

HookEvent::~HookEvent()
{

}

HookEvent& HookEvent::instance()
{
	static HookEvent hookEvent;
	return hookEvent;
}

void HookEvent::init()
{
	std::lock_guard<std::mutex> locker(m_mutex);

	if (!m_isEnabled)
	{
		TCHAR name[1024];

		swprintf_s(name, 1024, L"%S", HOOK_EVENT_D3D_INIT);
		m_events[HOOK_EVENT_D3D_INIT] = CreateEvent(NULL, FALSE, FALSE, name);

		swprintf_s(name, 1024, L"%S", HOOK_EVENT_D3D_EXIT);
		m_events[HOOK_EVENT_D3D_EXIT] = CreateEvent(NULL, FALSE, FALSE, name);

		m_isEnabled = true;
	}
}

void HookEvent::exit()
{
	std::lock_guard<std::mutex> locker(m_mutex);

	if (m_isEnabled)
	{
		m_isEnabled = false;
		for (auto iter : m_events)
		{
			CloseHandle(iter.second);
		}
		m_events.clear();
	}
}

bool HookEvent::wait(const char* event, int msec)
{
	std::lock_guard<std::mutex> locker(m_mutex);

	if (m_isEnabled)
	{
		auto iter = m_events.find(event);
		if (iter != m_events.end())
		{
			if (WaitForSingleObject(iter->second, (DWORD)msec) == WAIT_OBJECT_0)
			{
				return true;
			}
		}
	}

	return false;
}

bool HookEvent::notify(const char* event)
{
	std::lock_guard<std::mutex> locker(m_mutex);

	if (m_isEnabled)
	{
		auto iter = m_events.find(event);
		if (iter != m_events.end())
		{
			SetEvent(iter->second);
			return true;
		}
	}

	return false;
}