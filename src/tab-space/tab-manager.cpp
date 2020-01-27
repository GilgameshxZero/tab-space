#include "../rain-library-4/rain-libraries.h"

#include "tab-manager.h"

#include <iostream>

namespace TabSpace {
	TabManager::TabManager() {
		// Defaults for a new tab.
		this->width = 1280;
		this->height = 720;
	}

	void TabManager::captureFunction() {
		// Wait until we can get the browser window.
		this->browser = this->rootWindow->GetBrowser();
		while (this->browser == NULL) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			this->browser = this->rootWindow->GetBrowser();
		}

		this->hWnd = this->browser->GetHost()->GetWindowHandle();
		this->hdc = GetDC(this->hWnd);
		this->hDest = CreateCompatibleDC(this->hdc);
		this->hbmp = CreateCompatibleBitmap(this->hdc, this->width, this->height);
		SelectObject(this->hDest, this->hbmp);

		this->data = new char[1000000];

		while (true) {
			// TODO: 0x00000002 is very necessary for capturing GPU renders.
			PrintWindow(this->hWnd, this->hDest, 0x00000002);

			// Keep the bitmap.
			Gdiplus::Bitmap bmp(this->hbmp, (HPALETTE) 0);
			IStream *istream = nullptr;
			HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &istream);
			bmp.Save(istream, this->jpegClsid);

			HGLOBAL hg = NULL;
			GetHGlobalFromStream(istream, &hg);

			//copy IStream to buffer
			int bufsize = GlobalSize(hg);

			//lock & unlock memory
			LPVOID pimage = GlobalLock(hg);
			memcpy(this->data, pimage, bufsize);
			this->bufsize = bufsize;
			GlobalUnlock(hg);

			istream->Release();

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		ReleaseDC(NULL, this->hdc);
		DeleteObject(this->hbmp);
		DeleteDC(this->hDest);
	}
}
