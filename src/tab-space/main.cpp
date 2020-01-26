#include "../tab-space/cefclient_win.h"

#include <chrono>
#include <thread>

#include "../cefclient/browser/root_window_manager.h"

#include "client_http.hpp"
#include "server_http.hpp"

#include "../tab-space/cef-info.h"
#include "../tab-space/webserver.h"

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

int webserver(CefInfo &cefInfo) {
	std::cout << "Starting webserver thread..." << std::endl;

	HttpServer httpServer;
	setupHttpServer(httpServer);
	std::cout << "Webserver starting..." << std::endl;
	httpServer.start();
	std::cout << "Webserver terminated." << std::endl;

	return 0;
}

int mainLogic(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, CefInfo &cefInfo) {
	std::cout << "Starting main logic thread..." << std::endl;

	////// Take commands in main thread.
	//std::this_thread::sleep_for(std::chrono::milliseconds(4000));
	////client::RootWindowConfig window_config;
	////window_config.always_on_top = false;
	////window_config.with_controls = true;
	////window_config.with_osr = false;
	////std::cout << "Launching root window...";
	////cefInfo.context->GetRootWindowManager()->CreateRootWindow(window_config);
	//cefThread.join();

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
	CefInfo cefInfo;
	cefInfo.mainLogicFunction = mainLogic;
	cefInfo.webserverFunction = webserver;
	return client::RunMain::RunMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow, cefInfo);
}
