#include <malloc.h>
#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

#include "webserver.h"
#include "state.h"

using namespace std;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;

namespace TabSpace {
	void serveStatic(shared_ptr<HttpServer::Response> response, std::string requestPath) {
		// Will respond with content in the static/-directory, and its subdirectories.
		// Default file: index.html
		// Can for instance be used to retrieve an HTML 5 client that uses REST-resources on this server
		try {
			// Previously was using canonical, but that didn't seem to work.
			// Using executable path to avoid Visual Studio debugging issues.
			auto web_root_path = boost::filesystem::absolute(boost::dll::program_location().parent_path() / "../../static");
			auto path = boost::filesystem::absolute(web_root_path / requestPath);

			// Check if path is within web_root_path
			if (distance(web_root_path.begin(), web_root_path.end()) > distance(path.begin(), path.end()) ||
				!equal(web_root_path.begin(), web_root_path.end(), path.begin()))
				throw invalid_argument("path must be within root path");
			if (boost::filesystem::is_directory(path))
				path /= "index.html";

			SimpleWeb::CaseInsensitiveMultimap header;

			header.emplace("Cache-Control", "private, no-cache, no-store, must-revalidate");
			header.emplace("Expires", "-1");
			header.emplace("Pragma", "no-cache");

			auto ifs = make_shared<ifstream>();
			ifs->open(path.string(), ifstream::in | ios::binary | ios::ate);

			if (*ifs) {
				auto length = ifs->tellg();
				ifs->seekg(0, ios::beg);

				header.emplace("Content-Length", to_string(length));
				response->write(header);

				// Trick to define a recursive function within this scope (for example purposes)
				class FileServer {
					public:
					static void read_and_send(const shared_ptr<HttpServer::Response> &response, const shared_ptr<ifstream> &ifs) {
						// Read and send 128 KB at a time
						static vector<char> buffer(131072); // Safe when server is running on one thread
						streamsize read_length;
						if ((read_length = ifs->read(&buffer[0], static_cast<streamsize>(buffer.size())).gcount()) > 0) {
							response->write(&buffer[0], read_length);
							if (read_length == static_cast<streamsize>(buffer.size())) {
								response->send([response, ifs](const SimpleWeb::error_code &ec) {
									if (!ec)
										read_and_send(response, ifs);
									else
										cerr << "Connection interrupted." << endl;
								}
								);
							}
						}
					}
				};
				FileServer::read_and_send(response, ifs);
			}
			else
				throw invalid_argument("could not read file");
		}
		catch (const exception & e) {
			response->write(SimpleWeb::StatusCode::client_error_bad_request, "Could not open path " + requestPath + ": " + e.what());
		}
	}

	void getInfo(shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
		stringstream stream;
		stream << "<h1>Request from " << request->remote_endpoint_address() << ":" << request->remote_endpoint_port() << "</h1>";

		stream << request->method << " " << request->path << " HTTP/" << request->http_version;

		stream << "<h2>Query Fields</h2>";
		auto query_fields = request->parse_query_string();
		for (auto &field : query_fields)
			stream << field.first << ": " << field.second << "<br>";

		stream << "<h2>Header Fields</h2>";
		for (auto &field : request->header)
			stream << field.first << ": " << field.second << "<br>";

		response->write(stream);
	}

	auto getNewCurried(State &state) {
		return [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
			std::string id = state.generateUniqueTabId();
			state.tabInfos[id] = TabInfo();
			TabInfo &tabInfo = state.tabInfos[id];

			client::RootWindowConfig windowConfig;
			windowConfig.always_on_top = false;
			windowConfig.with_controls = false;
			windowConfig.with_osr = false;
			// windowConfig.initially_hidden = true;
			windowConfig.bounds = CefRect(100, 100, tabInfo.width, tabInfo.height);
			scoped_refptr<client::RootWindow> rootWindow = state.context->GetRootWindowManager()->CreateRootWindow(windowConfig);

			// Setup tabInfo.
			tabInfo.id = id;
			tabInfo.rootWindow = rootWindow;
			tabInfo.jpegClsid = &state.jpegClsid;
			tabInfo.captureThread = std::thread([](TabInfo *pTabInfo) {
				pTabInfo->captureFunction();
			}, &tabInfo
			);

			// Hand off to main thread to complete tab initialization.
			// PostMessage(state.mainRWnd.hwnd, WM_RAINAVAILABLE, reinterpret_cast<WPARAM>(&tabInfo), 0);

			std::cout << "Launched new tab with ID " << id << "." << std::endl;
			response->write(id);
		};
	}

	void getTab(shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
		serveStatic(response, "/tab.html");
	}

	auto getStreamCurried(State &state) {
		return [&](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
			std::string id = request->path_match[1].str();
			std::cout << "MJPEG stream requested for tab ID " << id << std::endl;

			std::string s;
			s = "HTTP/1.1 200\r\n";
			s += "Content-Type: multipart/x-mixed-replace;boundary=frame\r\n\r\n";
			response->write(s.c_str(), s.length());
			while (true) {
				s = "";
				s += "--frame\r\n";
				s += "Content-Type: image/jpeg\r\n";
				char buffer[100];
				itoa(state.tabInfos[id].bufsize, buffer, 10);
				s += "Content-Length: " + std::string(buffer) + "\r\n";
				s += "\r\n";
				response->write(s.c_str(), s.length());
				response->write(state.tabInfos[id].data, state.tabInfos[id].bufsize);
				response->write("\r\n\r\n", 4);
				response->send();
				//std::cout << "Sent some data." << std::endl;
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		};
	}

	void getDefault(shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
		// Default GET-example. If no other matches, this anonymous function will be called.
		serveStatic(response, request->path);
	}

	void httpServerError(shared_ptr<HttpServer::Request> request, const SimpleWeb::error_code &ec) {
		// Handle errors here
		// Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
		// cout << "Http error: " << ec << endl;
	}

	void setupHttpServer(SimpleWeb::Server<SimpleWeb::HTTP> &server, State &state) {
		server.config.port = 61001;
		server.config.thread_pool_size = 8;
		server.resource["^/info$"]["GET"] = getInfo;
		server.resource["^/new$"]["GET"] = getNewCurried(state);
		server.resource["^/tab/.+$"]["GET"] = getTab;
		server.resource["^/stream/(.+)$"]["GET"] = getStreamCurried(state);
		server.default_resource["GET"] = getDefault;
		server.on_error = httpServerError;
	}
}
