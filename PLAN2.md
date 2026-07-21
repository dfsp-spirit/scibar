# scibar: Architecture & Specification Plan

**Single-Header Scientific Colorbar Rendering for C++17**

`scibar` is a lightweight, single-header C++17 library for rendering scientific colorbar legends. It is designed to bridge raw scientific data limits and colormaps with clear, publication-ready visual legends—supporting both **in-engine real-time pixel rasterization** and **dependency-free vector export (SVG)**.

---

## 1. Scope & Philosophy

`scibar` follows the **"One-Thing Well"** philosophy for C++ scientific visualization tools (such as [scimesh](https://github.com/dfsp-spirit/scimesh)):

* **What we do:**
  * Calculate layout geometry, ticks, and text positioning.
  * Render legends into RGBA pixel buffers for direct UI/texture overlays.
  * Generate clean, native SVG vector graphics for publication figures (including hybrid raster-mesh embedding).
* **What we do NOT do:**
  * Window management, event handling, or complex charting frameworks.
  * GPU pipeline management (we operate strictly on CPU memory/string output).

---

## 2. Dual-Backend Architecture

To satisfy both real-time C++ engine overlays and journal publication standards, `scibar` decouples layout generation from rendering targets.

```
                         ┌─────────────────────────┐
                         │   scibar::Spec/Style    │
                         └────────────┬────────────┘
                                      │
                                      ▼
                         ┌─────────────────────────┐
                         │  scibar::LayoutEngine   │
                         │ (tick math, bounds,     │
                         │  typography spacing)    │
                         └────────────┬────────────┘
                                      │
           ┌──────────────────────────┴──────────────────────────┐
           ▼                                                     ▼
┌───────────────────────────┐                         ┌───────────────────────────┐
│     Pixel Buffer Target   │                         │     SVG Vector Target     │
│  (stb_truetype + canvas)  │                         │ (String-stream output)    │
│  For real-time viewports  │                         │ For journal manuscripts   │
└───────────────────────────┘                         └───────────────────────────┘
```

### A. Viewport Pixel Rendering (Real-Time Overlays)
Renders directly into a caller-provided packed RGBA pixel buffer (`uint32_t*`). Ideal for 3D engine overlays, HUDs, or frame exports.

### B. SVG Vector Rendering (Publication Exports)
Outputs clean, human-readable SVG XML strings. Supports **hybrid publication figures** where a heavy 3D mesh render (e.g., from `scimesh`) is embedded via an `<image>` tag alongside razor-sharp vector ticks, typography, and color gradients.

---

## 3. Data Structures & Memory Safety

All data structures prioritize strict memory safety, explicitly avoiding dangling references (`std::string_view` lifetime traps) and byte-order ambiguity across platforms.

```cpp
#ifndef SCIBAR_HPP
#define SCIBAR_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <optional>

namespace scibar {

enum class ScaleType { Linear, Logarithmic, Categorical };

// Explicit RGBA structure (eliminates endianness bugs across platforms)
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    static constexpr Color fromHex(uint32_t hex) {
        return { uint8_t(hex >> 24), uint8_t(hex >> 16), uint8_t(hex >> 8), uint8_t(hex & 0xFF) };
    }
};

struct Scale {
    ScaleType type = ScaleType::Linear;
    float min = 0.0f;
    float max = 1.0f;
    float midpoint = 0.0f; // For diverging scales or log shifts
};

struct Font {
    const void* handle = nullptr; // nullptr = use embedded Inter font
    float size = 14.0f;
};

struct Tick {
    float value;
    std::string label; // Owning string avoids dangling view pointers
};

// Data & Domain Specification
struct Spec {
    Scale scale;
    std::vector<Color> colormap; // RGBA lookup table
    std::string title;
    std::vector<Tick> ticks;    // Custom ticks; auto-generated if empty
};

// Visual Presentation & Theme
struct Style {
    bool showFrame = true;
    Color frameColor = Color::fromHex(0x000000FF);
    Color tickColor  = Color::fromHex(0x000000FF);
    Color textColor  = Color::fromHex(0x000000FF);
    Font font;

    static Style defaultLight();
    static Style defaultDark();
};

// Canvas wrapper for pixel rendering
struct Canvas {
    uint32_t* pixels = nullptr;
    int width = 0;
    int height = 0;
};

struct Rect {
    int x, y, width, height;
};

// Options for SVG Export & Hybrid Figure Composition
struct SVGOptions {
    int totalWidth = 800;
    int totalHeight = 600;
    
    // Embedded Main Content Render (e.g., 3D Mesh output)
    std::string mainImageHref = ""; // Local path or "data:image/png;base64,..."
    Rect mainImageBounds = {20, 20, 550, 550};
    
    Rect colorbarBounds = {600, 50, 150, 500};
};

} // namespace scibar

#endif // SCIBAR_HPP
```

---

## 4. API Specification

`scibar` provides both a high-level "smart" API for automated layout and low-level primitive functions for manual placement.

### High-Level API

```cpp
namespace scibar {

// 1. Render legend auto-layout into pixel buffer
void drawLegend(Canvas& canvas, const Spec& spec, const Style& style = Style::defaultLight());

// 2. Export standalone or hybrid vector legend to SVG string
std::string exportToSVG(const Spec& spec, const Style& style = Style::defaultLight(), 
                        const SVGOptions& options = {});

} // namespace scibar
```

### Low-Level Primitives (Manual Layout Control)

```cpp
namespace scibar {

void drawColorBar(Canvas& canvas, Rect bounds, const Spec& spec, const Style& style);
void drawTicks(Canvas& canvas, Rect bounds, const Spec& spec, const Style& style);
void drawTitle(Canvas& canvas, Rect bounds, const std::string& title, const Style& style);

// Utility for manual text positioning
std::array<float, 2> measureText(const std::string& text, const Font& font);

} // namespace scibar
```

---

## 5. Usage Examples

### A. In-Engine Pixel Buffer Rendering (Viewport / Texture)

```cpp
#include "scibar.hpp"

// Setup packed RGBA canvas
uint32_t my_buffer[200 * 600];
scibar::Canvas canvas{my_buffer, 200, 600};

// Define Data
scibar::Spec spec;
spec.scale.min = 0.0f;
spec.scale.max = 100.0f;
spec.title = "Activation (μV)";
spec.colormap = my_viridis_lut; // std::vector<scibar::Color>

scibar::Style style = scibar::Style::defaultDark();

// Draw legend automatically into pixel canvas
scibar::drawLegend(canvas, spec, style);
```

### B. Hybrid Publication Vector Export (SVG)

```cpp
#include "scibar.hpp"
#include <fstream>

scibar::Spec spec;
spec.scale.min = -3.5f;
spec.scale.max = 3.5f;
spec.title = "Z-Score";
spec.colormap = my_coolwarm_lut;

scibar::SVGOptions opts;
opts.totalWidth = 800;
opts.totalHeight = 600;
opts.mainImageHref = "rendered_mesh.png"; // Mesh image exported from 3D engine
opts.mainImageBounds = {20, 20, 550, 550};
opts.colorbarBounds = {600, 50, 120, 500};

// Generate pure vector SVG container with embedded raster content
std::string svg_data = scibar::exportToSVG(spec, scibar::Style::defaultLight(), opts);

std::ofstream out("figure1.svg");
out << svg_data;
```

---

## 6. Font Handling & Embedded Assets

`scibar` uses `stb_truetype.h` for pixel font rasterization.

* **Default Embedded Font:** Includes **Inter-Regular.ttf** as an embedded byte array. Inter is an open-source, journal-approved typeface optimized for legibility in data visualizations.
* **Custom Fonts:** Users can override the default font by supplying a pointer to their own loaded `stbtt_fontinfo` handle in `Style::font`.
* **SVG Vector Text:** In SVG exports, text elements use standard CSS font family declarations (`font-family="Inter, sans-serif"`), ensuring crisp text rendering in browsers, Adobe Illustrator, Inkscape, and PDF renderers.

---

## 7. Single-Header Integration & Third-Party Dependencies

`scibar` is distributed as a single-header file (`scibar.hpp`). Implementation is enabled in exactly one translation unit using the standard macro definition:

```cpp
#define SCIBAR_IMPLEMENTATION
#include "scibar.hpp"
```

### Vendored Third-Party Dependencies (`src/third_party/`)

* **[canvas-ity](https://github.com/a-e-k/canvas_ity):** Immediate-mode 2D rasterization library (ISC License).
* **[stb_truetype.h](https://github.com/nothings/stb):** Font loading and glyph rasterization (Public Domain / MIT).
* **[Catch2](https://github.com/catchorg/Catch2):** Unit testing framework, development only (BSL-1.0 License).
