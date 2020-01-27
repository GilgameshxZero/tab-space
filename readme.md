# tab-space

A shared space for your internet browser tabs.

## Development

We refer to the directory containing this `readme` as the *root directory* of the project, at `./`.

### Visual Studio 2019

`tab-space` uses Visual Studio 2019 as an IDE. The solution and project files are located under `./sln/`. The project file includes the relevant headers, DLLs, and LIBs already for the build version of the dependencies of `tab-space`. However, when upgrading, the paths to these files may change and need to be updated.

#### Debug

Without `libcef.dll.pdb`, debug builds are pretty useless. So, always build for release.

### Chromium Embedded Framework

`tab-space` uses the Chromium Embedded Framework (CEF). We do not use sandboxing as it is unreliable on different hardware. Download the relevant binary bundle from <http://opensource.spotify.com/cefbuilds/index.html>. The project builds are based on `cef_binary_79.1.31+gfc9ef34+chromium-79.0.3945.117_windows64.tar.bz2`. Unzip the bundle into `./cef_binary_*/` and `cd` into the directory.

Use

```batch
cmake -G "Visual Studio 16 2019" -S . -B build
```

to generate `./cef_binary_*/cef.sln`. Opening the solution in Visual Studio, make the `libcef_dll_wrapper` project under both `Debug` and `Release` configurations. The paths to the generated binaries are included in the Visual Studio project for `tab-space` already.

### CMake

`tab-space` uses `cmake` as part of its dependence on CEF. Please add `cmake` to the user PATH or system PATH, after installing via <https://cmake.org/download/>.

### Simple Web Server

`tab-space` uses the Simple Web Server by `eidheim`, `v3.0.2`. Download the relevant bundle from <https://gitlab.com/eidheim/Simple-Web-Server/-/releases>, and unzip it into `./Simple-Web-Server-*/`.

### Boost

`tab-space` uses the Boost C++ Libraries, version `1.72.0`. Downloads are at <https://www.boost.org/users/download/>. `tab-space` uses statically linked libraries, which can be generated by running

```batch
.\b2 runtime-link=static
```

in the Boost install directory. If Boost is already installed on the system, one may symbolically link it into `./boost_*`. The appropriate library and include paths have already been included in the project configuration.

#### `_alloca`: Identifier not found

You got this error? Sucks to be you, I guess.

Try `#include <malloc.h>` in the file that it complains about.

### HTTP Multiplexer

`tab-space` is deployed at <https://tab.gilgamesh.cc> via [http-mux](https://github.com/GilgameshxZero/http-mux) running on an AWS EC2 instance. The relevant tunneling command from the `tab-space` server is

```bash
ssh -p 2222 -R tab.gilgamesh.cc:80:127.0.0.1:61001 gilgamesh.cc
```

### Emilia/rain-aeternum

`tab-space` may transition to using the `rain-aeternum` library for its webserver, primarily used in `Emilia`. The code for `rain-aeternum` is included in source control already.

## Endpoints

`tab-space` reveals several endpoints for the web client.

Endpoint|Usage
-|-
`/`|Homepage
`/info`|Testing endpoint. Should return some basic information about the HTTP multiplexer.
`/new`|Launches tab on the backend. Returns tab ID.
`/tab/.+`|Proxy for `tab.html`, with some tab ID enforced.
`/stream/.+`|MJPEG stream for some tab ID.
