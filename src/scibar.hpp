// scibar — Single-Header Scientific Colorbar Rendering for C++17
// See PLAN3.md for architecture and design rationale.
//
// Usage:
//   In exactly one .cpp file:
//     #define SCIBAR_IMPLEMENTATION
//     #include "scibar.hpp"
//   Everywhere else:
//     #include "scibar.hpp"
//
// Dependencies (vendored in src/third_party/):
//   - canvas_ity.hpp  (ISC license) — 2D rasterization
//   - stb_truetype.h  (Public Domain) — font metrics & glyph rasterization

#ifndef SCIBAR_HPP
#define SCIBAR_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <optional>

namespace scibar {

// =========================================================================
// Enums
// =========================================================================

enum class ScaleType { Linear, Logarithmic, Categorical };

enum class Orientation { Vertical, Horizontal };

// =========================================================================
// Data Structures
// =========================================================================

// Explicit RGBA structure — eliminates endianness bugs across platforms.
// byte 0 (LSB) = R, byte 1 = G, byte 2 = B, byte 3 (MSB) = A.
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    static constexpr Color fromHex(uint32_t hex) {
        return { uint8_t(hex >> 24), uint8_t(hex >> 16),
                 uint8_t(hex >> 8),  uint8_t(hex & 0xFF) };
    }
};

struct Scale {
    ScaleType type = ScaleType::Linear;
    float min = 0.0f;
    float max = 1.0f;
    float midpoint = 0.0f; // For diverging scales or log shifts
};

struct Font {
    const void* handle = nullptr; // nullptr = use embedded Inter font (Tier 2, future)
    float size = 14.0f;
};

struct FontMetrics {
    float ascender   = 0.0f; // Distance from baseline to top of tallest glyph
    float descender  = 0.0f; // Distance from baseline to bottom (negative)
    float lineHeight = 0.0f; // Recommended line spacing
};

struct Tick {
    float value = 0.0f;
    std::string label;
};

// Non-owning colormap view — avoids heap allocation per frame.
// Implicitly constructible from std::vector<Color> for ergonomic usage.
// Rvalue vector constructor deleted to prevent dangling from temporaries.
struct ColorMapView {
    const Color* data = nullptr;
    size_t size = 0;

    ColorMapView() = default;
    ColorMapView(const std::vector<Color>& v) : data(v.data()), size(v.size()) {}
    ColorMapView(std::vector<Color>&&) = delete;
    ColorMapView(const Color* d, size_t s) : data(d), size(s) {}
};

struct Spec {
    Scale scale;
    ColorMapView colormap;
    std::string title;
    std::vector<Tick> ticks; // Custom ticks; auto-generated via generateTicks() if empty
};

struct Style {
    bool  showFrame     = true;
    Color frameColor    = Color::fromHex(0x000000FF);
    Color tickColor     = Color::fromHex(0x000000FF);
    Color textColor     = Color::fromHex(0x000000FF);
    Font  font;

    float tickLength    = 5.0f;  // Outward tick mark length in pixels
    int   tickPrecision = 6;     // Significant digits for auto-generated tick labels (%.*g)

    static Style defaultLight();
    static Style defaultDark();
};

struct Canvas {
    // pixels[] is packed uint32_t in RGBA byte order:
    // byte 0 (LSB) = R, byte 1 = G, byte 2 = B, byte 3 (MSB) = A.
    uint32_t* pixels = nullptr;
    int width  = 0;
    int height = 0;
};

struct Rect {
    int x = 0, y = 0, width = 0, height = 0;
};

inline Rect unionRect(const Rect& a, const Rect& b) {
    if (a.width <= 0 || a.height <= 0) return b;
    if (b.width <= 0 || b.height <= 0) return a;
    int xMin = a.x < b.x ? a.x : b.x;
    int yMin = a.y < b.y ? a.y : b.y;
    int xMax = (a.x + a.width)  > (b.x + b.width)  ? (a.x + a.width)  : (b.x + b.width);
    int yMax = (a.y + a.height) > (b.y + b.height) ? (a.y + a.height) : (b.y + b.height);
    return { xMin, yMin, xMax - xMin, yMax - yMin };
}

struct LayoutResult {
    Rect totalBoundingBox;
    Rect colorbarBoundingBox;
    int  generatedTickCount = 0;
};

struct SVGOptions {
    int totalWidth  = 800;
    int totalHeight = 600;

    std::string mainImageHref = ""; // Local path or "data:image/png;base64,..."
    Rect mainImageBounds = {20, 20, 550, 550};
    Rect colorbarBounds   = {600, 50, 150, 500};
};

// =========================================================================
// API Declarations
// =========================================================================

// --- Font loading ---
Font loadFont(const char* ttfFilePath, float size = 14.0f);

// --- Text measurement ---
std::array<float, 2> measureText(const std::string& text, const Font& font);
FontMetrics          fontMetrics(const Font& font);
float textAdvance(const Font& font, const std::string& text, int upToIndex);
float codepointAdvance(const Font& font, int leftCodepoint, int rightCodepoint);

// --- Tick generation ---
std::vector<Tick> generateTicks(const Scale& scale, int targetCount = 5, int precision = 6);

// --- Low-level drawing (pixel backend) ---
Rect drawColorBar(Canvas& canvas, Rect bounds, const Spec& spec, const Style& style);
Rect drawTicks(Canvas& canvas, Rect barBounds, const Spec& spec, const Style& style);
Rect drawTitle(Canvas& canvas, Rect bounds, const std::string& title, const Style& style);

// --- High-level convenience ---
LayoutResult drawLegend(Canvas& canvas, const Spec& spec,
                        const Style& style = Style::defaultLight());

// --- SVG export ---
std::string exportToSVG(const Spec& spec,
                        const Style& style = Style::defaultLight(),
                        const SVGOptions& options = {});

} // namespace scibar

#endif // SCIBAR_HPP
