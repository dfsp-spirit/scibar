# scibar
colorbars for the masses, in c++

scibar is a header-only, single-purpose C++ library for rendering scientific colorbar legends. It is designed to be the "missing link" for lightweight renderers like [scimesh](https://github.com/dfsp-spirit/scimesh), turning your raw data limits and colormaps into clear, informative visual legends.

## Scope: The "One-Thing" Philosophy

scibar is a lightweight, header-only C++ library for rendering scientific colorbar legends. It is designed to be the missing link between your raw scientific data and a polished visual presentation.

    We do: Render colorbars, tick marks, and custom labels into an RGBA pixel buffer.

    We do not: Perform data analysis, manage windowing, or provide complex charting frameworks.

    Design: scibar is backend-agnostic. Whether you are using OpenGL, Vulkan, or a software renderer, you simply overlay the resulting pixel buffer onto your scene.

## The Pipeline

scibar follows a "composition over configuration" philosophy. Rather than a "black box" that guesses your layout, we provide specialized drawing primitives that you place exactly where they belong.

    Canvas Wrapper: Initialize a scibar::Canvas to manage your raw pixel buffer and dimensions.

    Define Style & Spec: Separate your visual appearance (Style) from your data bounds and labels (Spec).

    Compose: Use primitive drawing functions to paint components onto the canvas at specific coordinates.

    Output: Retrieve the pixel array, ready for texture upload or image export.

## API Example

This design uses a Canvas wrapper to clean up function calls and separates Style (theme/colors) from Spec (data/range).

```c++
#include "scibar.hpp"

// 1. Prepare your canvas
scibar::Canvas canvas(200, 600); // Manages buffer and dimensions

// 2. Define style and data spec
scibar::Style style = scibar::Style::defaultDark();
scibar::Spec spec;
spec.range = {0.0f, 100.0f};      // Data range
spec.colormap = viridis_lut;      // std::vector<uint32_t> (RGBA)
spec.ticks = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};

// 3. Compose: Primitive functions take a rect {x, y, w, h}
// All functions operate directly on the canvas buffer
scibar::drawColorBar(canvas, {50, 50, 40, 500}, spec, style);
scibar::drawTicks(canvas, {90, 50, 60, 500}, spec, style);
scibar::drawTitle(canvas, {20, 20, 160, 30}, "Temp (°C)", style);

// Now 'canvas.data()' contains the fully composed, frame-bordered legend.
```


## Implementation Notes

* The Canvas Wrapper: Creating a struct Canvas { std::vector<uint32_t> buffer; int w, h; }; is a major win for API cleanliness.
* Decoupling: By splitting Style (colors, frame settings) from Spec (range, ticks, LUT), we prevent parameter bloat. We can now change the "Dark Mode" theme without touching the data logic.
* Manual Control: By using a std::vector of tick structs ({float value, std::string label}), we satisfy the need for scientific precision (e.g., handling $\pi$ or scientific notation) without needing a complex "nice number" algorithm on day one.


## Dependencies

We vendor these, see `src/third_party`.

* [canvas-ity](https://github.com/a-e-k/canvas_ity): For path/primitive drawing and gradients, ISC licence.

* [stb_truetype.h](https://github.com/nothings/stb): For font rasterization and text rendering, public omain.

* [catch2](https://github.com/catchorg/Catch2) (development only, in [cpp_tests/](./cpp_tests/)): For unit tests, BSL-1.0 license.
