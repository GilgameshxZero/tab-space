#pragma once

#include "../rain-library-4/rain-libraries.h"

#include "state.h"

namespace TabSpace {
	int tabSpaceThreadMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, State &state);
}

int main();
