#pragma once

// Everything we need to know about a tab.
class TabInfo {
public:
	std::string id;

	scoped_refptr<client::RootWindow> rootWindow;
};
