#include <chrono>
#include <thread>
#include <malloc.h>

#include "client_http.hpp"
#include "server_http.hpp"

#include "../cefclient/browser/root_window_manager.h"

#include "../tab-space/cefclient_win.h"
#include "../tab-space/tab-space-state.h"
#include "../tab-space/webserver.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

int webserverStart(TabSpaceState &tabSpaceState) {
	std::cout << "Starting webserver thread..." << std::endl;

	HttpServer httpServer;
	setupHttpServer(httpServer, tabSpaceState);
	std::cout << "Webserver starting..." << std::endl;
	httpServer.start();
	std::cout << "Webserver terminated." << std::endl;

	return 0;
}

int mainLogicStart(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, TabSpaceState &tabSpaceState) {
	std::cout << "Starting main logic thread..." << std::endl;

	//std::this_thread::sleep_for(std::chrono::milliseconds(20000));
	//client::RootWindowConfig window_config;
	//window_config.always_on_top = false;
	//window_config.with_controls = false;
	//window_config.with_osr = false;
	//std::cout << "Launching root window..." << std::endl;
	//tabSpaceState.context->GetRootWindowManager()->CreateRootWindow(window_config);

	// TODO: Take commands in main thread.
	while (true) {
		std::cin.get();
	}

	std::cout << "Terminating main logic thread..." << std::endl;
	return 0;
}

int main() {
	// Windows setup.
	HINSTANCE hInstance = GetModuleHandle(NULL);
	HINSTANCE hPrevInstance = NULL;
	LPTSTR lpCmdLine = GetCommandLine();
	int nCmdShow = SW_SHOWNORMAL;

	std::cout << "Starting main process..." << std::endl;

	// This process will be started multiple times. Ensure that in each, the CEF loop is run on the main thread. However, other threads should only be started from the main process.

	// Setup CEF state.
	TabSpaceState tabSpaceState;
	tabSpaceState.mainLogicFunction = mainLogicStart;
	tabSpaceState.webserverFunction = webserverStart;
	return client::RunMain::RunMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow, tabSpaceState);
}
