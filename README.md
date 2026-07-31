# scibar
colorbars for the masses, in c++


<!-- badges: start -->
  [![tests_cpp](https://github.com/dfsp-spirit/scibar/actions/workflows/unittests.yml/badge.svg?branch=main)](https://github.com/dfsp-spirit/scibar/actions)
  [![examples_cpp](https://github.com/dfsp-spirit/scibar/actions/workflows/examples.yml/badge.svg?branch=main)](https://github.com/dfsp-spirit/scibar/actions)
  [![Docs](https://github.com/dfsp-spirit/scibar/actions/workflows/docs.yml/badge.svg?branch=main)](https://dfsp-spirit.github.io/scibar/)
<!-- badges: end -->


## About

scibar is a single-file, header only, C++17 library for plotting color bars to both vector and raster images, geared towards scientific visualization. It is also the natural fit for [scimesh](https://github.com/dfsp-spirit/scimesh).


![colorbar gallery](web/gallery.png)


## Features

- **Four scale types** — Linear, logarithmic, diverging, and categorical (qualitative) data scales
- **Built-in colormaps** — viridis (sequential) and vik (diverging), plus custom `std::vector<Color>` colormaps
- **Vector & raster output** — clean SVG with linear gradients, plus PPM (zero-dependency) and PNG (via stb)
- **Horizontal & vertical** — both orientations with independent layout control
- **Auto-generated ticks** — smart major and minor tick generation with configurable precision, plus custom tick labels
- **Colormap direction** — independently toggle axis inversion and colormap reversal
- **Light & dark themes** — `Style::defaultLight()` and `Style::defaultDark()`, fully customizable
- **Embedded font** — Inter Regular baked in, zero-config; drop in any `.ttf` for custom fonts
- **Two API levels** — `drawLegend()` for rapid prototyping, low-level `drawColorBar`/`drawTicks`/`drawTitle` for full control
- **Tick styling** — inward/outward ticks, sub-ticks on/off, configurable lengths


## Documentation

- 📖 [API Reference](https://dfsp-spirit.github.io/scibar/) — Doxygen-generated API docs
- 🚀 [Getting Started Guide](docs/GETTING_STARTED.md) — quick start and core concepts
- ❓ [FAQ](docs/FAQ.md) — common questions about colormap direction, ticks, layout, and more


## Quick Start


```c++
#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar.hpp"

#include <vector>

int main() {
    using namespace scibar;

    // Canvas: wide and short for a horizontal bar
    const int W = 600, H = 120;
    std::vector<uint32_t> buf(W * H);
    Canvas canvas{buf.data(), W, H};
    fillCanvas(canvas, Color{255, 255, 255, 255});

    // Data + colormap
    auto cmap = util::viridis();   // store locally — ColorMapView is non-owning
    Spec spec;
    spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
    spec.title    = "Temperature (°C)";
    spec.colormap = cmap;

    // Render
    Style style = Style::defaultLight();
    Rect barRect{50, 45, 500, 30};
    drawColorBar(canvas, barRect, spec, style, Orientation::Horizontal);
    drawTicks(canvas, barRect, spec, style, Orientation::Horizontal);

    // Save as PPM (no external dependencies)
    writePPM(canvas, "colorbar.ppm");

    return 0;
}
```

![minimal colorbar](web/example_outputs/minimal/colorbar.png)

See [here](./examples/minimal/) for the runnable example program to generate this image.



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

Our [full example applications](./examples/) use these additional tools:

* the `batlow` colormap by Fabio Crameri, https://www.fabiocrameri.ch/colourmaps/
* the toml++ header only library by Mark Gillard, https://github.com/marzer/tomlplusplus

Once more, thank you!

## Author & License

* License: [MIT](./LICENSE)
* Author: [Tim Schäfer](https://ts.rcmd.org)



