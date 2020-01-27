#pragma once

#include "../cefclient/browser/main_context_impl.h"

#include "../rain-library-4/gdi-plus-include.h"

#include <string>
#include <thread>

namespace TabSpace {
	// Everything we need to know about a tab.
	class TabInfo {
		public:
		TabInfo();

		// Pointer to the CLSID to the JPEG encoder, retrieved globally previously.
		CLSID *jpegClsid;

		// Tab ID.
		std::string id;

		// Current width and height of the tab, and thus the video capture as well.
		int width, height;

		// CEF handles.
		scoped_refptr<client::RootWindow> rootWindow;
		scoped_refptr<CefBrowser> browser;

		HWND hWnd;
		HDC hdc, hDest;
		HBITMAP hbmp;

		std::thread captureThread;
		void captureFunction();

		// Most recent JPEG capture of the window.
		char *data;
		int bufsize;
	};
}
