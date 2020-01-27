#include "../rain-library-4/rain-libraries.h"

#include "tab-manager.h"

#include <iostream>

namespace TabSpace {
	CLSID TabManager::jpegClsid = CLSID();

	TabManager::TabManager() {
		// Defaults for a new tab.
		this->width = 1920;
		this->height = 1080;

		this->rootWindow = NULL;
		this->id = std::string();
	}

	void TabManager::startCaptureThread() {
		// Wait until we can get the browser window.
		this->browser = this->rootWindow->GetBrowser();
		while (this->browser == NULL) {
			std::this_thread::sleep_for(std::chrono::milliseconds(17));
			this->browser = this->rootWindow->GetBrowser();
		}

		// Create the capture objects.
		this->hWnd = this->browser->GetHost()->GetWindowHandle();
		this->hDc = GetDC(this->hWnd);
		this->hDest = CreateCompatibleDC(this->hDc);
		this->hBmp = CreateCompatibleBitmap(this->hDc, this->width, this->height);
		SelectObject(this->hDest, this->hBmp);

		std::thread([&]() {
			// For JPEG compression options.
			Gdiplus::EncoderParameters encoderParameters;
			ULONG quality = 30;
			encoderParameters.Count = 1;
			encoderParameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
			encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
			encoderParameters.Parameter[0].NumberOfValues = 1;
			encoderParameters.Parameter[0].Value = &quality;

			IStream *iStream = nullptr;
			HGLOBAL hg = NULL;
			while (true) {
				// 0x00000002 allows us to capture GPU renders.
				PrintWindow(this->hWnd, this->hDest, 0x00000002);

				// Save the JPEG data.
				Gdiplus::Bitmap bmp(this->hBmp, (HPALETTE) 0);
				CreateStreamOnHGlobal(NULL, TRUE, &iStream);
				bmp.Save(iStream, &TabManager::jpegClsid, &encoderParameters);
				GetHGlobalFromStream(iStream, &hg);

				// Copy IStream to buffer.
				int bufsize = GlobalSize(hg);
				this->jpegData.resize(bufsize);
				LPVOID pImage = GlobalLock(hg);
				memcpy(&this->jpegData[0], pImage, bufsize);
				GlobalUnlock(hg);
				iStream->Release();

				std::this_thread::sleep_for(std::chrono::milliseconds(17));
			}

			ReleaseDC(NULL, this->hDc);
			DeleteObject(this->hBmp);
			DeleteDC(this->hDest);
		}).detach();
	}
}
