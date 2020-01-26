#pragma once

#include <windows.h>

#include "../tab-space/tab-space-state.h"

namespace client {
	namespace RunMain {
		int RunMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, TabSpaceState &tabSpaceState);
	}
}