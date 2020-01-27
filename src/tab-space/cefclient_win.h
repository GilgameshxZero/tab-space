#pragma once

#include "../rain-library-4/rain-libraries.h"

#include "state.h"

namespace client {
	namespace RunMain {
		int RunMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, TabSpace::State &state);
	}
}
