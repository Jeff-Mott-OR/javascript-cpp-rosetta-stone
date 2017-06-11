## Synopsis

This project mirrors and tests the C++ code samples used in "JavaScript/C++ Rosetta Stone" to verify they compile and work as advertised.

## Prerequisites

To build and run these tests, you'll need [CMake](https://cmake.org/) and a C++ compiler.

- On Windows, that probably means [Visual Studio](https://www.visualstudio.com/vs/community/).
- On Mac, [Xcode Command Line Tools](https://developer.apple.com/download/more/).
- And on Linux, [build-essential package](https://packages.ubuntu.com/xenial/build-essential).

## Usage

```
mkdir build
cd build
cmake ..
```

Then, in Mac or Linux environments, run `make`, or in a Windows environment, open and build the generated Visual Studio solution.

The make/build step will download C++ dependencies (including Boost, so be patient), then build and run the test executable. If all goes well, the last output you should see in the build log is:

```
Running N test cases...

*** No errors detected

```

## Copyright

Copyright 2017 Jeff Mott. [MIT License](https://opensource.org/licenses/MIT).
