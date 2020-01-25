#pragma once

#include <windows.h>

#include "../tab-space/cef-info.h"

namespace client {
	namespace RunMain {
		int RunMain(HINSTANCE hInstance, int nCmdShow, CefInfo &cefInfo);
	}
}