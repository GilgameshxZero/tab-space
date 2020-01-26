#pragma once

#include <thread>

#include "../cefclient/browser/main_context_impl.h"

#include "../tab-space/tab-info.h"

class TabSpaceState {
public:
	// CEF context. Freed automatically at the end of CEF loop.
	client::MainContextImpl *context;

	// Thread which starts logic not involving CEF directly.
	std::thread *mainLogicThread;

	// Function which should be associated with mainLogicThread.
	typedef int (*MainLogicFunction)(HINSTANCE, HINSTANCE, LPTSTR, int, TabSpaceState &);
	MainLogicFunction mainLogicFunction;

	// Threads and functions for webserver thread.
	std::thread *webserverThread;
	typedef int (*WebserverFunction)(TabSpaceState &);
	WebserverFunction webserverFunction;

	// TODO: Why can't we share RootWindowConfigs?

	// Mapping from ID to TabInfo. IDs are unique.
	std::map<std::string, TabInfo> tabInfos;

	// Function to generate a unique tab ID, at least with regards to tabInfos
	std::string generateUniqueTabId();
};
