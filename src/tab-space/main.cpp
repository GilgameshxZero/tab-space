#include <malloc.h>

#include "client_http.hpp"
#include "server_http.hpp"

#include "../cefclient/browser/root_window_manager.h"

#include "../rain-library-4/rain-libraries.h"

#include "cefclient_win.h"
#include "state.h"
#include "tab-manager.h"
#include "webserver.h"

#include <chrono>
#include <thread>

namespace TabSpace {
	int tabSpaceThreadMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, State &state) {
		// Initialize Gdiplus.
		ULONG_PTR gdiplusToken;
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		Rain::getEncoderClsid(L"image/jpeg", &TabManager::jpegClsid);

		// Setup tab destruct handler.
		TabManager::onDestructHandler = [&state](TabManager *tabManager) {
			state.tabManagers.erase(tabManager->id);
			delete tabManager;
		};

		// Random seed for tab IDs.
		// state.rng.seed(std::chrono::system_clock::now().time_since_epoch().count());

		SimpleWeb::Server<SimpleWeb::HTTP> httpServer;
		initHttpServer(httpServer, state);
		Rain::tsCout("Webserver starting on thread ", std::this_thread::get_id(), "...", Rain::CRLF);
		std::cout.flush();

		// Blocking call to webserver.
		httpServer.start();

		Gdiplus::GdiplusShutdown(gdiplusToken);

		return 0;
	}
}

int main() {
	// Windows setup.
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HINSTANCE hPrevInstance = NULL;
	LPTSTR lpCmdLine = GetCommandLine();
	int nCmdShow = SW_SHOWNORMAL;

	// std::cout << "Starting process..." << std::endl;

	// This process will be started multiple times. Ensure that in each, the CEF loop is run on the main thread. However, other threads should only be started from the main process.

	// Setup CEF state.
	TabSpace::State state;
	state.tabSpaceThreadMain = TabSpace::tabSpaceThreadMain;
	return client::RunMain::RunMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow, state);
}
