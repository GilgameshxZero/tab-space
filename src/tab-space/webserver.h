#pragma once

#include "client_http.hpp"
#include "server_http.hpp"

#include "state.h"

namespace TabSpace {
	void setupHttpServer(SimpleWeb::Server<SimpleWeb::HTTP> &server, State &state);
}
