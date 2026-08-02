# Getting Started with scibar {#getting_started}

A gentle introduction to using the scibar C++ colorbar rendering library.
No GPU, display server, or external dependencies required — just a C++17
compiler.

## What is scibar?

scibar is a **single-header C++17 library** for rendering scientific
colorbars.  It produces:

- **Raster output** (packed RGBA pixel buffers) for real-time overlays,
  HUDs, and texture maps
- **Vector output** (clean SVG) for publication figures, including hybrid
  figures that embed a raster image alongside vector ticks and typography

scibar is **not** a charting library, a GUI toolkit, or a layout engine.
It draws exactly the elements you ask for at exactly the coordinates you
specify.

## Prerequisites

- A C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or newer (for examples and tests)

## Quick Start — Single Header Drop-in

Copy `src/core/scibar/scibar.hpp` into your project.  In **exactly one** `.cpp` file:

```cpp
#define SCIBAR_IMPLEMENTATION
#include "scibar.hpp"
```

In all other files:

```cpp
#include "scibar.hpp"
```

That's it.  No libraries to link, no external dependencies at runtime.
The bundled third-party code (canvas_ity, stb_truetype) is compiled
automatically.

## Minimal Example — A Vertical Colorbar

```cpp
#define SCIBAR_IMPLEMENTATION
#include "scibar.hpp"
#include <fstream>

int main() {
    using namespace scibar;

    // 1. Create a pixel canvas
    constexpr int W = 200, H = 600;
    std::vector<uint32_t> buf(W * H);
    Canvas canvas{buf.data(), W, H};

    // 2. Define the data domain and colormap
    auto cmap = util::viridis();   // built-in 256-entry colormap
    Spec spec;
    spec.scale = Scale{ScaleType::Linear, 0.0f, 100.0f};
    spec.title = "Temperature (°C)";
    spec.colormap = cmap;

    // 3. Render with sensible defaults
    drawLegend(canvas, spec, Style::defaultLight());

    // 4. Save as PPM (or PNG with stb_image_write)
    writePPM(canvas, "colorbar.ppm");

    return 0;
}
```

## Minimal Example — SVG Export

```cpp
#define SCIBAR_IMPLEMENTATION
#include "scibar.hpp"
#include <fstream>

int main() {
    using namespace scibar;

    auto cmap = util::vik();   // diverging colormap
    Spec spec;
    spec.scale = Scale{ScaleType::Linear, -3.5f, 3.5f};
    spec.title = "Z-Score";
    spec.colormap = cmap;

    // Hybrid figure: embed a raster image alongside the vector colorbar
    SVGOptions opts;
    opts.totalWidth  = 800;
    opts.totalHeight = 600;
    opts.mainImageHref = "rendered_mesh.png";
    opts.mainImageBounds = {20, 20, 550, 550};
    opts.colorbarBounds  = {600, 50, 120, 500};

    std::string svg = exportToSVG(spec, Style::defaultLight(), opts);

    std::ofstream out("figure.svg");
    out << svg;

    return 0;
}
```

## Core Concepts

### Data Model

| Struct | Purpose |
|--------|---------|
| `Scale` | The data domain — min, max, scale type, and direction |
| `ColorMapView` | Non-owning view of an RGBA colormap (vector or raw array) |
| `Spec` | Complete specification: scale + colormap + ticks + title |
| `Style` | Visual theme: colors, fonts, tick sizes, layout flags |
| `Canvas` | Packed RGBA pixel buffer (for raster output) |
| `Rect` | Axis-aligned integer rectangle for layout |
| `SVGOptions` | SVG canvas size and element placement |

### Scale Types

| Type | When to use |
|------|------------|
| `ScaleType::Linear` | Uniform data spacing (temperature, distance) |
| `ScaleType::Logarithmic` | Data spanning multiple orders of magnitude |
| `ScaleType::Diverging` | Data centered around a midpoint (± deviations) |
| `ScaleType::Categorical` | Discrete categories with equal-width segments |

### Built-in Colormaps

```cpp
#include "scibar.hpp"
// Returns 256-entry vectors, backed by static data — cheap to call repeatedly.
auto viridis = scibar::util::viridis();  // Perceptually-uniform sequential
auto vik     = scibar::util::vik();      // Diverging blue→yellow→red (Crameri)
```

### Two API Levels

**High-level** — `drawLegend()` applies sensible defaults for a vertical
colorbar filling the entire canvas.  Good for rapid prototyping and debug
overlays.

**Low-level** — `drawColorBar()`, `drawTicks()`, `drawSubTicks()`,
`drawTitle()` give you pixel-level control over every element.  Use these
for publication-quality output.

### Font Handling

scibar embeds the **Inter** font (SIL Open Font License).  When
`Font::handle == nullptr`, the embedded font is used automatically — zero
configuration required.

To use a custom font during development:

```cpp
scibar::Font font = scibar::loadFont("path/to/font.ttf", 14.0f);
style.font = font;
```

### Two Output Backends

```
                         ┌──────────────────────────────┐
                         │    scibar::Spec / Style      │
                         │  (data, colormap, theming)   │
                         └────────────┬─────────────────┘
                                      │
           ┌──────────────────────────┴──────────────────────────┐
           ▼                                                     ▼
┌───────────────────────────┐                         ┌───────────────────────────┐
│     Pixel Buffer Target   │                         │     SVG Vector Target     │
│  (stb_truetype + canvas)  │                         │ (String-stream output)    │
│  For real-time viewports  │                         │ For journal manuscripts   │
└───────────────────────────┘                         └───────────────────────────┘
```

## Performance Tips

- The `ColorMapView` is non-owning — pass a persistent vector or static
  array, not a temporary.
- Built-in colormaps (`viridis()`, `vik()`) use static data — zero
  allocation on repeated calls.
- For real-time overlays, reuse the same `Canvas` buffer and call
  `fillCanvas()` to clear before each frame.
- SVG output for continuous scales uses `<linearGradient>` with discrete
  `<stop>` elements — far smaller than per-pixel output.

## Building the Examples

```bash
cd scibar
cmake -B build -DSCIBAR_BUILD_EXAMPLES=ON
cmake --build build
./build/examples/linear_vertical_viridis/linear_vertical_viridis
./build/examples/full_CLI_demo_app/full_CLI_demo_app
```

Alternatively, use the convenience script to build and run all examples at once:

```bash
./examples/run_all_examples.sh
```

## Building and Running Tests

```bash
cmake -B build -DSCIBAR_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Next Steps

- Browse the [API Reference](annotated.html) for detailed function and
  struct documentation.
- Check the [FAQ](md_docs_FAQ.html) for common questions about colormap
  direction, tick formatting, and layout.
- See the `examples/` directory for 20+ complete example programs
  covering all scale types, orientations, and colormap directions.
