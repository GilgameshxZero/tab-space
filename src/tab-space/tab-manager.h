#pragma once

#include "../cefclient/browser/main_context_impl.h"

#include "../rain-library-4/rain-libraries.h"

#include <string>
#include <thread>

namespace TabSpace {
	// Everything we need to know about a tab.
	class TabManager {
		public:
		TabManager();

		// CLSID to the JPEG encoder, retrieved globally previously.
		static CLSID jpegClsid;

		// Tab destruction handler.
		static std::function<void(TabManager *)> handleStateOnDestruct;

		// Tab ID.
		std::string id;

		// Current width and height of the tab, and thus the video capture as well.
		int width, height;

		// CEF handles.
		scoped_refptr<client::RootWindow> rootWindow;
		scoped_refptr<CefBrowser> browser;
		scoped_refptr<CefBrowserHost> host;

		// Capturing variables.
		HWND hWnd;
		HDC hDc, hDest;
		HBITMAP hBmp;

		// Launches a thread, once we have set rootWindow, id, and jpegClsid.
		void startCaptureThread();

		// Most recent JPEG capture of the window.
		std::vector<char> jpegData;

		// All threads which are reading from the JPEG capture.
		Rain::ConditionVariable nonZeroListenerCV;
		std::mutex listenerMutex;
		std::set<std::thread::id> listeningThreads;

		// Action modifiers.
		bool isShiftKeyDown;
		bool isControlKeyDown;
		bool isAltKeyDown;
	};
}
