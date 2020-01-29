#include <malloc.h>

#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

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
#include <fstream>

namespace TabSpace {
	int tabSpaceThreadMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow, State &state) {
		// Initialize Gdiplus.
		ULONG_PTR gdiplusToken;
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		Rain::getEncoderClsid(L"image/jpeg", &TabManager::jpegClsid);

		// Setup tab destruct handler.
		TabManager::handleStateOnDestruct = [&state](TabManager *tabManager) {
			state.tabManagers.erase(tabManager->id);
			delete tabManager;
		};

		// Random seed for tab IDs.
		state.rng.seed(std::chrono::system_clock::now().time_since_epoch().count());

		// Load user login information.
		auto userLoginInfoPath = boost::filesystem::absolute(boost::dll::program_location().parent_path() / "../../db/user-login-info.json");
		std::ifstream ifs(userLoginInfoPath.string(), std::ifstream::in | std::ios::binary);
		if (ifs) {
			// File is just a long list of usernames and hashsalts separated by spaces and newlines.
			// The first number is the number of pairs of usernames and hashsalts.
			int n;
			std::string username, hashsalt;
			ifs >> n;
			for (int a = 0; a < n; a++) {
				ifs >> username, hashsalt;
				state.userLoginInfo[username] = hashsalt;
			}
			ifs.close();
			Rain::tsCout("Loaded ", state.userLoginInfo.size(), " users.", Rain::CRLF);
		}

		// Start a thread to periodically save the user login information.
		std::thread([](State *state) {
			std::mutex mtx;
			std::unique_lock<std::mutex> lck(mtx);
			std::cv_status waitResult = std::cv_status::timeout;
			while (waitResult == std::cv_status::timeout) {
				// Returns false if timeout, true if terminated.
				waitResult = state->userLoginCv.wait_for(lck, std::chrono::seconds(5));
				state->saveUserLoginInfo();
			}
		}, &state).detach();

		// Save user information on exit.
		SetConsoleCtrlHandler([](DWORD fdwCtrlType) -> BOOL {
			switch (fdwCtrlType) {
				case CTRL_C_EVENT:
				case CTRL_CLOSE_EVENT:
				case CTRL_LOGOFF_EVENT:
				case CTRL_SHUTDOWN_EVENT:
					Rain::tsCout("CTRL+C signal detected. Exiting...", Rain::CRLF);
					std::cout.flush();

					// Wait a bit for save.
					std::this_thread::sleep_for(std::chrono::seconds(3));
					ExitProcess(0);
					return TRUE;
				default:
					return FALSE;
			}
		}, TRUE);

		// Setup webserver.
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
