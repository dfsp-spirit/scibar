# scibar
colorbars for the masses, in c++

scibar is a header-only, single-purpose C++ library for rendering scientific colorbar legends. It is designed to be the "missing link" for lightweight renderers like [scimesh](https://github.com/dfsp-spirit/scimesh), turning your raw data limits and colormaps into clear, informative visual legends.

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

// 1. Prepare your canvas (e.g., 200x600 pixels)
// Using 0x00000000 for a transparent background
std::vector<uint32_t> canvas(200 * 600, 0x00000000);

// 2. Define global style and data state
// We use a dark theme preset and then tweak the colormap
scibar::Config config = scibar::Config::defaultDark();
config.range = {0.0f, 100.0f};  // The data range being visualized
config.colormap = viridis_lut;  // std::vector<u_int32t> of RGBA colors

// 3. Compose your legend
// Rect struct format: { x, y, width, height }
// All coordinates are relative to the top-left of the canvas.

// The draw functions use the style/colors defined in 'config'
// drawColorBar automatically handles the frame if config.showFrame is true
scibar::drawColorBar(canvas, 200, 600, {50, 50, 40, 500}, config);

// Draw ticks at the edge of the bar
scibar::drawTicks(canvas, 200, 600, {90, 50, 60, 500}, config);

// Draw the title (using the text color defined in config)
scibar::drawTitle(canvas, 200, 600, {20, 20, 160, 30}, "Temp (°C)");

// Now 'canvas' contains the fully composed, frame-bordered legend.
```

## Dependencies

We vendor these, see `src/third_party`.

* [canvas-ity](https://github.com/a-e-k/canvas_ity): For path/primitive drawing and gradients, ISC licence.

* [stb_truetype.h](https://github.com/nothings/stb): For font rasterization and text rendering, public omain.

* [catch2](https://github.com/catchorg/Catch2) (development only, in [cpp_tests/](./cpp_tests/)): For unit tests, BSL-1.0 license.
