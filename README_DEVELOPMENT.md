## Developer Information for scibar


### Installing dev requirements


### Running the C++ tests


### Making a release

* Bump version in `src/core/scibar.hpp` (see all four `#define` entries about version, like `#define SCIBAR_VERSION_MAJOR  1`)
* Bump version in `CMakeLists.txt`
* log recent changes in CHANGES file
* run the test (see above)
* run `doxygen Doxyfile` to re-generate API docs
* git tag the commit and `git push --tags`
* create release from the tag on github