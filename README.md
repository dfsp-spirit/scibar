# scibar
colorbars for the masses, in c++

scibar is a header-only, single-purpose C++ library for rendering scientific colorbar legends. It is designed to be the "missing link" for lightweight renderers like [scimesh](https://github.com/dfsp-spirit/scimesh), turning your raw data limits and colormaps into clear, informative visual legends.


## Scope: The "One-Thing" Philosophy

scibar is a lightweight, header-only C++ library for rendering scientific colorbar legends. It is designed to be the missing link between your raw scientific data and a polished visual presentation.

* We do: Render colorbars, tick marks, and custom labels into an RGBA pixel buffer.

* We do not: Perform data analysis, manage windowing, or provide complex charting frameworks.

Design: scibar is backend-agnostic. Whether you are using OpenGL, Vulkan, or a software renderer, you simply overlay the resulting pixel buffer onto your scene.


## The Pipeline

scibar follows a "composition over configuration" philosophy. Rather than a "black box" that guesses your layout, we provide specialized drawing primitives that you place exactly where they belong.

* Canvas Wrapper: Initialize a scibar::Canvas to manage your raw pixel buffer and dimensions.

* Define Style & Spec: Separate your visual appearance (Style) from your data bounds and labels (Spec).

* Compose: Use primitive drawing functions to paint components onto the canvas at specific coordinates.

* Output: Retrieve the pixel array, ready for texture upload or image export.


## Font Handling

scibar uses `stb_truetype.h` for font rasterization. To minimize dependencies and keep the library portable, scibar does not ship with built-in fonts.

* Setup: Load your .ttf file in your application using stbtt_InitFont().

* Usage: Pass the resulting stbtt_fontinfo pointer into your scibar::Style object.

* Flexibility: This allows you to use any font you like (e.g., a standard sans-serif for academic figures, or a monospace font for technical labels).


## Data Structures


```c++
// Defines the data range, mapping, and specific markers
struct Spec {
    float range[2];             // {min, max} for the colormap
    float midpoint;             // For diverging maps (e.g., 0.0)
    std::vector<uint32_t> colormap; // 256 RGBA colors

    // Custom tick labels: {value, label_text}
    std::vector<std::pair<float, std::string>> ticks;
};

// Defines the visual appearance and font configuration
struct Style {
    bool showFrame = true;      // Draw a frame around the colorbar
    uint32_t frameColor = 0xFF000000;
    uint32_t tickColor = 0xFF000000;
    uint32_t textColor = 0xFF000000;

    // Font configuration (must be initialized by the user)
    const stbtt_fontinfo* font;
    float fontSize = 14.0f;

    // Factory methods for quick setup
    static Style defaultLight();
    static Style defaultDark();
};
```

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

Another example for a divergent colormap:

```c++
// ...
// --- Divergent Example ---
scibar::Style style = scibar::Style::defaultLight();
scibar::Spec spec;

spec.range = {-1.0f, 1.0f};          // Divergent range (centered on 0)
spec.colormap = coolwarm_lut;        // Diverging LUT
spec.midpoint = 0.0f;                // Forces 0 to map to the neutral color

// The tick labels now clearly indicate the divergence
spec.ticks = {{-1.0f, "-1"}, {0.0f, "0"}, {1.0f, "1"}};

scibar::drawColorBar(canvas, {50, 50, 40, 500}, spec, style);
scibar::drawTicks(canvas, {90, 50, 60, 500}, spec, style);
```



## Dependencies

We vendor these, see `src/third_party`.

* [canvas-ity](https://github.com/a-e-k/canvas_ity): For path/primitive drawing and gradients, ISC licence.

* [stb_truetype.h](https://github.com/nothings/stb): For font rasterization and text rendering, public domain.

* [catch2](https://github.com/catchorg/Catch2) (development only, in [cpp_tests/](./cpp_tests/)): For unit tests, BSL-1.0 license.
