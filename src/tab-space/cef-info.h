#pragma once

#include <thread>

#include "../cefclient/browser/main_context_impl.h"

class CefInfo {
public:
	// CEF context. Freed automatically at the end of CEF loop.
	client::MainContextImpl *context;

	// Thread which starts logic not involving CEF directly.
	std::thread *mainLogicThread;

	// Function which should be associated with mainLogicThread.
	typedef int (*MainLogicFunction)(HINSTANCE, HINSTANCE, LPTSTR, int, CefInfo &);
	MainLogicFunction mainLogicFunction;

	// Threads and functions for webserver thread.
	std::thread *webserverThread;
	typedef int (*WebserverFunction)(CefInfo &);
	WebserverFunction webserverFunction;
};
