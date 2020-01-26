#include "tab-space-state.h"

std::string TabSpaceState::generateUniqueTabId() {
  static const int ID_LEN = 4;
  static const std::string alphabet =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz";

	// Generate a random number, and check that it isn't already used.
  while (true) {
    std::string id;
    for (int a = 0; a < ID_LEN; a++) {
      id += alphabet[rand() % alphabet.length()];
    }

    if (this->tabInfos.find(id) == this->tabInfos.end()) {
      return id;
    }

    // Otherwise try again.
  }
}
