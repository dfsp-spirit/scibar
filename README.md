# scibar
colorbars for the masses, in c++

scibar is a header-only, single-purpose C++ library for rendering scientific colorbar legends. It is designed to be the "missing link" for lightweight renderers like scimesh, turning your raw data limits and colormaps into clear, informative visual legends.

## Scope: The "One-Thing" Philosophy

scibar does exactly one thing: it draws a rectangular colorbar, places ticks, and renders a text title (e.g., "Temperature in °C").

 * We do not: handle complex plots, axis labeling, data processing, or window management.

 * We do: take your range, your colormap, and your labels, and return a pixel buffer ready for display.

The Pipeline

* Input: User provides data range (min/max), clipping thresholds, a color lookup table, and a string title.

* Layout: scibar calculates normalized tick positions and text placement.

* Rasterize: Uses internal lightweight primitives to draw the bar and text into a provided buffer.

* Output: A raw RGBA pixel array.

Dependencies

We vendor these, see `src/third_party`.

* [canvas-ity](https://github.com/a-e-k/canvas_ity): For path/primitive drawing and gradients, ISC licence.

* [stb_truetype.h](https://github.com/nothings/stb): For font rasterization and text rendering, public omain.

* [catch2](https://github.com/catchorg/Catch2) (development only, in [cpp_tests/](./cpp_tests/)): For unit tests, BSL-1.0 license.


API Example


```c++
#include "scibar.hpp"

// Setup the legend
scibar::Legend legend;
legend.set_range(0.0f, 100.0f);           // Data bounds
legend.set_colormap(viridis_lut);        // Vector of colors
legend.set_title("Temperature in °C");   // Optional title

// Render to a pre-allocated buffer
std::vector<uint32_t> pixels(256 * 64);
legend.render(pixels.data(), 256, 64);

// Upload 'pixels' to a texture for your renderer,
// save as a PNG file, or whatever.
```