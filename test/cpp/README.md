## Synopsis

This project mirrors and tests the C++ code samples used in "JavaScript/C++ Rosetta Stone" to verify they compile and work as advertised.

## Prerequisites

To build and run these tests, you'll need [CMake](https://cmake.org/) and a C++ compiler.

- On Windows, that probably means [Visual Studio](https://www.visualstudio.com/vs/community/).
- On Mac, [Xcode Command Line Tools](https://developer.apple.com/download/more/).
- And on Linux, [build-essential package](https://packages.ubuntu.com/xenial/build-essential).

## Build and test

```
cmake
cmake --build . --config Release
```

The build step will download C++ dependencies -- including Boost, so be patient. If all goes well, you should see `100% tests passed` near the end of the build output.

## Copyright

Copyright 2017 Jeff Mott. [MIT License](https://opensource.org/licenses/MIT).
