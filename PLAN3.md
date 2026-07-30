# scibar: Architecture & Specification Plan

**Single-Header Scientific Colorbar Rendering for C++17**

`scibar` is a lightweight, single-header C++17 library for rendering scientific colorbar legends. It is designed to bridge raw scientific data limits and colormaps with clear, publication-ready visual legends—supporting both **in-engine real-time pixel rasterization** and **dependency-free vector export (SVG)**.

---

## 1. Scope & Philosophy

`scibar` follows the **"One-Thing Well"** philosophy for C++ scientific visualization tools (such as [scimesh](https://github.com/dfsp-spirit/scimesh)):

* **What we do:**
  * Render colorbar gradients, tick marks, and text labels into RGBA pixel buffers for direct UI/texture overlays.
  * Generate clean, native SVG vector graphics for publication figures (including hybrid raster-mesh embedding).
  * Generate sensible tick values from a data range via a nice-numbers algorithm.
  * Measure text dimensions for manual layout calculations.
* **What we do NOT do:**
  * Window management, event handling, or complex charting frameworks.
  * GPU pipeline management (we operate strictly on CPU memory/string output).

### What scibar is NOT

**scibar is not a layout engine.** It does not solve spatial placement problems — no constraint solving, no collision detection, no multi-pass refinement, no automatic margin/padding negotiation. The user always controls where every element goes by passing explicit `Rect` bounds to the low-level API.

**scibar is not a general-purpose 2D drawing library.** The pixel backend exists solely to render colorbar components. If you need to draw arbitrary shapes, paths, or UI widgets, use a dedicated library like `canvas_ity` directly.

**The high-level `drawLegend()` is not a typesetting system.** It applies rough, hardcoded defaults for a vertical colorbar filling the entire canvas — useful for rapid prototyping and real-time debug overlays where precise layout is irrelevant. For anything that will appear in a publication, use the low-level API.

### Error Handling Policy

scibar uses **assertions** for invalid input (e.g., logarithmic scale with `min <= 0`, null canvas pixels, zero-dimension `Rect`). This follows the philosophy that scibar is a low-level building block: the caller is responsible for providing valid data. No exceptions, no silent clamping, no error codes — just fail fast and loud during development. In release builds (`-DNDEBUG`), behavior is undefined for invalid input.

---

## 2. Dual-Backend Architecture

`scibar` provides two independent rendering backends that share a common data model (`Spec`/`Style`). Both backends are driven by explicit user-provided layout coordinates — there is no layout engine between them.

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

**Helper utilities** available to both backends (not an architectural layer — just functions):
- `generateTicks()`: Nice-numbers algorithm to produce sensible tick values from a data range.
- `measureText()`: Returns pixel dimensions of a text string for the given font.

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

// Font vertical metrics and glyph-level measurements.
// Exposed so users can build custom layout on top of scibar
// (e.g., superscript placement, cursor tracking, kerning-aware character spacing).
struct FontMetrics {
    float ascender;     // Distance from baseline to top of tallest glyph
    float descender;    // Distance from baseline to bottom of lowest glyph (negative)
    float lineHeight;   // Recommended line spacing (ascender - descender + internal leading)
};

struct Tick {
    float value;
    std::string label; // Owning string avoids dangling view pointers
};

// Non-owning colormap view — avoids heap allocation per frame in real-time loops.
// Implicitly constructible from std::vector<Color> for ergonomic one-shot usage.
// Rvalue vector constructor is deleted to prevent dangling from temporaries.
struct ColorMapView {
    const Color* data = nullptr;
    size_t size = 0;

    ColorMapView() = default;
    ColorMapView(const std::vector<Color>& v) : data(v.data()), size(v.size()) {}
    ColorMapView(std::vector<Color>&&) = delete;
    ColorMapView(const Color* d, size_t s) : data(d), size(s) {}
};

// Data & Domain Specification
struct Spec {
    Scale scale;
    ColorMapView colormap;  // Non-owning RGBA lookup table
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

    float tickLength = 5.0f;        // Outward tick mark length in pixels
    int tickPrecision = 6;          // Significant digits for auto-generated tick labels (%.*g)

    static Style defaultLight();
    static Style defaultDark();
};

// Canvas wrapper for pixel rendering.
// pixels[] is a packed uint32_t array in RGBA byte order:
//   byte 0 (LSB) = R, byte 1 = G, byte 2 = B, byte 3 (MSB) = A.
// This matches the layout of Color::fromHex() and common RGBA8888 framebuffers.
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

`scibar` provides low-level primitive functions for fine-grained manual control, plus a single high-level convenience function for rapid prototyping.

### Low-Level Primitives (Primary API)

These are the core of `scibar`. Every element is placed at explicit `Rect` coordinates chosen by the user. This is the intended API for publication-quality output and any use case where precise spatial control matters.

```cpp
namespace scibar {

// Each draw function returns the actual bounding box of everything it rendered.
// This may extend beyond the input Rect (e.g., tick marks and labels protrude
// outward from the bar edge). The caller can union these to compute total extent.
Rect drawColorBar(Canvas& canvas, Rect bounds, const Spec& spec, const Style& style);
Rect drawTicks(Canvas& canvas, Rect barBounds, const Spec& spec, const Style& style);
Rect drawTitle(Canvas& canvas, Rect bounds, const std::string& title, const Style& style);

// Utility: measure text dimensions for manual layout calculations
std::array<float, 2> measureText(const std::string& text, const Font& font);

// Font metrics: vertical dimensions and per-glyph advance widths.
// These are the primitives a real layout engine would call — scibar does not
// do layout itself, but exposes these so users can build their own on top.
FontMetrics fontMetrics(const Font& font);
float textAdvance(const Font& font, const std::string& text, int upToIndex);
float codepointAdvance(const Font& font, int leftCodepoint, int rightCodepoint);

// Utility: generate sensible tick values via nice-numbers algorithm.
// Labels are formatted with Style::tickPrecision via printf("%.*g").
std::vector<Tick> generateTicks(const Scale& scale, int targetCount = 5, int precision = 6);

// Font loading: parse a .ttf file from disk for development and testing.
// Returns a Font whose handle points to scibar-managed internal storage.
// Caller does not own the returned resources — the font data persists for
// the lifetime of the process. In production, omit this call and rely on
// the embedded Inter font (Font::handle == nullptr).
Font loadFont(const char* ttfFilePath, float size = 14.0f);

} // namespace scibar
```

### High-Level Convenience API (Rapid Prototyping Only)

`drawLegend()` applies hardcoded defaults for a vertical colorbar filling the entire canvas, with ticks on the right and title centered above. It performs no constraint solving, collision detection, or multi-pass refinement. **Use it for quick iteration and real-time debug overlays — not for publication figures.**

```cpp
namespace scibar {

// Render a vertical colorbar with reasonable defaults into a pixel buffer.
// Returns LayoutResult with the actual bounding boxes used.
LayoutResult drawLegend(Canvas& canvas, const Spec& spec,
                        const Style& style = Style::defaultLight());

// Export standalone or hybrid vector legend to SVG string.
// Uses bounds from SVGOptions — no automatic layout.
std::string exportToSVG(const Spec& spec, const Style& style = Style::defaultLight(),
                        const SVGOptions& options = {});

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

// Generate pure vector SVG container with embedded raster content.
// Embedded <image> tags use image-rendering="crisp-edges" to prevent
// bilinear blurring of scientific mesh data in browsers and vector editors.
std::string svg_data = scibar::exportToSVG(spec, scibar::Style::defaultLight(), opts);

std::ofstream out("figure1.svg");
out << svg_data;
```

---

## 6. Font Handling & Embedded Assets

`scibar` uses `stb_truetype.h` for pixel font rasterization and supports three tiers of font usage:

### Tier 1 — Disk-loaded (Development & Testing)

```cpp
scibar::Font font = scibar::loadFont("fonts/Inter-Regular.ttf", 14.0f);
style.font = font;
```

`loadFont()` reads the .ttf file at startup, parses it, and retains it in internal static storage for the lifetime of the process. No cleanup required by the caller. This is the intended workflow during development — no 300KB header to scroll past.

### Tier 2 — Embedded (Production, planned for later)

When `Font::handle == nullptr`, scibar uses an embedded copy of Inter-Regular.ttf compiled directly into the library binary as a static byte array under `#ifdef SCIBAR_IMPLEMENTATION`. Zero file I/O, zero dependencies, single-header drop-in.

This will be added after the core implementation stabilizes.

### Tier 3 — Custom (Advanced)

Users can override the default font entirely by supplying a pointer to their own loaded `stbtt_fontinfo` handle in `Style::font`.

* **SVG Vector Text:** In SVG exports, text elements use standard CSS font family declarations (`font-family="Inter, sans-serif"`), ensuring crisp text rendering in browsers, Adobe Illustrator, Inkscape, and PDF renderers.
* **Glyph-Level Metrics:** scibar exposes `fontMetrics()`, `textAdvance()`, and `codepointAdvance()` so users can perform per-glyph layout (superscripts, subscripts, custom cursor positioning, kerning-aware spacing) without reaching into stb_truetype internals directly. These are backend-agnostic — they return plain floats, not stb-specific types — keeping user code portable between pixel and SVG backends. scibar does NOT expose glyph bitmaps, glyph IDs, shaping, or line-breaking; those remain the domain of dedicated libraries.

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


## Additional details

More things to keep in mind:

* `drawLegend()` returns a `LayoutResult` so callers know where elements ended up:

```cpp
struct LayoutResult {
    Rect totalBoundingBox;    // Outer bounds including title and labels
    Rect colorbarBoundingBox; // Bounds of just the gradient bar
    int generatedTickCount;
};
```

* specify exactly the text alignment relative to ticks for vertical vs horizontal colorbars. Scientific colorbars are almost universally vertical on the side of a mesh, but occasionally horizontal at the bottom of a 2D plot. Define tick label anchor geometry (`text-anchor` in SVG, alignment bounding box in stb_truetype) via an `Orientation` enum:
  - **Vertical Bar (Right Ticks):** Left-aligned text (`text-anchor="start"`), vertically centered on tick mark `Y`.
  - **Horizontal Bar (Bottom Ticks):** Horizontally centered text (`text-anchor="middle"`), top-aligned below tick mark `X`.

```cpp
enum class Orientation { Vertical, Horizontal };
```

* specify SVG drawing method for colorbars of different types, to avoid render artefacts. In SVG export (`exportToSVG`), continuous colormaps are rendered via `<linearGradient>`. However, PDF/SVG vector engines often create subtle anti-aliasing seams (faint white or grey lines) when rendering adjacent shapes or continuous gradient stops across sharp binned thresholds:
  - **Continuous Scales** (Linear, Log): Render as a single SVG `<rect>` filled with a `<linearGradient>` containing discrete `<stop>` elements.
  - **Discrete Scales** (Binned, Categorical): Render as explicit individual `<rect>` elements stacked along the bar.

* **Tick direction:** Outward only (ticks extend away from the bar into user-managed space). No minor ticks in v1.

* **Tick label formatting:** `generateTicks()` uses `printf("%.*g", precision, value)` with the precision from `Style::tickPrecision` (default 6). Labels are owned strings in the `Tick` struct — no lifetime dependency.

* **Spatial contract:** Each draw function renders exactly within its input `Rect` *except* ticks and labels, which protrude outward. The returned `Rect` reports the full extent including protrusions. The user is responsible for leaving enough canvas space — scibar never clips, pads, or repositions.

* **Discrete / categorical colorbar seams:** When rendering binned or categorical scales as adjacent blocks, anti-aliased rasterizers may leave faint 1px gaps between blocks. In the pixel backend, snap block boundaries to integer pixel coordinates and overlap adjacent blocks by 0.5–1 px to prevent seams. In SVG, use `shape-rendering="crispEdges"` on each discrete `<rect>`.

* **Reversed colorbar:** Not supported in v1. Users can reverse their `ColorMapView` data before passing it in. A built-in flag may be added later.

* provide some convenience overloads for functions, like a `drawLegend()` version that does not require the third `style` parameter, and defaults to bright mode if it is not given.