#pragma once

#include "../cefclient/browser/main_context_impl.h"

#include "tab-info.h"

#include <thread>
#include <random>

namespace TabSpace {
	class State {
		public:
		// CLSID for jpeg encoder.
		CLSID jpegClsid;

		// CEF context. Freed automatically at the end of CEF loop.
		client::MainContextImpl *context;

		// Thread which starts logic not involving CEF directly.
		std::thread tabSpaceThread;

		// Function which should be associated with mainLogicThread.
		typedef int (*TabSpaceThreadMain)(HINSTANCE, HINSTANCE, LPTSTR, int, State &);
		TabSpaceThreadMain tabSpaceThreadMain;

		// TODO: Why can't we share RootWindowConfigs?

		// RNG.
		std::mt19937 rng;

		// Mapping from ID to TabInfo. IDs are unique.
		std::map<std::string, TabInfo> tabInfos;

		// Function to generate a unique tab ID, at least with regards to tabInfos
		std::string generateUniqueTabId();
	};
}
