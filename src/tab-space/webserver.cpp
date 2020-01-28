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

	auto getNewCurried(State &state) {
		return [&](std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
			std::string id = state.generateUniqueTabId();
			state.tabManagers[id] = new TabManager();
			TabManager &tabManager = *state.tabManagers[id];

			client::RootWindowConfig windowConfig;
			windowConfig.always_on_top = false;
			windowConfig.with_controls = false;
			windowConfig.with_osr = false;
			// windowConfig.url = "https://www.dunnbypaul.net/js_mouse/";
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
			cefMouseEvent.x = propertyTree.get<double>("x") * 1280 / 1.5;
			cefMouseEvent.y = propertyTree.get<double>("y") * 720 / 1.5;
			bool mouseUp = propertyTree.get<std::string>("direction") == "up";
			state.tabManagers[id]->host->SendMouseClickEvent(cefMouseEvent, cef_mouse_button_type_t::MBT_LEFT, mouseUp, 1);
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

			// Unwrap JSON.
			boost::property_tree::ptree propertyTree;
			std::istringstream iss(request->content.string());
			boost::property_tree::read_json(iss, propertyTree);

			std::string key = propertyTree.get<std::string>("key");
			WPARAM vk = 0x5A;
			UINT scan = MapVirtualKey(vk, 0);
			LPARAM lParam = 0x00000001 | (LPARAM) (scan << 16);  // Scan code, repeat=1

			CefKeyEvent cefKeyEvent;
			cefKeyEvent.windows_key_code = vk;
			cefKeyEvent.native_key_code = lParam;
			cefKeyEvent.is_system_key = 0;
			cefKeyEvent.type = KEYEVENT_RAWKEYDOWN;
			cefKeyEvent.modifiers = 0;
			state.tabManagers[id]->host->SendKeyEvent(cefKeyEvent);
		};
	}

	void getDefault(std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response> response, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request) {
		serveStaticFile(response, request->path, SimpleWeb::StatusCode::success_ok);
	}

	void onError(std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Request> request, const SimpleWeb::error_code &ec) {
		// Do nothing for now.
	}

	void initHttpServer(SimpleWeb::Server<SimpleWeb::HTTP> &server, State &state) {
		// Default.
		// TODO: Read from command line.
		server.config.port = 61001;

		// Single-threaded. Must change buffer in serveStaticFile if this is modified.
		server.config.thread_pool_size = 1;

		// Endpoints.
		server.resource["^/new$"]["GET"] = getNewCurried(state);
		server.resource["^/tab/(.+)$"]["GET"] = getTabCurried(state);
		server.resource["^/stream/(.+)$"]["GET"] = getStreamCurried(state);
		server.resource["^/action/(.+)/mouse$"]["POST"] = getActionMouseCurried(state);
		server.resource["^/action/(.+)/key"]["POST"] = getActionKeyCurried(state);

		server.default_resource["GET"] = getDefault;
		server.on_error = onError;
	}
}
