# scibar
colorbars for the masses, in c++



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

*Note: This section is to give credit only. Users do **not** need to worry about installing these dependencies, they come vendored with scimesh already.*



