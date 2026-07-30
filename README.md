# scibar
colorbars for the masses, in c++


<!-- badges: start -->
  [![tests_cpp](https://github.com/dfsp-spirit/scibar/actions/workflows/unittests.yml/badge.svg?branch=main)](https://github.com/dfsp-spirit/scibar/actions)
<!-- badges: end -->


## About

scibar is a single-file, header only, C++17 library for plotting color bars to both vector and raster images, geared towards scientific visualization. It is also the natural fit for [scimesh](https://github.com/dfsp-spirit/scimesh).


## Usage

Not yet, this is work in progress.


## Acknowledgements


Thanks heaps to the authors of these great software packages that scibar is built upon:

* `src/third_party/canvas_ity.hpp`: [canvas_ity](https://github.com/a-e-k/canvas_ity) by Andrew Kensler
* `cpp_tests/catch_amalgamated.{h,cpp}`: [catchorg/Catch2](https://github.com/catchorg/Catch2/tree/devel/extras) — C++ test framework by Martin Hořeňovský
* `src/third_party/stb_image.h`, `src/third_party/stb_image_write.h`, `src/third_party/stb_truetype.h`: [nothings/stb](https://github.com/nothings/stb) — image loading/saving + truetype, maintained by Sean Barrett
* the `vik` diverging colormap by Fabio Crameri, https://www.fabiocrameri.ch/colourmaps/ (embedded in code)
* the `viridis` colormap by Stéfan van der Walt and Nathaniel Smith, (embedded in code), see following paper:

```latex
@inproceedings{smith2015viridis,
  author    = {Nathaniel Smith and St{\'e}fan van der Walt},
  title     = {A Better Default Colormap for Matplotlib},
  booktitle = {SciPy 2015: 14th Python in Science Conference},
  year      = {2015},
  url       = {https://www.youtube.com/watch?v=xAoljeRJ3lU}
}
```

And of course, thanks to the authors of the dependencies of these packages, and their dependencies...


## Author & License

* License: [MIT](./LICENSE)
* Author: [Tim Schäfer](https://ts.rcmd.org)



