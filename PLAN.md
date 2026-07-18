# scibar
colorbars for the masses, in c++17

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

scibar uses `stb_truetype.h` for font rasterization. scibar comes with Inter (sans-serif) embedded, ensuring you get clean raster legends suitable for scientific figures out of the box.

* Included Font: Inter-Regular.ttf (embedded as a byte array).

* Customization / Flexibility: You can override the default font by passing your own stbtt_fontinfo pointer via the Style struct. This allows you to use any font you like (e.g., a standard sans-serif for academic figures, or a monospace font for technical labels).

* Why Inter? It is an open-source, journal-approved typeface optimized for legibility in data visualizations, ensuring your tick labels and titles remain crisp even when reduced for a manuscript.

* Flexibility: This allows you to use any font you like (e.g., a standard sans-serif for academic figures, or a monospace font for technical labels).




## Data Structures


```c++
#include <string_view>
#include <vector>
#include <utility>

enum class ScaleType { Linear, Logarithmic, Categorical };

struct Scale {
    ScaleType type = ScaleType::Linear;
    float range[2] = {0.0f, 1.0f};
    float midpoint = 0.0f; // Useful for Diverging/Log shifts
};

// 1. Opaque Font Wrapper
struct Font {
    const void* handle = nullptr; // Internal pointer to font engine
    float size = 14.0f;
};

// 2. Data Specification
struct Spec {
    Scale scale;

    const uint32_t* colormap = nullptr;
    std::string_view title;
    size_t colormap_size = 0;

    // Annotations
    std::vector<std::pair<float, std::string_view>> ticks;
};


// 3. Visual Style
struct Style {
    bool showFrame = true;
    uint32_t frameColor = 0xFF000000;
    uint32_t tickColor = 0xFF000000;
    uint32_t textColor = 0xFF000000;

    Font font;

    static Style defaultLight();
    static Style defaultDark();
};

// 4. Canvas Wrapper (C++17 compatible)
struct Canvas {
    uint32_t* pixels;
    int width;
    int height;

    Canvas(uint32_t* data, int w, int h)
        : pixels(data), width(w), height(h) {}

    // Helpful helper for the implementation
    size_t size() const { return static_cast<size_t>(width) * height; }
};
```




## API Example

This design uses a Canvas wrapper to clean up function calls and separates Style (theme/colors) from Spec (data/range).


### High-level API Example

```c++
#include "scibar.hpp"


// Setup
uint32_t my_buffer[200 * 600]; // Existing buffer from your engine, packed RGBA pixels
scibar::Canvas canvas(my_buffer, 200, 600);



scibar::Spec spec;
spec.scale.type = scibar::ScaleType::Logarithmic;
spec.scale.range[0] = 1.0f;
spec.scale.range[1] = 1000.0f;

spec.colormap = my_lut.data();
spec.colormap_size = my_lut.size();

scibar::Style style = scibar::Style::defaultDark();

// The "Smart" API: One function call, no math required.
// Internal logic handles positioning/spacing automatically.
scibar::drawLegend(canvas, spec, style);
```

### Low-level API example

```c++
#include "scibar.hpp"

uint32_t my_buffer[200 * 600]; // Existing buffer from your engine, packed RGBA pixels
scibar::Canvas canvas(my_buffer, 200, 600);
scibar::Spec spec = { {0.0f, 100.0f}, viridis_lut, {{0, "0"}, {100, "100"}} };
scibar::Style style = scibar::Style::defaultDark();

// Manual layout: User has full control over where every piece goes.
// You can even leave gaps or overlay other UI elements.

// 1. Draw the bar at a specific location
scibar::drawColorBar(canvas, {50, 50, 40, 500}, spec, style);

// 2. Draw ticks on the left side of the bar
scibar::drawTicks(canvas, {10, 50, 30, 500}, spec, style);

// 3. Draw a custom title at the top
scibar::drawTitle(canvas, {50, 10, 100, 30}, "Activation (μV)", style);
```


## Integration (How to use it in your app)

scibar is a single-header library. To integrate it:

* Drop `scibar.hpp` into your project.

* In exactly one `.cpp` file, define `#define SCIBAR_IMPLEMENTATION` before including the header.

* Use the library everywhere else normally.

This provides the simplicity of a header-only library while keeping your project's compile times fast, as the heavy rendering logic is only compiled once.



## Dependencies

We vendor these, see `src/third_party`.

* [canvas-ity](https://github.com/a-e-k/canvas_ity): For path/primitive drawing and gradients, ISC licence.

* [stb_truetype.h](https://github.com/nothings/stb): For font rasterization and text rendering, public domain.

* [catch2](https://github.com/catchorg/Catch2) (development only, in [cpp_tests/](./cpp_tests/)): For unit tests, BSL-1.0 license.
