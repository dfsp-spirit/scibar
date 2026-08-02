## Developer Information for scibar


### Installing dev requirements

A C++17 compiler and CMake ≥ 3.14 are required. On Ubuntu/Debian, install the essentials:

```bash
sudo apt install build-essential cmake git
```

All other dependencies (Catch2 for testing) are bundled in the repository.

### Running the C++ tests

The tests use [Catch2](https://github.com/catchorg/Catch2) (bundled as an amalgamated header).
Build and run them from the repository root:

```bash
# Configure (if not already done)
cmake -B build

# Build the test target
cmake --build build --target scibar_tests

# Run the tests
cd build/cpp_tests && ./scibar_tests
```

Alternatively, use CTest:

```bash
ctest --test-dir build
```


### Making a release

* Bump version in `src/core/scibar/scibar.hpp` (see all four `#define` entries about version, like `#define SCIBAR_VERSION_MAJOR  1`)
* Bump version in `CMakeLists.txt`
* log recent changes in CHANGES file
* run the test (see above)
* run `doxygen Doxyfile` to re-generate API docs
* git tag the commit and `git push --tags`
* create release from the tag on github