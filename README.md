
# logger
C++ Logging Facade

## Purpose

This project provides a modern C++ logging facade that unifies multiple logging implementations (such as SimpleLogger, Log4cxx, and spdlog) under a single, consistent API. The facade enables you to:

- Switch logging implementations without changing application code
- Centralize logger configuration and management
- Write portable, testable, and maintainable logging code
- Integrate advanced features from different implementations as needed

It is ideal for projects that require flexibility, cross-platform support, and robust logging infrastructure.

## Example Usage

```cpp
#include <LoggerFactory.h>

using namespace sk::logger;

int main() {
  // Obtain a logger instance by name
  LoggerPtr logger = LoggerFactory::getInstance().getLogger("TestLogger");

  // Set log level
  logger->setLevel(Logger::Level::Info);

  // Log messages at different levels
  logger->info("Hello %s!", "world");
  logger->error("Something went wrong: %d", 42);
  logger->debug("Debugging value: %f", 3.14);

  return 0;
}
```
## Build Instructions

### 1. Install Conan (v2)

- On Windows or Linux, use pip:
  ```sh
  pip install conan
  ```
- Or see the official docs: https://docs.conan.io/2/installation.html


### 2. Install dependencies with Conan

From the project root, you can select which logger implementation to enable using Conan options:


**Available options:**
- `with_log4cxx` (default: False)
- `with_spdlog` (default: False)

**Example commands:**


- Enable Log4cxx implementation:
  ```sh
  conan install ./conan/conanfile.py --output-folder=build/conan --build=missing --profile:all=./conan/profile/windows -s:a build_type=Release -o with_log4cxx=True
  ```

- Enable spdlog implementation:
  ```sh
  conan install ./conan/conanfile.py --output-folder=build/conan --build=missing --profile:all=./conan/profile/windows -s:a build_type=Release -o with_spdlog=True
  ```


Replace `--profile:all=./conan/profile/windows` with `--profile:all=./conan/profile/linux` for Linux builds.

**Note for Linux debug builds:**
If your application or tests are built with `_GLIBCXX_DEBUG` (common for debug builds with GCC), you must also build all Conan dependencies with this define to avoid ABI incompatibility and runtime errors. Run Conan with:

```
conan install ./conan/conanfile.py --output-folder=build/conan --build=missing --profile:all=./conan/profile/linux -s:a build_type=Debug -c tools.build:defines=["_GLIBCXX_DEBUG"] -o with_log4cxx=True
```

This ensures consistent debug settings across your project and its dependencies.


### 3. Configure build and run tests with CMake

Use the provided CMakePresets to select the logger implementation and platform:

- **Windows (SimpleLogger):**
  ```sh
  cmake --preset windows-simplelogger
  cmake --build --preset windows-simplelogger-release
  ctest --preset windows-simplelogger-test
  ```

- **Windows (Log4Cxx):**
  ```sh
  cmake --preset windows-log4cxx
  cmake --build --preset windows-log4cxx-release
  ctest --preset windows-log4cxx-test
  ```

- **Linux (SimpleLogger):**
  ```sh
  cmake --preset linux-simplelogger
  cmake --build --preset linux-simplelogger-release
  ctest --preset simplelogger-log4cxx-test
  ```

- **Linux (Log4Cxx):**
  ```sh
  cmake --preset linux-log4cxx
  cmake --build --preset linux-log4cxx-release
  ctest --preset linux-log4cxx-test
  ```

---
For more details, see the CMakePresets.json and conanfile.txt files.
