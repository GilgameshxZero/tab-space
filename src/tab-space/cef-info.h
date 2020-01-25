#pragma once

#include <mutex>

#include "../cefclient/browser/main_context_impl.h"

class CefInfo {
public:
	// CEF context. Freed automatically at the end of CEF loop.
	client::MainContextImpl *context;
};
