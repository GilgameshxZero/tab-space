#include "cefclient_win.h"

int main() {
	// Windows setup.
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HINSTANCE hPrevInstance = NULL;
	LPTSTR lpCmdLine = GetCommandLine();
	int nCmdShow = SW_SHOWNORMAL;

	return client::RunMain::RunMain(hInstance, nCmdShow);
}
