#include "../rain-library-4/rain-libraries.h"

#include "tab-manager.h"

#include <iostream>

namespace TabSpace {
	CLSID TabManager::jpegClsid = CLSID();
	std::function<void(TabManager *)> TabManager::handleStateOnDestruct = nullptr;

	TabManager::TabManager() {
		// Defaults for a new tab.
		this->width = 1280;
		this->height = 720;

		this->rootWindow = NULL;
		this->id = std::string();

		this->isShiftKeyDown = false;
		this->isControlKeyDown = false;
		this->isAltKeyDown = false;
	}

	void TabManager::startCaptureThread() {
		static const UINT JPEG_QUALITY = 50;

		// Wait until we can get the browser window.
		this->browser = this->rootWindow->GetBrowser();
		while (this->browser == NULL) {
			std::this_thread::sleep_for(std::chrono::milliseconds(17));
			this->browser = this->rootWindow->GetBrowser();
		}
		this->host = this->browser->GetHost();

		// TODO: Audio.
		this->host->SetAudioMuted(true);

		// Create the capture objects.
		this->hWnd = this->browser->GetHost()->GetWindowHandle();
		this->hDc = GetDC(this->hWnd);
		this->hDest = CreateCompatibleDC(this->hDc);
		this->hBmp = CreateCompatibleBitmap(this->hDc, this->width, this->height);
		HGDIOBJ prevBmp = SelectObject(this->hDest, this->hBmp);

		std::thread([&]() {
			Rain::tsCout("Launched tab ", id, " on thread ", std::this_thread::get_id(), ".", Rain::CRLF);
			std::cout.flush();

			// For JPEG compression options.
			Gdiplus::EncoderParameters encoderParameters;
			UINT quality = JPEG_QUALITY;
			encoderParameters.Count = 1;
			encoderParameters.Parameter[0].Guid = Gdiplus::EncoderQuality;
			encoderParameters.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
			encoderParameters.Parameter[0].NumberOfValues = 1;
			encoderParameters.Parameter[0].Value = &quality;

			// Enable only capture when we have listeners.
			std::unique_lock<std::mutex> lck(this->nonZeroListenerCV.getMutex());

			IStream *iStream = nullptr;
			HGLOBAL hg = NULL;
			while (true) { // Only breaks if tab expires.
				// TODO: Lower DPI on display instead.
				// this->host->SetZoomLevel(-2);

				// Check that we still have listeners.
				if (this->listeningThreads.size() == 0) {
					// Tabs expire in 60 seconds without listeners.
					Rain::tsCout("Paused capturing for tab ", this->id, ".", Rain::CRLF);
					if (this->nonZeroListenerCV.wait_for(lck, std::chrono::seconds(60)) == false) {
						Rain::tsCout("Tab ", this->id, " has expired due to inactivity.", Rain::CRLF);
						break;
					} else {
						Rain::tsCout("Resuming capturing for tab ", this->id, ".", Rain::CRLF);
					}
					std::cout.flush();
				}

				// 0x00000002 allows us to capture GPU renders.
				this->resolutionMutex.lock();
				PrintWindow(this->hWnd, this->hDest, 0x00000002);

				// Save the JPEG data.
				Gdiplus::Bitmap bmp(this->hBmp, (HPALETTE) 0);
				this->resolutionMutex.unlock();

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

			SelectObject(this->hDest, prevBmp);
			ReleaseDC(NULL, this->hDc);
			DeleteObject(this->hBmp);
			DeleteDC(this->hDest);

			// Prepare to destroy things.
			this->rootWindow->Close(false);
			TabManager::handleStateOnDestruct(this);
		}).detach();
	}

	void TabManager::setResolution(int width, int height) {
		this->resolutionMutex.lock();

		this->width = width;
		this->height = height;
		HBITMAP newBmp = CreateCompatibleBitmap(this->hDc, this->width, this->height);
		SelectObject(this->hDest, newBmp);
		DeleteObject(this->hBmp);
		this->hBmp = newBmp;

		// A bit larger for good measure.
		this->rootWindow->SetBounds(0, 0, this->width + 100, this->height + 100);

		// Update browser.
		SetWindowPos(this->hWnd, NULL, 0, 0,
			this->width, this->height,
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

		this->resolutionMutex.unlock();
	}
}
