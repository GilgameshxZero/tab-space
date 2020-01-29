#include <malloc.h>

#include "client_http.hpp"
#include "server_http.hpp"

#include "../rain-library-4/rain-libraries.h"

#include "webserver.h"
#include "state.h"

#include <boost/filesystem.hpp>
#include <boost/dll.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <fstream>

namespace TabSpace {
	void serveStaticFile(std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::string requestPath, SimpleWeb::StatusCode statusCode) {
		static const boost::filesystem::path STATIC_RELATIVE_LOCATION = "../../static";
		static const boost::filesystem::path DEFAULT_INDEX = "index.html";

		// Will respond with content in the static/-directory, and its subdirectories.
		// Can for instance be used to retrieve an HTML 5 client that uses REST-resources on this server
		try {
			// Previously was using canonical, but that didn't seem to work.
			// Using executable path so that debugging in Visual Studio isn't a pain.
			auto web_root_path = boost::filesystem::absolute(boost::dll::program_location().parent_path() / STATIC_RELATIVE_LOCATION);
			auto path = boost::filesystem::absolute(web_root_path / requestPath);

			// Check if path is within web_root_path
			if (std::distance(web_root_path.begin(), web_root_path.end()) > std::distance(path.begin(), path.end()) ||
				!std::equal(web_root_path.begin(), web_root_path.end(), path.begin())) {
				throw std::invalid_argument("Path must be within static root path.");
			}
			if (boost::filesystem::is_directory(path))
				path /= DEFAULT_INDEX;

			auto ifs = std::make_shared<std::ifstream>();
			ifs->open(path.string(), std::ifstream::in | std::ios::binary | std::ios::ate);

			// Handle as 404.
			if (!*ifs) {
				path = boost::filesystem::absolute(web_root_path / "/404.html");
				ifs->open(path.string(), std::ifstream::in | std::ios::binary | std::ios::ate);
				statusCode = SimpleWeb::StatusCode::client_error_bad_request;
			}

			// Need to be able to open the file.
			if (*ifs) {
				auto length = ifs->tellg();
				ifs->seekg(0, std::ios::beg);

				// Content-Length and some other headers too.
				SimpleWeb::CaseInsensitiveMultimap header;
				header.emplace("Content-Length", to_string(length));
				response->write(statusCode, header);

				// Send the file in chunks.
				// Read and send 128 KB at a time
				/*static std::vector<char> buffer(131072); // Safe when server is running on one thread
				std::streamsize read_length;
				bool interrupted = false;

				// Waiting for callback to finish.
				Rain::ConditionVariable cv;
				std::unique_lock<std::mutex> lck(cv.getMutex());

				while (!interrupted &&
					(read_length = ifs->read(&buffer[0], static_cast<std::streamsize>(buffer.size())).gcount()) > 0) {
					response->write(&buffer[0], read_length);
					if (read_length == static_cast<std::streamsize>(buffer.size())) {
						response->send([&interrupted, &cv](const SimpleWeb::error_code &ec) {
							if (ec) {
								std::cerr << "Connection interrupted." << std::endl;
								interrupted = true;
							}
							// It is okay if this line is executed first.
							cv.notify_one();
						});
						cv.wait(lck);
					}
				}*/
				// TODO: Make this non-recursive correctly.
				class FileServer {
					public:
					static void read_and_send(const std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> &response, const std::shared_ptr<std::ifstream> &ifs) {
						// Read and send 128 KB at a time
						static std::vector<char> buffer(131072); // Safe when server is running on one thread
						std::streamsize read_length;
						if ((read_length = ifs->read(&buffer[0], static_cast<std::streamsize>(buffer.size())).gcount()) > 0) {
							response->write(&buffer[0], read_length);
							if (read_length == static_cast<std::streamsize>(buffer.size())) {
								response->send([response, ifs](const SimpleWeb::error_code &ec) {
									if (!ec)
										read_and_send(response, ifs);
									else
										std::cerr << "Connection interrupted" << std::endl;
								});
							}
						}
					}
				};
				FileServer::read_and_send(response, ifs);
			} else {
				throw std::invalid_argument("Could not read file.");
			}
		} catch (...) {
			response->write(SimpleWeb::StatusCode::client_error_bad_request, "Bad Request.");
		}
	}

	void getDefault(std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
		serveStaticFile(response, request->path, SimpleWeb::StatusCode::success_ok);
	}

	void onError(std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request, const SimpleWeb::error_code &ec) {
		// Do nothing for now.
	}

	auto getNewCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = state.generateUniqueTabId();
			state.tabManagers[id] = new TabManager();
			TabManager &tabManager = *state.tabManagers[id];

			client::RootWindowConfig windowConfig;
			windowConfig.always_on_top = false;
			windowConfig.with_controls = false;
			windowConfig.with_osr = false;
			// windowConfig.initially_hidden = true;
			windowConfig.bounds = CefRect(0, 0, tabManager.width, tabManager.height);
			scoped_refptr<client::RootWindow> rootWindow = state.context->GetRootWindowManager()->CreateRootWindow(windowConfig);

			// Setup tabManager.
			tabManager.id = id;
			tabManager.rootWindow = rootWindow;
			tabManager.startCaptureThread();

			response->write(id);
		};
	}

	auto getTabCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = request->path_match[1].str();

			// Check that the ID is valid.
			if (state.tabManagers.find(id) == state.tabManagers.end()) {
				serveStaticFile(response, "/404.html", SimpleWeb::StatusCode::client_error_bad_request);
			} else {
				serveStaticFile(response, "/tab.html", SimpleWeb::StatusCode::success_ok);
			}
		};
	}

	auto getStreamCurried(State &state) {
		static const std::string FRAME_BOUNDARY = "tab-space-boundary";
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = request->path_match[1].str();

			// Check that the ID is valid.
			if (state.tabManagers.find(id) == state.tabManagers.end()) {
				serveStaticFile(response, "/404.html", SimpleWeb::StatusCode::client_error_bad_request);
				return;
			}

			*response << "HTTP/1.1 200" << Rain::CRLF
				<< "Content-Type: multipart/x-mixed-replace;boundary=" << FRAME_BOUNDARY << Rain::CRLF
				<< Rain::CRLF;

			// Start separate thread to push data through.
			std::thread([response](TabManager *tabManager) {
				std::thread::id threadId = std::this_thread::get_id();
				tabManager->listenerMutex.lock();
				tabManager->listeningThreads.insert(threadId);
				if (tabManager->listeningThreads.size() == 1) {
					tabManager->nonZeroListenerCV.notify_one();
				}
				Rain::tsCout("Stream requested for tab ", tabManager->id, " on thread ", threadId, " [", tabManager->listeningThreads.size(), " total].", Rain::CRLF);
				tabManager->listenerMutex.unlock();
				std::cout.flush();

				Rain::ConditionVariable cv;
				std::unique_lock<std::mutex> lck(cv.getMutex());
				bool active = true;

				while (active) {
					*response << "--" << FRAME_BOUNDARY << Rain::CRLF
						<< "Content-Type: image/jpeg" << Rain::CRLF
						<< "Content-Length: " << Rain::tToStr(tabManager->jpegData.size()) << Rain::CRLF
						<< Rain::CRLF;
					response->write(&tabManager->jpegData[0], tabManager->jpegData.size());
					*response << Rain::CRLF << Rain::CRLF;
					response->send([&](const SimpleWeb::error_code &ec) {
						if (ec) {
							active = false;
							tabManager->listenerMutex.lock();
							tabManager->listeningThreads.erase(threadId);
							Rain::tsCout("Stream for tab ", tabManager->id, " on thread ", threadId, " closed with value ", ec.value(), " [", tabManager->listeningThreads.size(), " total].", Rain::CRLF);
							tabManager->listenerMutex.unlock();
							std::cout.flush();
						}
						cv.notify_one();
					});
					// TODO: How to do better sync?
					std::this_thread::sleep_for(std::chrono::milliseconds(50));
					cv.wait(lck);
				}
			}, state.tabManagers[id]).detach();
		};
	}

	auto getActionMouseCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = request->path_match[1].str();
			if (state.tabManagers.find(id) == state.tabManagers.end()) {
				serveStaticFile(response, "/404.html", SimpleWeb::StatusCode::client_error_bad_request);
				return;
			}
			response->write(SimpleWeb::StatusCode::success_ok);

			// Unwrap JSON.
			boost::property_tree::ptree propertyTree;
			std::istringstream iss(request->content.string());
			boost::property_tree::read_json(iss, propertyTree);

			CefMouseEvent cefMouseEvent;
			// TODO: Calculate DPI of screen.
			static const double DPI = 1;

			cefMouseEvent.x = propertyTree.get<double>("x") / DPI;
			cefMouseEvent.y = propertyTree.get<double>("y") / DPI;
			bool mouseUp = propertyTree.get<std::string>("direction") == "up";

			std::string type = propertyTree.get<std::string>("type");
			if (type == "lclick") {
				state.tabManagers[id]->host->SendMouseClickEvent(cefMouseEvent, cef_mouse_button_type_t::MBT_LEFT, mouseUp, 1);
			} else if (type == "move") {
				state.tabManagers[id]->host->SendMouseMoveEvent(cefMouseEvent, false);
			} else if (type == "wheel") {
				state.tabManagers[id]->host->SendMouseWheelEvent(cefMouseEvent, propertyTree.get<double>("wheelDeltaX"), propertyTree.get<double>("wheelDeltaY"));
			}
		};
	}

	auto getActionKeyCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = request->path_match[1].str();
			if (state.tabManagers.find(id) == state.tabManagers.end()) {
				serveStaticFile(response, "/404.html", SimpleWeb::StatusCode::client_error_bad_request);
				return;
			}
			response->write(SimpleWeb::StatusCode::success_ok);

			// Handlers for different types of keys. Do inside lambda to initiate static const.
			static const std::map<std::pair<std::string, std::string>, std::function<void(TabManager *)>> keyHandlers = []() {
				std::map<std::pair<std::string, std::string>, std::function<void(TabManager *)>> keyHandlerProxy;

				// Helper functions.
				static const auto applyCefKeyEventModifiers = [](CefKeyEvent &cefKeyEvent, TabManager *tabManager) {
					if (tabManager->isShiftKeyDown)
						cefKeyEvent.modifiers |= EVENTFLAG_SHIFT_DOWN;
					if (tabManager->isControlKeyDown)
						cefKeyEvent.modifiers |= EVENTFLAG_CONTROL_DOWN;
					if (tabManager->isAltKeyDown)
						cefKeyEvent.modifiers |= EVENTFLAG_ALT_DOWN;
				};
				static const auto constructBasicCefKeyEventWithVkCode = [](CefKeyEvent &cefKeyEvent, BYTE vkCode) {
					UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
					cefKeyEvent.native_key_code = 0x00000001 | (LPARAM) (scanCode << 16);  // Scan code, repeat=1.
					cefKeyEvent.windows_key_code = vkCode;
				};
				static const auto constructBasicCefKeyEvent = [](CefKeyEvent &cefKeyEvent, CHAR character) {
					BYTE vkCode = LOBYTE(VkKeyScan(character));
					constructBasicCefKeyEventWithVkCode(cefKeyEvent, vkCode);
				};
				static const auto constructBasicDownKeyHandlerWithKeyCode = [](char keyCharacter, int keyCode) {
					return [keyCharacter, keyCode](TabManager *tabManager) {
						CefKeyEvent cefKeyEvent;
						constructBasicCefKeyEvent(cefKeyEvent, keyCharacter);
						applyCefKeyEventModifiers(cefKeyEvent, tabManager);
						cefKeyEvent.type = KEYEVENT_RAWKEYDOWN;
						tabManager->host->SendKeyEvent(cefKeyEvent);
						cefKeyEvent.windows_key_code = keyCode;
						cefKeyEvent.type = KEYEVENT_CHAR;
						tabManager->host->SendKeyEvent(cefKeyEvent);
					};
				};
				static const auto constructBasicDownKeyHandler = [](char keyCharacter) {
					return constructBasicDownKeyHandlerWithKeyCode(keyCharacter, keyCharacter);
				};
				static const auto constructNoCharDownKeyHandlerWithVkCode = [](BYTE vkCode) {
					return [vkCode](TabManager *tabManager) {
						CefKeyEvent cefKeyEvent;
						constructBasicCefKeyEventWithVkCode(cefKeyEvent, vkCode);
						applyCefKeyEventModifiers(cefKeyEvent, tabManager);
						cefKeyEvent.type = KEYEVENT_RAWKEYDOWN;
						tabManager->host->SendKeyEvent(cefKeyEvent);
					};
				};
				static const auto constructBasicUpKeyHandlerWithVkCode = [](BYTE vkCode) {
					return [vkCode](TabManager *tabManager) {
						CefKeyEvent cefKeyEvent;
						constructBasicCefKeyEventWithVkCode(cefKeyEvent, vkCode);
						applyCefKeyEventModifiers(cefKeyEvent, tabManager);
						cefKeyEvent.native_key_code |= 0xC0000000;  // Bits 30 and 31 should be always 1 for WM_KEYUP.
						cefKeyEvent.type = KEYEVENT_KEYUP;
						tabManager->host->SendKeyEvent(cefKeyEvent);
					};
				};
				static const auto constructBasicUpKeyHandler = [](char keyCharacter) {
					return [keyCharacter](TabManager *tabManager) {
						CefKeyEvent cefKeyEvent;
						constructBasicCefKeyEvent(cefKeyEvent, keyCharacter);
						applyCefKeyEventModifiers(cefKeyEvent, tabManager);
						cefKeyEvent.native_key_code |= 0xC0000000;  // Bits 30 and 31 should be always 1 for WM_KEYUP.
						cefKeyEvent.type = KEYEVENT_KEYUP;
						tabManager->host->SendKeyEvent(cefKeyEvent);
					};
				};

				// Most of the ASCII keys.
				for (char a : " !\"#$%&'()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz[\\]^_`{|}~") {
					keyHandlerProxy[std::make_pair("down", std::string(1, a))] = constructBasicDownKeyHandler(a);
					keyHandlerProxy[std::make_pair("up", std::string(1, a))] = constructBasicUpKeyHandler(a);
				}

				// Capitals.
				for (char a : "ABCDEFGHIJKLMNOPQRSTUVWXYZ") {
					keyHandlerProxy[std::make_pair("down", std::string(1, a))] = constructBasicDownKeyHandlerWithKeyCode(tolower(a), a);
					keyHandlerProxy[std::make_pair("up", std::string(1, a))] = constructBasicUpKeyHandler(tolower(a));
				}

				// Arrow keys.
				keyHandlerProxy[std::make_pair("down", "ArrowUp")] = constructNoCharDownKeyHandlerWithVkCode(VK_UP);
				keyHandlerProxy[std::make_pair("up", "ArrowUp")] = constructBasicUpKeyHandlerWithVkCode(VK_UP);
				keyHandlerProxy[std::make_pair("down", "ArrowRight")] = constructNoCharDownKeyHandlerWithVkCode(VK_RIGHT);
				keyHandlerProxy[std::make_pair("up", "ArrowRight")] = constructBasicUpKeyHandlerWithVkCode(VK_RIGHT);
				keyHandlerProxy[std::make_pair("down", "ArrowDown")] = constructNoCharDownKeyHandlerWithVkCode(VK_DOWN);
				keyHandlerProxy[std::make_pair("up", "ArrowDown")] = constructBasicUpKeyHandlerWithVkCode(VK_DOWN);
				keyHandlerProxy[std::make_pair("down", "ArrowLeft")] = constructNoCharDownKeyHandlerWithVkCode(VK_LEFT);
				keyHandlerProxy[std::make_pair("up", "ArrowLeft")] = constructBasicUpKeyHandlerWithVkCode(VK_LEFT);

				// Spacing.
				keyHandlerProxy[std::make_pair("down", "Enter")] = [](TabManager *tabManager) {
					CefKeyEvent cefKeyEvent;
					constructBasicCefKeyEventWithVkCode(cefKeyEvent, VK_RETURN);
					applyCefKeyEventModifiers(cefKeyEvent, tabManager);
					cefKeyEvent.type = KEYEVENT_RAWKEYDOWN;
					tabManager->host->SendKeyEvent(cefKeyEvent);
					cefKeyEvent.windows_key_code = '\r';
					cefKeyEvent.type = KEYEVENT_CHAR;
					tabManager->host->SendKeyEvent(cefKeyEvent);
					cefKeyEvent.windows_key_code = '\n';
					tabManager->host->SendKeyEvent(cefKeyEvent);
				};
				keyHandlerProxy[std::make_pair("up", "Enter")] = [](TabManager *tabManager) {
					CefKeyEvent cefKeyEvent;
					constructBasicCefKeyEventWithVkCode(cefKeyEvent, VK_RETURN);
					applyCefKeyEventModifiers(cefKeyEvent, tabManager);
					cefKeyEvent.native_key_code |= 0xC0000000;
					cefKeyEvent.type = KEYEVENT_KEYUP;
					tabManager->host->SendKeyEvent(cefKeyEvent);
				};
				// TODO: Why does the key event modifiers kill Backspace?
				keyHandlerProxy[std::make_pair("down", "Backspace")] = [](TabManager *tabManager) {
					CefKeyEvent cefKeyEvent;
					constructBasicCefKeyEventWithVkCode(cefKeyEvent, VK_BACK);
					// applyCefKeyEventModifiers(cefKeyEvent, tabManager);
					cefKeyEvent.type = KEYEVENT_RAWKEYDOWN;
					tabManager->host->SendKeyEvent(cefKeyEvent);
					cefKeyEvent.windows_key_code = '\b';
					cefKeyEvent.type = KEYEVENT_CHAR;
					tabManager->host->SendKeyEvent(cefKeyEvent);
				};
				keyHandlerProxy[std::make_pair("up", "Backspace")] = [](TabManager *tabManager) {
					CefKeyEvent cefKeyEvent;
					constructBasicCefKeyEventWithVkCode(cefKeyEvent, VK_BACK);
					// applyCefKeyEventModifiers(cefKeyEvent, tabManager);
					cefKeyEvent.native_key_code |= 0xC0000000;
					cefKeyEvent.type = KEYEVENT_KEYUP;
					tabManager->host->SendKeyEvent(cefKeyEvent);
				};
				keyHandlerProxy[std::make_pair("down", "Tab")] = constructBasicDownKeyHandlerWithKeyCode('\t', VK_TAB);
				keyHandlerProxy[std::make_pair("up", "Tab")] = constructBasicUpKeyHandlerWithVkCode(VK_TAB);
				keyHandlerProxy[std::make_pair("down", "Delete")] = constructNoCharDownKeyHandlerWithVkCode(VK_DELETE);
				keyHandlerProxy[std::make_pair("up", "Delete")] = constructBasicUpKeyHandlerWithVkCode(VK_DELETE);

				// Modifiers.
				keyHandlerProxy[std::make_pair("down", "Shift")] = [](TabManager *tabManager) {
					tabManager->isShiftKeyDown = true;
				};
				keyHandlerProxy[std::make_pair("up", "Shift")] = [](TabManager *tabManager) {
					tabManager->isShiftKeyDown = false;
				};
				keyHandlerProxy[std::make_pair("down", "Control")] = [](TabManager *tabManager) {
					tabManager->isControlKeyDown = true;
				};
				keyHandlerProxy[std::make_pair("up", "Control")] = [](TabManager *tabManager) {
					tabManager->isControlKeyDown = false;
				};
				keyHandlerProxy[std::make_pair("down", "Alt")] = [](TabManager *tabManager) {
					tabManager->isAltKeyDown = true;
				};
				keyHandlerProxy[std::make_pair("up", "Alt")] = [](TabManager *tabManager) {
					tabManager->isAltKeyDown = false;
				};

				return keyHandlerProxy;
			}();

			// Unwrap JSON.
			boost::property_tree::ptree propertyTree;
			std::istringstream iss(request->content.string());
			boost::property_tree::read_json(iss, propertyTree);

			std::string direction = propertyTree.get<std::string>("direction"),
				key = propertyTree.get<std::string>("key");

			// Construct the right parameters for CefKeyEvent.
			auto it = keyHandlers.find(std::make_pair(direction, key));
			if (it == keyHandlers.end()) {
				Rain::tsCout("Failed to handle key (", direction, ", ", key, ")!", Rain::CRLF);
				std::cout.flush();
			} else {
				it->second(state.tabManagers[id]);
			}
		};
	}

	auto getControlBackCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = request->path_match[1].str();
			if (state.tabManagers.find(id) == state.tabManagers.end()) {
				serveStaticFile(response, "/404.html", SimpleWeb::StatusCode::client_error_bad_request);
				return;
			}
			response->write(SimpleWeb::StatusCode::success_ok);

			state.tabManagers[id]->browser->GoBack();
		};
	}

	auto getControlForwardCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = request->path_match[1].str();
			if (state.tabManagers.find(id) == state.tabManagers.end()) {
				serveStaticFile(response, "/404.html", SimpleWeb::StatusCode::client_error_bad_request);
				return;
			}
			response->write(SimpleWeb::StatusCode::success_ok);

			state.tabManagers[id]->browser->GoForward();
		};
	}

	auto getControlReloadCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = request->path_match[1].str();
			if (state.tabManagers.find(id) == state.tabManagers.end()) {
				serveStaticFile(response, "/404.html", SimpleWeb::StatusCode::client_error_bad_request);
				return;
			}
			response->write(SimpleWeb::StatusCode::success_ok);

			state.tabManagers[id]->browser->Reload();
		};
	}

	auto getAccountCreateCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			response->write(SimpleWeb::StatusCode::success_ok);
		};
	}

	auto getAccountLoginCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			response->write(SimpleWeb::StatusCode::success_ok);
		};
	}

	auto getAccountLogoutCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			response->write(SimpleWeb::StatusCode::success_ok);
		};
	}

	auto getAccountInfoCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			response->write(SimpleWeb::StatusCode::success_ok);
		};
	}

	void initHttpServer(SimpleWeb::Server<SimpleWeb::HTTP> &server, State &state) {
		// Default.
		// TODO: Read from command line.
		server.config.port = 61001;

		// Single-threaded. Must change buffer in serveStaticFile if this is modified.
		server.config.thread_pool_size = 1;

		// Endpoints.
		server.default_resource["GET"] = getDefault;
		server.on_error = onError;

		server.resource["^/new$"]["GET"] = getNewCurried(state);
		server.resource["^/tab/(.+)$"]["GET"] = getTabCurried(state);
		server.resource["^/stream/(.+)$"]["GET"] = getStreamCurried(state);
		server.resource["^/action/(.+)/mouse$"]["POST"] = getActionMouseCurried(state);
		server.resource["^/action/(.+)/key"]["POST"] = getActionKeyCurried(state);
		server.resource["^/control/(.+)/back"]["POST"] = getControlBackCurried(state);
		server.resource["^/control/(.+)/forward"]["POST"] = getControlForwardCurried(state);
		server.resource["^/control/(.+)/reload"]["POST"] = getControlReloadCurried(state);
		server.resource["^/account/create"]["POST"] = getAccountCreateCurried(state);
		server.resource["^/account/login"]["POST"] = getAccountLoginCurried(state);
		server.resource["^/account/logout"]["POST"] = getAccountLogoutCurried(state);
		server.resource["^/account/info"]["POST"] = getAccountInfoCurried(state);
	}
}
