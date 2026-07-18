# scibar
colorbars for the masses, in c++

scibar is a header-only, single-purpose C++ library for rendering scientific colorbar legends. It is designed to be the "missing link" for lightweight renderers like scimesh, turning your raw data limits and colormaps into clear, informative visual legends.

## Scope: The "One-Thing" Philosophy

scibar does exactly one thing: it draws a rectangular colorbar, places ticks, and renders a text title (e.g., "Temperature in °C").

 * We do not: handle complex plots, axis labeling, data processing, or window management.

 * We do: take your range, your colormap, and your labels, and return a pixel buffer ready for display.

## The Pipeline

scibar follows a "composition over configuration" philosophy. We don't try to guess your layout; we provide the drawing primitives, and you place them.

Instead of one "black box" function, you have granular control:


* Define a Canvas: Initialize a raw RGBA pixel buffer of your desired output size.

* Compose: Call scibar primitive functions to draw components (the bar, the ticks, the title) onto that buffer at specific coordinates.

* Rasterize: scibar uses lightweight primitives to draw shapes and fonts directly into your buffer.

* Output: You are left with your modified pixel array, ready for GPU-texture upload or PNG export.


## API Example

```c++
#include "scibar.hpp"

// 1. Prepare your canvas (e.g., 200x600)
std::vector<uint32_t> canvas(200 * 600, 0x00000000);

// 2. Define global state
scibar::Config config;
config.range = {0.0f, 100.0f};
config.colormap = viridis_lut; // std::vector<uint32_t>, defining 256 RBGA colors

// 3. Compose your legend (User controls placement/padding)
// These functions operate directly on the canvas buffer
scibar::drawColorBar(canvas, 200, 600, {50, 50, 40, 500}, config);
scibar::drawTicks(canvas, 200, 600, {90, 50, 60, 500}, config);
scibar::drawTitle(canvas, 200, 600, {20, 20, 160, 30}, "Temp (°C)");

// Now 'canvas' contains the fully composed legend.
```

## Dependencies

We vendor these, see `src/third_party`.

* [canvas-ity](https://github.com/a-e-k/canvas_ity): For path/primitive drawing and gradients, ISC licence.

* [stb_truetype.h](https://github.com/nothings/stb): For font rasterization and text rendering, public omain.

* [catch2](https://github.com/catchorg/Catch2) (development only, in [cpp_tests/](./cpp_tests/)): For unit tests, BSL-1.0 license.
