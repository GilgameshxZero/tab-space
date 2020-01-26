#pragma once

#include "client_http.hpp"
#include "server_http.hpp"

#include "../tab-space/tab-space-state.h"

void setupHttpServer(SimpleWeb::Server<SimpleWeb::HTTP> &server, TabSpaceState &tabSpaceState);
