// Taken from the rain-aeternum library.
#pragma once

#pragma comment (lib, "Gdiplus.lib")

#include "windows-lam-include.h"
#include <algorithm>

namespace Gdiplus
{
  using std::min;
  using std::max;
}

#include <Objidl.h> // Must come before Gdiplus.h.
#include <Gdiplus.h>
