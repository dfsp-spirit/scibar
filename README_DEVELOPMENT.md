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


### Running all Examples

In repo root:

```bash
./examples/run_all_examples.sh
```


### Regenerating the API docs

The API docs are generated with Doxygen into `docs/api/html/` (git-ignored) and deployed
to GitHub Pages from CI (see `.github/workflows/docs.yml`).

**Doxygen version requirement:** use Doxygen >= 1.10.0. Older versions (including the
1.9.8 package shipped with Ubuntu 24.04) escape inline code spans in Markdown headings
as literal `<tt>` text instead of rendering them (doxygen issue #10466, fixed in 1.10.0).
The CI installs a pinned recent binary from the official Doxygen releases, so use the same
version locally:

```bash
# e.g. from https://github.com/doxygen/doxygen/releases
curl -sL -o /tmp/doxygen.tar.gz \
  https://github.com/doxygen/doxygen/releases/download/Release_1_13_2/doxygen-1.13.2.linux.bin.tar.gz
tar xzf /tmp/doxygen.tar.gz -C /tmp
export PATH="/tmp/doxygen-1.13.2/bin:$PATH"
doxygen --version   # should print 1.13.2
doxygen Doxyfile    # regenerates docs/api/html/
```


### Making a release

* Bump version in `src/core/scibar/scibar.hpp` (see all four `#define` entries about version, like `#define SCIBAR_VERSION_MAJOR  1`)
* Bump version in `CMakeLists.txt`
* log recent changes in CHANGES file
* run the tests (see above)
* run all examples (see above)
* run `doxygen Doxyfile` to re-generate API docs
* git tag the commit and `git push --tags`
* create release from the tag on github