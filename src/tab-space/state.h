#pragma once

#include "../cefclient/browser/main_context_impl.h"

#include "../rain-library-4/rain-libraries.h"

#include "tab-manager.h"

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

		// Mapping from ID to TabManager. IDs are unique.
		std::map<std::string, TabManager *> tabManagers;

		// Function to generate a unique tab ID, at least with regards to tabManagers
		std::string generateUniqueTabId();

		// Profiles and user data.
		std::condition_variable userLoginCv;
		std::mutex userLoginMutex;
		std::map<std::string, std::string> userLoginInfo;
		std::map<std::string, std::string> userLoginTokens;
		void saveUserLoginInfo();
	};
}
