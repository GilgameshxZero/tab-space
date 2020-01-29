#include <boost/filesystem.hpp>
#include <boost/dll.hpp>

#include "state.h"

#include <iostream>
#include <fstream>

namespace TabSpace {
	std::string State::generateUniqueTabId() {
		static const int ID_LEN = 5;
		static const std::string alphabet =
			"0123456789"
			"abcdefghijklmnopqrstuvwxyz";

		// Generate a random number, and check that it isn't already used.
		while (true) {
			std::string id;
			for (int a = 0; a < ID_LEN; a++) {
				id += alphabet[this->rng() % alphabet.length()];
			}

			if (this->tabManagers.find(id) == this->tabManagers.end()) {
				return id;
			}

			// Otherwise try again.
		}
	}

	void State::saveUserLoginInfo() {
		this->userLoginMutex.lock();
		auto userLoginInfoPath = boost::filesystem::absolute(boost::dll::program_location().parent_path() / "../../db/user-login-info.json");
		std::ofstream ofs(userLoginInfoPath.string(), std::ifstream::out | std::ios::binary);
		if (ofs) {
			ofs << this->userLoginInfo.size() << Rain::CRLF;
			for (auto loginInfo : this->userLoginInfo) {
				ofs << loginInfo.first << " " << loginInfo.second << Rain::CRLF;
			}
			ofs.close();
			Rain::tsCout("Saved ", this->userLoginInfo.size(), " users.", Rain::CRLF);
			std::cout.flush();
		}
		this->userLoginMutex.unlock();
	}

	std::string State::generateLoginToken() {
		static const int TOKEN_LEN = 25;
		static const std::string alphabet =
			"0123456789"
			"abcdefghijklmnopqrstuvwxyz";

		while (true) {
			std::string token;
			for (int a = 0; a < TOKEN_LEN; a++) {
				token += alphabet[this->rng() % alphabet.length()];
			}
			if (this->userLoginToken.find(token) == this->userLoginToken.end()) {
				return token;
			}
		}
	}
}
