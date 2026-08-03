#include "catch_amalgamated.hpp"
#define SCIBAR_IMPLEMENTATION
#include "scibar/scibar.hpp"

#include <cmath>
#include <filesystem>
#include <string>
#include <sstream>
#include <fstream>
#include <algorithm>

// =========================================================================
// Pixel helpers
// =========================================================================

// Canvas pixel format: packed uint32_t RGBA — byte 0 (LSB) = R, byte 1 = G, byte 2 = B, byte 3 (MSB) = A.
static constexpr uint32_t WHITE_PIXEL = 0xFFFFFFFF;

static bool isDarkPixel(uint32_t px, int threshold = 200) {
    uint8_t r = px & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = (px >> 16) & 0xFF;
    return r < threshold && g < threshold && b < threshold;
}

static bool isLightPixel(uint32_t px, int threshold = 128) {
    uint8_t r = px & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = (px >> 16) & 0xFF;
    return r > threshold && g > threshold && b > threshold;
}

static bool isNotWhite(uint32_t px) {
    return px != WHITE_PIXEL;
}

static bool pixelMatchesColor(uint32_t px, scibar::Color expected, int tolerance = 40) {
    uint8_t r = px & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = (px >> 16) & 0xFF;
    return std::abs(static_cast<int>(r) - static_cast<int>(expected.r)) <= tolerance &&
           std::abs(static_cast<int>(g) - static_cast<int>(expected.g)) <= tolerance &&
           std::abs(static_cast<int>(b) - static_cast<int>(expected.b)) <= tolerance;
}

// =========================================================================
// SVG helpers
// =========================================================================

/// Extract all <line> attributes from an SVG string.
/// Returns vector of {x1, y1, x2, y2} for each line element.
static std::vector<std::array<float, 4>> extractSvgLines(const std::string& svg) {
    std::vector<std::array<float, 4>> lines;
    size_t pos = 0;
    while (true) {
        pos = svg.find("<line ", pos);  // space avoids matching <linearGradient>
        if (pos == std::string::npos) break;

        float vals[4] = {};
        const char* attrs[] = {"x1=\"", "y1=\"", "x2=\"", "y2=\""};
        size_t searchPos = pos;
        for (int i = 0; i < 4; ++i) {
            size_t attrPos = svg.find(attrs[i], searchPos);
            if (attrPos != std::string::npos) {
                vals[i] = std::stof(svg.substr(attrPos + strlen(attrs[i])));
            }
        }
        lines.push_back({vals[0], vals[1], vals[2], vals[3]});
        pos++;
    }
    return lines;
}

/// Extract all <text> element x attributes from an SVG string.
static std::vector<float> extractSvgTextX(const std::string& svg) {
    std::vector<float> xs;
    size_t pos = 0;
    while (true) {
        pos = svg.find("<text ", pos);  // space avoids matching other elements
        if (pos == std::string::npos) break;

        size_t xPos = svg.find("x=\"", pos);
        if (xPos != std::string::npos) {
            xs.push_back(std::stof(svg.substr(xPos + 3)));
        }
        pos++;
    }
    return xs;
}

/// Extract all <text> element {x, y} positions from an SVG string.
static std::vector<std::array<float, 2>> extractSvgTextPositions(const std::string& svg) {
    std::vector<std::array<float, 2>> positions;
    size_t pos = 0;
    while (true) {
        pos = svg.find("<text ", pos);
        if (pos == std::string::npos) break;

        float x = 0.0f, y = 0.0f;
        size_t xPos = svg.find("x=\"", pos);
        if (xPos != std::string::npos) {
            x = std::stof(svg.substr(xPos + 3));
        }
        size_t yPos = svg.find("y=\"", pos);
        if (yPos != std::string::npos) {
            y = std::stof(svg.substr(yPos + 3));
        }
        positions.push_back({x, y});
        pos++;
    }
    return positions;
}

/// Extract the title <text> element position {x, y} from an SVG string.
/// Searches for a <text> element containing the exact label content string.
static std::array<float, 2> extractSvgLabelPosition(const std::string& svg,
                                                      const std::string& titleText) {
    // Find the label text content in the SVG
    size_t contentPos = svg.find(">" + titleText + "<");
    if (contentPos == std::string::npos) return {0.0f, 0.0f};

    // Search backwards for the nearest <text tag
    size_t textStart = svg.rfind("<text ", contentPos);
    if (textStart == std::string::npos || textStart > contentPos) return {0.0f, 0.0f};

    float x = 0.0f, y = 0.0f;
    size_t xPos = svg.find("x=\"", textStart);
    if (xPos != std::string::npos && xPos < contentPos) {
        x = std::stof(svg.substr(xPos + 3));
    }
    size_t yPos = svg.find("y=\"", textStart);
    if (yPos != std::string::npos && yPos < contentPos) {
        y = std::stof(svg.substr(yPos + 3));
    }
    return {x, y};
}

/// Extract Y positions of major tick labels (text elements with a numeric label).
/// Returns {y_position} for each tick label, excluding the label.
static std::vector<float> extractSvgTickLabelY(const std::string& svg,
                                                const std::string& titleText) {
    std::vector<float> ys;
    size_t pos = 0;
    while (true) {
        pos = svg.find("<text ", pos);
        if (pos == std::string::npos) break;

        // Skip the label element
        size_t contentEnd = svg.find("</text>", pos);
        if (contentEnd == std::string::npos) break;

        std::string elementText = svg.substr(pos, contentEnd - pos);
        if (elementText.find(titleText) != std::string::npos) {
            pos = contentEnd + 7;
            continue;
        }

        // Extract y attribute
        size_t yPos = svg.find("y=\"", pos);
        if (yPos != std::string::npos && yPos < contentEnd) {
            ys.push_back(std::stof(svg.substr(yPos + 3)));
        }
        pos = contentEnd + 7;
    }
    return ys;
}

/// Extract X positions of major tick labels (for horizontal bars).
static std::vector<float> extractSvgTickLabelX(const std::string& svg,
                                                const std::string& titleText) {
    std::vector<float> xs;
    size_t pos = 0;
    while (true) {
        pos = svg.find("<text ", pos);
        if (pos == std::string::npos) break;

        size_t contentEnd = svg.find("</text>", pos);
        if (contentEnd == std::string::npos) break;

        std::string elementText = svg.substr(pos, contentEnd - pos);
        if (elementText.find(titleText) != std::string::npos) {
            pos = contentEnd + 7;
            continue;
        }

        size_t xPos = svg.find("x=\"", pos);
        if (xPos != std::string::npos && xPos < contentEnd) {
            xs.push_back(std::stof(svg.substr(xPos + 3)));
        }
        pos = contentEnd + 7;
    }
    return xs;
}

/// Extract the colorbar gradient <rect> position {x, y, w, h} from an SVG string.
static std::array<float, 4> extractSvgBarRect(const std::string& svg) {
    // The gradient rect uses fill="url(#scibarGrad)"
    size_t pos = svg.find("url(#scibarGrad)");
    if (pos == std::string::npos) return {0.0f, 0.0f, 0.0f, 0.0f};

    size_t rectStart = svg.rfind("<rect ", pos);
    if (rectStart == std::string::npos || rectStart > pos) return {0.0f, 0.0f, 0.0f, 0.0f};

    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    size_t xPos = svg.find("x=\"", rectStart);
    if (xPos != std::string::npos && xPos < pos) x = std::stof(svg.substr(xPos + 3));
    size_t yPos = svg.find("y=\"", rectStart);
    if (yPos != std::string::npos && yPos < pos) y = std::stof(svg.substr(yPos + 3));
    size_t wPos = svg.find("width=\"", rectStart);
    if (wPos != std::string::npos && wPos < pos) w = std::stof(svg.substr(wPos + 7));
    size_t hPos = svg.find("height=\"", rectStart);
    if (hPos != std::string::npos && hPos < pos) h = std::stof(svg.substr(hPos + 8));
    return {x, y, w, h};
}

/// Scan the raster canvas to find the Y position where the label text appears
/// (first row from top with non-background pixels, within the center third).
static int findTitleTopY(const scibar::Canvas& canvas, int bgThreshold = 250) {
    int centerStart = canvas.width / 3;
    int centerEnd   = centerStart * 2;
    for (int y = 0; y < canvas.height / 3; ++y) {
        for (int x = centerStart; x < centerEnd; ++x) {
            uint32_t px = canvas.pixels[y * canvas.width + x];
            uint8_t r = px & 0xFF, g = (px >> 8) & 0xFF, b = (px >> 16) & 0xFF;
            if (r < bgThreshold || g < bgThreshold || b < bgThreshold)
                return y;
        }
    }
    return -1;
}

/// Find the leftmost non-background pixel in a row (finds the left edge of the colorbar).
static int findBarLeftX(const scibar::Canvas& canvas, int rowY, int bgThreshold = 250) {
    for (int x = 0; x < canvas.width; ++x) {
        uint32_t px = canvas.pixels[rowY * canvas.width + x];
        uint8_t r = px & 0xFF, g = (px >> 8) & 0xFF, b = (px >> 16) & 0xFF;
        if (r < bgThreshold || g < bgThreshold || b < bgThreshold)
            return x;
    }
    return -1;
}

/// Find the rightmost non-background pixel in a row (right edge of the colorbar).
static int findBarRightX(const scibar::Canvas& canvas, int rowY, int bgThreshold = 250) {
    for (int x = canvas.width - 1; x >= 0; --x) {
        uint32_t px = canvas.pixels[rowY * canvas.width + x];
        uint8_t r = px & 0xFF, g = (px >> 8) & 0xFF, b = (px >> 16) & 0xFF;
        if (r < bgThreshold || g < bgThreshold || b < bgThreshold)
            return x;
    }
    return -1;
}

TEST_CASE("Color fromHex", "[data]") {
    auto c = scibar::Color::fromHex(0x12345678);
    REQUIRE(c.r == 0x12);
    REQUIRE(c.g == 0x34);
    REQUIRE(c.b == 0x56);
    REQUIRE(c.a == 0x78);
}

TEST_CASE("Color fromFloat and asFloat roundtrip", "[data]") {
    // Roundtrip: float → uint8_t → float should be lossless at endpoints.
    {
        auto c = scibar::Color::fromFloat(0.0f, 0.5f, 1.0f, 1.0f);
        REQUIRE(c.r == 0);
        REQUIRE(c.g == 128);
        REQUIRE(c.b == 255);
        REQUIRE(c.a == 255);

        auto cf = c.asFloat();
        REQUIRE(cf.r == Catch::Approx(0.0f).margin(0.005f));
        REQUIRE(cf.g == Catch::Approx(0.5f).margin(0.005f));
        REQUIRE(cf.b == Catch::Approx(1.0f).margin(0.005f));
        REQUIRE(cf.a == Catch::Approx(1.0f).margin(0.005f));
    }

    // Identity roundtrip for all 256 gray levels.
    for (int i = 0; i <= 255; ++i) {
        float v = i / 255.0f;
        auto c  = scibar::Color::fromFloat(v, v, v);
        auto cf = c.asFloat();
        REQUIRE(c.r == i);
        REQUIRE(cf.r == Catch::Approx(v).margin(0.005f));
    }

    // Default alpha.
    {
        auto c = scibar::Color::fromFloat(0.2f, 0.4f, 0.6f);
        REQUIRE(c.a == 255); // default opaque
    }

    // ColorF default values.
    {
        scibar::ColorF cf;
        REQUIRE(cf.r == 0.0f);
        REQUIRE(cf.g == 0.0f);
        REQUIRE(cf.b == 0.0f);
        REQUIRE(cf.a == 1.0f);
    }
}

TEST_CASE("ColorMapView from vector", "[data]") {
    std::vector<scibar::Color> colors = {
        scibar::Color{255, 0, 0, 255},
        scibar::Color{0, 255, 0, 255},
    };

    scibar::ColorMapView view(colors);
    REQUIRE(view.size == 2);
    REQUIRE(view.data[0].r == 255);
    REQUIRE(view.data[1].g == 255);
}

TEST_CASE("unionRect", "[data]") {
    scibar::Rect a{0, 0, 10, 10};
    scibar::Rect b{5, 5, 10, 10};
    auto u = scibar::unionRect(a, b);
    REQUIRE(u.x == 0);
    REQUIRE(u.y == 0);
    REQUIRE(u.width == 15);
    REQUIRE(u.height == 15);
}

TEST_CASE("Style defaults", "[style]") {
    auto light = scibar::Style::defaultLight();
    REQUIRE(light.frameColor.r == 0x00);
    REQUIRE(light.frameColor.g == 0x00);
    REQUIRE(light.frameColor.b == 0x00);

    auto dark = scibar::Style::defaultDark();
    REQUIRE(dark.frameColor.r == 0xFF);
    REQUIRE(dark.frameColor.g == 0xFF);
    REQUIRE(dark.frameColor.b == 0xFF);
}

TEST_CASE("embedded font metrics", "[font]") {
    scibar::Font font;  // uses embedded Inter font
    REQUIRE(font.handle == nullptr);
    REQUIRE(font.size == 14.0f);

    auto metrics = scibar::fontMetrics(font);
    REQUIRE(metrics.ascender > 0.0f);
    REQUIRE(metrics.descender < 0.0f);
    REQUIRE(metrics.lineHeight > 0.0f);
    REQUIRE(metrics.lineHeight > metrics.ascender);
}

TEST_CASE("measureText", "[font]") {
    scibar::Font font;  // uses embedded Inter font

    auto dims = scibar::measureText("Hello", font);
    REQUIRE(dims[0] > 0.0f);  // width
    REQUIRE(dims[1] > 0.0f);  // height
}

TEST_CASE("textAdvance", "[font]") {
    scibar::Font font;  // uses embedded Inter font

    // Advance up to "He" should be roughly half of "Hello"
    float advance2  = scibar::textAdvance(font, "Hello", 2);
    float advance5  = scibar::textAdvance(font, "Hello", 5);
    float fullWidth = scibar::measureText("Hello", font)[0];

    REQUIRE(advance2 > 0.0f);
    REQUIRE(advance2 < advance5);
    REQUIRE(advance5 == Catch::Approx(fullWidth).margin(0.5f));
}

TEST_CASE("codepointAdvance", "[font]") {
    scibar::Font font;  // uses embedded Inter font

    float adv = scibar::codepointAdvance(font, 'A', 'V');
    REQUIRE(adv > 0.0f);
}

TEST_CASE("generateTicks linear", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min  = 0.0f;
    scale.max  = 10.0f;

    auto ticks = scibar::generateTicks(scale, 6, 2);
    REQUIRE(ticks.size() >= 4);
    REQUIRE(ticks.size() <= 8);

    // First tick should be >= min
    REQUIRE(ticks.front().value >= scale.min);
    // Last tick should be <= max
    REQUIRE(ticks.back().value <= scale.max);

    // Tick values should be monotonically increasing
    for (size_t i = 1; i < ticks.size(); ++i) {
        REQUIRE(ticks[i].value > ticks[i - 1].value);
    }

    // Labels should not be empty
    for (const auto& t : ticks) {
        REQUIRE_FALSE(t.label.empty());
    }
}

TEST_CASE("generateTicks linear — all negative range", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min  = -4.0f;
    scale.max  = -2.0f;

    auto ticks = scibar::generateTicks(scale, 5, 2);
    REQUIRE(ticks.size() >= 3);

    // Ticks should be within the data range: first tick >= min, last tick <= max
    REQUIRE(ticks.front().value >= scale.min);
    REQUIRE(ticks.back().value <= scale.max);

    // All ticks should be negative (since the data range is entirely negative)
    for (const auto& t : ticks) {
        REQUIRE(t.value <= 0.0f);
        REQUIRE_FALSE(t.label.empty());
    }

    // Tick values should be monotonically increasing
    for (size_t i = 1; i < ticks.size(); ++i) {
        REQUIRE(ticks[i].value > ticks[i - 1].value);
    }
}

TEST_CASE("generateTicks linear — range crossing zero", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min  = -5.0f;
    scale.max  = 7.0f;

    auto ticks = scibar::generateTicks(scale, 5, 2);
    REQUIRE(ticks.size() >= 3);

    // Ticks should be within the data range
    REQUIRE(ticks.front().value >= scale.min);
    REQUIRE(ticks.back().value <= scale.max);

    // Should include at least one negative tick and one positive tick
    bool hasNegative = false;
    bool hasPositive = false;
    for (const auto& t : ticks) {
        if (t.value < 0.0f) hasNegative = true;
        if (t.value > 0.0f) hasPositive = true;
        REQUIRE_FALSE(t.label.empty());
    }
    REQUIRE(hasNegative);
    REQUIRE(hasPositive);

    // Tick values should be monotonically increasing
    for (size_t i = 1; i < ticks.size(); ++i) {
        REQUIRE(ticks[i].value > ticks[i - 1].value);
    }
}

TEST_CASE("generateTicks linear — all positive range", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min  = 5.0f;
    scale.max  = 11.0f;

    auto ticks = scibar::generateTicks(scale, 5, 2);
    REQUIRE(ticks.size() >= 3);

    // Ticks should be within the data range
    REQUIRE(ticks.front().value >= scale.min);
    REQUIRE(ticks.back().value <= scale.max);

    // All ticks should be positive
    for (const auto& t : ticks) {
        REQUIRE(t.value >= 0.0f);
        REQUIRE_FALSE(t.label.empty());
    }

    // Tick values should be monotonically increasing
    for (size_t i = 1; i < ticks.size(); ++i) {
        REQUIRE(ticks[i].value > ticks[i - 1].value);
    }
}

TEST_CASE("generateTicks linear — wide asymmetric range (real-world)", "[ticks]") {
    // Real-world case from the brain sulcal depth example:
    // range [-9.37742, 11.8088] with viridis colormap.
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min  = -9.37742f;
    scale.max  = 11.8088f;

    auto ticks = scibar::generateTicks(scale, 5, 2);
    REQUIRE(ticks.size() >= 3);

    // Ticks should be within the data range
    REQUIRE(ticks.front().value >= scale.min);
    REQUIRE(ticks.back().value <= scale.max);

    // Must include negative ticks (the ceil-based start with improved
    // niceStep must not skip negative territory)
    bool hasNegative = false;
    for (const auto& t : ticks) {
        if (t.value < 0.0f) hasNegative = true;
        REQUIRE_FALSE(t.label.empty());
    }
    REQUIRE(hasNegative);

    // Tick values should be monotonically increasing
    for (size_t i = 1; i < ticks.size(); ++i) {
        REQUIRE(ticks[i].value > ticks[i - 1].value);
    }
}

TEST_CASE("generateTicks logarithmic", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Logarithmic;
    scale.min  = 1.0f;
    scale.max  = 1000.0f;

    auto ticks = scibar::generateTicks(scale, 5, 3);
    REQUIRE(ticks.size() >= 2);

    // Should include powers of 10: 1, 10, 100, 1000
    bool has1    = false;
    bool has10   = false;
    bool has100  = false;
    bool has1000 = false;
    for (const auto& t : ticks) {
        if (std::abs(t.value - 1.0f)    < 0.01f) has1    = true;
        if (std::abs(t.value - 10.0f)   < 0.01f) has10   = true;
        if (std::abs(t.value - 100.0f)  < 0.01f) has100  = true;
        if (std::abs(t.value - 1000.0f) < 0.01f) has1000 = true;
        REQUIRE_FALSE(t.label.empty());
    }
    REQUIRE(has1);
    REQUIRE(has10);
    REQUIRE(has100);
    REQUIRE(has1000);
}

// =========================================================================
// Sub-tick tests
// =========================================================================

TEST_CASE("generateSubTicks linear — uniform spacing", "[subticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min  = 0.0f;
    scale.max  = 100.0f;

    auto majors = scibar::generateTicks(scale, 5, 2);
    REQUIRE(majors.size() >= 3);

    auto subs = scibar::generateSubTicks(scale, majors, 4);
    REQUIRE(subs.size() > 0);

    // The sub-tick step should be uniform throughout.
    float majorStep = std::abs(majors[1].value - majors[0].value);
    float expectedSubStep = majorStep / static_cast<float>(5);  // subTicksPerInterval=4

    // Every sub-tick must lie on a grid with spacing = expectedSubStep,
    // anchored at the first sub-tick position. Consecutive gaps may be
    // 2× expectedSubStep when a major tick falls on a grid line.
    float gridBase = subs.front().value;
    for (size_t i = 0; i < subs.size(); ++i) {
        float offset = subs[i].value - gridBase;
        float remainder = std::fmod(offset, expectedSubStep);
        // remainder should be ~0 (on the grid)
        REQUIRE(std::abs(remainder) < 0.01f);
        REQUIRE(std::abs(remainder - expectedSubStep) > 0.01f);  // not one full step off
    }

    // No two consecutive gaps should exceed 2× subStep
    for (size_t i = 1; i < subs.size(); ++i) {
        float gap = subs[i].value - subs[i - 1].value;
        REQUIRE(gap <= expectedSubStep * 2.0f + 0.01f);
    }

    // No sub-tick should coincide with a major tick
    for (const auto& s : subs) {
        for (const auto& m : majors) {
            REQUIRE(std::abs(s.value - m.value) > expectedSubStep * 0.1f);
        }
    }
}

TEST_CASE("generateSubTicks linear — cross-zero range has uniform spacing", "[subticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min  = -9.37742f;
    scale.max  = 11.8088f;

    auto majors = scibar::generateTicks(scale, 5, 2);
    REQUIRE(majors.size() >= 3);

    auto subs = scibar::generateSubTicks(scale, majors, 4);
    REQUIRE(subs.size() > 0);

    // Every sub-tick must lie on a uniform grid.
    float majorStep = std::abs(majors[1].value - majors[0].value);
    float subStep   = majorStep / 5.0f;

    float gridBase = subs.front().value;
    for (size_t i = 0; i < subs.size(); ++i) {
        float offset = subs[i].value - gridBase;
        float remainder = std::fmod(offset, subStep);
        INFO("Sub-tick " << i << " value=" << subs[i].value
             << " offset=" << offset << " remainder=" << remainder);
        REQUIRE(std::abs(remainder) < 0.01f);
    }

    // Sub-ticks must cover leading and trailing gaps.
    float firstMajor = majors.front().value;
    float lastMajor  = majors.back().value;
    if (firstMajor > lastMajor) std::swap(firstMajor, lastMajor);

    bool hasSubBelowFirst = false;
    for (const auto& s : subs) {
        if (s.value < firstMajor) { hasSubBelowFirst = true; break; }
    }
    REQUIRE(hasSubBelowFirst);

    bool hasSubAboveLast = false;
    for (const auto& s : subs) {
        if (s.value > lastMajor) { hasSubAboveLast = true; break; }
    }
    REQUIRE(hasSubAboveLast);

    // No sub-tick should coincide with a major tick
    for (const auto& s : subs) {
        for (const auto& m : majors) {
            REQUIRE(std::abs(s.value - m.value) > subStep * 0.1f);
        }
    }
}

TEST_CASE("generateSubTicks — all within range, linear", "[subticks]") {
    // Verify every sub-tick is strictly within [scale.min, scale.max] for linear scales.
    struct Case { float min; float max; const char* desc; };
    for (auto [lo, hi, desc] : {
            Case{0.0f, 100.0f, "positive"},
            Case{-4.0f, -2.0f, "negative-only"},
            Case{-9.38f, 11.81f, "cross-zero"},
            Case{5.0f, 11.0f, "positive-offset"},
            Case{-0.001f, 0.001f, "tiny-range"},
        }) {
        DYNAMIC_SECTION(desc) {
            scibar::Scale scale;
            scale.type = scibar::ScaleType::Linear;
            scale.min  = lo;
            scale.max  = hi;

            auto majors = scibar::generateTicks(scale, 5, 2);
            REQUIRE(majors.size() >= 2);

            auto subs = scibar::generateSubTicks(scale, majors, 4);
            for (const auto& s : subs) {
                INFO("Sub-tick " << s.value << " outside [" << lo << ", " << hi << "]");
                REQUIRE(s.value > lo);
                REQUIRE(s.value < hi);
            }
        }
    }
}

TEST_CASE("generateSubTicks — all within range, diverging", "[subticks]") {
    scibar::Scale scale;
    scale.type     = scibar::ScaleType::Diverging;
    scale.min      = -3.0f;
    scale.max      = 7.0f;
    scale.midpoint = 2.0f;

    auto majors = scibar::generateTicks(scale, 5, 2);
    REQUIRE(majors.size() >= 3);

    auto subs = scibar::generateSubTicks(scale, majors, 3);
    for (const auto& s : subs) {
        INFO("Sub-tick " << s.value << " outside [" << scale.min << ", " << scale.max << "]");
        REQUIRE(s.value > scale.min);
        REQUIRE(s.value < scale.max);
    }
}

TEST_CASE("generateSubTicks — all within range, logarithmic", "[subticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Logarithmic;
    scale.min  = 1.0f;
    scale.max  = 1000.0f;

    auto majors = scibar::generateTicks(scale, 5, 3);
    REQUIRE(majors.size() >= 2);

    auto subs = scibar::generateSubTicks(scale, majors, 4);
    // Log sub-ticks ignore subTicksPerInterval (they use 2–9 × decade).
    // They should still all be within range.
    for (const auto& s : subs) {
        INFO("Sub-tick " << s.value << " outside [" << scale.min << ", " << scale.max << "]");
        REQUIRE(s.value > scale.min);
        REQUIRE(s.value < scale.max);
    }
}

TEST_CASE("generateSubTicks — categorical returns empty", "[subticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Categorical;
    scale.min  = 0.0f;
    scale.max  = 5.0f;

    // Give it some dummy major ticks
    std::vector<scibar::Tick> majors = {{0.0f, "A"}, {5.0f, "E"}};
    auto subs = scibar::generateSubTicks(scale, majors, 4);
    REQUIRE(subs.empty());
}

TEST_CASE("generateSubTicks — linear inverted", "[subticks]") {
    scibar::Scale scale;
    scale.type     = scibar::ScaleType::Linear;
    scale.min      = 0.0f;
    scale.max      = 100.0f;
    scale.inverted = true;

    auto majors = scibar::generateTicks(scale, 5, 2);
    REQUIRE(majors.size() >= 3);

    // Inverted major ticks should be in descending order
    REQUIRE(majors.front().value > majors.back().value);

    auto subs = scibar::generateSubTicks(scale, majors, 4);

    // All sub-ticks within range
    for (const auto& s : subs) {
        REQUIRE(s.value > scale.min);
        REQUIRE(s.value < scale.max);
    }

    // Sub-ticks should be in descending order (matching inverted majors)
    if (subs.size() >= 2) {
        REQUIRE(subs.front().value > subs.back().value);
    }
}

// Test colormap: returns a reference to a static viridis instance
static const std::vector<scibar::Color>& testColormap() {
    static std::vector<scibar::Color> cmap = scibar::util::viridis();
    return cmap;
}

// All-white colormap — used as a control so gradient color does not
// masquerade as a frame line in frame-presence tests.
static const std::vector<scibar::Color>& whiteColormap() {
    static std::vector<scibar::Color> cmap(256, scibar::Color{255, 255, 255, 255});
    return cmap;
}

TEST_CASE("drawColorBar basic", "[draw]") {
    auto cmap = testColormap();

    const int W = 200, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();

    scibar::Rect bounds{50, 50, 40, 500};
    scibar::Rect result = scibar::drawColorBar(cv, bounds, spec, style);

    REQUIRE(result.x == bounds.x);
    REQUIRE(result.y == bounds.y);
    REQUIRE(result.width == bounds.width);
    REQUIRE(result.height == bounds.height);

    // Pixels in the bar area should be non-zero (not all black)
    bool hasColor = false;
    for (int y = 60; y < 540 && !hasColor; ++y) {
        for (int x = 55; x < 85 && !hasColor; ++x) {
            if (buf[y * W + x] != 0) hasColor = true;
        }
    }
    REQUIRE(hasColor);
}

TEST_CASE("drawLabel basic", "[draw]") {
    auto cmap = testColormap();
    scibar::Font font;  // uses embedded Inter font

    const int W = 400, H = 100;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.colormap = scibar::ColorMapView(cmap);
    spec.label = "Test Label";

    scibar::Style style = scibar::Style::defaultLight();
    style.font = font;

    scibar::Rect bounds{0, 0, W, H};
    scibar::Rect result = scibar::drawLabel(cv, bounds, spec.label, style);

    REQUIRE(result.width > 0);
    REQUIRE(result.height > 0);
}

TEST_CASE("drawTicks basic", "[draw]") {
    auto cmap = testColormap();
    scibar::Font font;  // uses embedded Inter font

    const int W = 400, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();
    style.font = font;

    scibar::Rect barBounds{50, 50, 40, 500};
    scibar::Rect result = scibar::drawTicks(cv, barBounds, spec, style);

    // Tick bounds should extend beyond the bar to the right
    REQUIRE(result.x <= barBounds.x);
    REQUIRE(result.width >= barBounds.width);
}

TEST_CASE("drawLegend basic", "[draw]") {
    auto cmap = testColormap();
    scibar::Font font;  // uses embedded Inter font

    const int W = 300, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.label = "Value";
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();
    style.font = font;

    scibar::LayoutResult result = scibar::drawLegend(cv, spec, style);

    REQUIRE(result.totalBoundingBox.width > 0);
    REQUIRE(result.totalBoundingBox.height > 0);
    REQUIRE(result.colorbarBoundingBox.width > 0);
    REQUIRE(result.colorbarBoundingBox.height > 0);
    REQUIRE(result.generatedTickCount > 0);

    // Verify pixels were written (not all zero)
    bool hasContent = false;
    for (size_t i = 0; i < buf.size() && !hasContent; ++i) {
        if (buf[i] != 0) hasContent = true;
    }
    REQUIRE(hasContent);
}

TEST_CASE("viridis colormap", "[util]") {
    auto cmap = scibar::util::viridis();
    REQUIRE(cmap.size() == 256);

    // All entries should be fully opaque
    for (const auto& c : cmap) {
        REQUIRE(c.a == 255);
    }

    // First entry is dark purple
    REQUIRE(cmap[0].r < 80);
    REQUIRE(cmap[0].g < 10);
    REQUIRE(cmap[0].b > 80);

    // Last entry is bright yellow
    REQUIRE(cmap[255].r > 230);
    REQUIRE(cmap[255].g > 220);
    REQUIRE(cmap[255].b < 60);
}

TEST_CASE("vik diverging colormap", "[util]") {
    auto cmap = scibar::util::vik();
    REQUIRE(cmap.size() == 256);

    for (const auto& c : cmap) {
        REQUIRE(c.a == 255);
    }

    // First entry is dark blue
    REQUIRE(cmap[0].b > cmap[0].r);
    // Middle is yellowish (midpoint ~128)
    REQUIRE(cmap[128].g > cmap[128].b);
    // Last entry is reddish
    REQUIRE(cmap[255].r > cmap[255].b);
}

TEST_CASE("generateTicks diverging", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Diverging;
    scale.min  = -3.0f;
    scale.max  = 7.0f;
    scale.midpoint = 2.0f;

    auto ticks = scibar::generateTicks(scale, 5, 2);
    REQUIRE(ticks.size() >= 3);

    // Midpoint should be included
    bool hasMidpoint = false;
    for (const auto& t : ticks) {
        if (std::abs(t.value - 2.0f) < 0.01f) hasMidpoint = true;
    }
    REQUIRE(hasMidpoint);
}

TEST_CASE("ticksInward", "[style]") {
    // Default is outward
    {
        scibar::Style s = scibar::Style::defaultLight();
        REQUIRE(s.ticksInward == false);
    }

    // Verify it can be set to true
    {
        scibar::Style s = scibar::Style::defaultLight();
        s.ticksInward = true;
        REQUIRE(s.ticksInward == true);
    }
}

TEST_CASE("drawTicks outward places tick line outside bar", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 200, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF); // white
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font        = font;
    style.ticksInward = false;
    style.tickLength  = 8.0f; // longer for reliable detection

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    // Midpoint tick at 50 → 50% of bar height
    int tickY    = barBounds.y + barBounds.height / 2; // 120
    int barRight = barBounds.x + barBounds.width;       // 70

    // Outward: non-white pixels at (barRight+1..barRight+tickLength, tickY)
    bool foundOutside = false;
    for (int dx = 1; dx <= static_cast<int>(style.tickLength); ++dx) {
        if (isNotWhite(buf[tickY * W + (barRight + dx)])) {
            foundOutside = true;
            break;
        }
    }
    REQUIRE(foundOutside);

    // Inward side should be white (canvas background)
    bool foundInside = false;
    for (int dx = 1; dx <= static_cast<int>(style.tickLength); ++dx) {
        if (isNotWhite(buf[tickY * W + (barRight - dx)])) {
            foundInside = true;
            break;
        }
    }
    REQUIRE_FALSE(foundInside);
}

TEST_CASE("drawTicks inward places tick line inside bar", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 200, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, WHITE_PIXEL);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font        = font;
    style.ticksInward = true;
    style.tickLength  = 8.0f;

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    int tickY    = barBounds.y + barBounds.height / 2;
    int barRight = barBounds.x + barBounds.width;

    // Inward: non-white pixels inside bar
    bool foundInside = false;
    for (int dx = 1; dx <= static_cast<int>(style.tickLength); ++dx) {
        if (isNotWhite(buf[tickY * W + (barRight - dx)])) {
            foundInside = true;
            break;
        }
    }
    REQUIRE(foundInside);

    // Outside should be white
    bool foundOutside = false;
    for (int dx = 1; dx <= static_cast<int>(style.tickLength); ++dx) {
        if (isNotWhite(buf[tickY * W + (barRight + dx)])) {
            foundOutside = true;
            break;
        }
    }
    REQUIRE_FALSE(foundOutside);
}

TEST_CASE("drawTicks outward horizontal places ticks below bar", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 300, H = 150;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font        = font;
    style.ticksInward = false;
    style.tickLength  = 8.0f;

    scibar::Rect barBounds{40, 30, 200, 20};
    scibar::drawTicks(cv, barBounds, spec, style, scibar::Orientation::Horizontal);

    // Midpoint tick at horizontal center of bar
    int tickX     = barBounds.x + barBounds.width / 2;  // 140
    int barBottom = barBounds.y + barBounds.height;      // 50

    // Outward: non-white pixels below bar
    bool foundBelow = false;
    for (int dy = 1; dy <= static_cast<int>(style.tickLength); ++dy) {
        if (isNotWhite(buf[(barBottom + dy) * W + tickX])) {
            foundBelow = true;
            break;
        }
    }
    REQUIRE(foundBelow);

    // Above should be white
    bool foundAbove = false;
    for (int dy = 1; dy <= static_cast<int>(style.tickLength); ++dy) {
        if (isNotWhite(buf[(barBottom - dy) * W + tickX])) {
            foundAbove = true;
            break;
        }
    }
    REQUIRE_FALSE(foundAbove);
}

TEST_CASE("drawTicks inward horizontal places ticks above bar", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 300, H = 150;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font        = font;
    style.ticksInward = true;
    style.tickLength  = 8.0f;

    scibar::Rect barBounds{40, 30, 200, 20};
    scibar::drawTicks(cv, barBounds, spec, style, scibar::Orientation::Horizontal);

    int tickX     = barBounds.x + barBounds.width / 2;
    int barBottom = barBounds.y + barBounds.height;

    // Inward: non-white pixels above bar (inside)
    bool foundAbove = false;
    for (int dy = 1; dy <= static_cast<int>(style.tickLength); ++dy) {
        if (isNotWhite(buf[(barBottom - dy) * W + tickX])) {
            foundAbove = true;
            break;
        }
    }
    REQUIRE(foundAbove);

    // Outside (below) should be white
    bool foundBelow = false;
    for (int dy = 1; dy <= static_cast<int>(style.tickLength); ++dy) {
        if (isNotWhite(buf[(barBottom + dy) * W + tickX])) {
            foundBelow = true;
            break;
        }
    }
    REQUIRE_FALSE(foundBelow);
}

TEST_CASE("drawSubTicks shows sub-ticks when enabled", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 200, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    // Major ticks at 0, 25, 50, 75, 100 → sub-tick at 10 is between 0 and 25
    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {25.0f, "25"}, {50.0f, "50"}, {75.0f, "75"}, {100.0f, "100"}};
    spec.subTicks   = {{10.0f}}; // explicit sub-tick at value 10
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font          = font;
    style.showSubTicks  = true;
    style.subTickLength = 6.0f;

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawSubTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    // Sub-tick at value 10 → 10% from bottom (since vertical origin at top)
    // fraction = 0.1, y = barBounds.y + barBounds.height - 0.1 * barBounds.height
    int subTickY = barBounds.y + barBounds.height -
                   static_cast<int>(0.1f * static_cast<float>(barBounds.height)); // 20 + 200 - 20 = 200
    int barRight = barBounds.x + barBounds.width;

    // Should find non-white pixel at the sub-tick line
    bool found = false;
    for (int dx = 1; dx <= static_cast<int>(style.subTickLength); ++dx) {
        if (isNotWhite(buf[subTickY * W + (barRight + dx)])) {
            found = true;
            break;
        }
    }
    REQUIRE(found);
}

TEST_CASE("drawSubTicks hidden when showSubTicks false", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 200, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};
    spec.subTicks   = {{25.0f}};
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font          = font;
    style.showSubTicks  = false;
    style.subTickLength = 6.0f;

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawSubTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    // Sub-tick at 25 → 75% from bottom
    int subTickY = barBounds.y + barBounds.height -
                   static_cast<int>(0.25f * static_cast<float>(barBounds.height));
    int barRight = barBounds.x + barBounds.width;

    // Should NOT find non-white pixels at the expected sub-tick position
    bool found = false;
    for (int dx = 1; dx <= static_cast<int>(style.subTickLength); ++dx) {
        if (isNotWhite(buf[subTickY * W + (barRight + dx)])) {
            found = true;
            break;
        }
    }
    REQUIRE_FALSE(found);
}

TEST_CASE("drawSubTicks inward places marks inside bar", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 200, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};
    spec.subTicks   = {{30.0f}};
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font          = font;
    style.ticksInward   = true;
    style.showSubTicks  = true;
    style.subTickLength = 6.0f;

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawSubTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    // Sub-tick at 30 → 70% from bottom
    int subTickY = barBounds.y + barBounds.height -
                   static_cast<int>(0.3f * static_cast<float>(barBounds.height));
    int barRight = barBounds.x + barBounds.width;

    // Inward: non-white pixels inside bar
    bool foundInside = false;
    for (int dx = 1; dx <= static_cast<int>(style.subTickLength); ++dx) {
        if (isNotWhite(buf[subTickY * W + (barRight - dx)])) {
            foundInside = true;
            break;
        }
    }
    REQUIRE(foundInside);

    // Outside should be white
    bool foundOutside = false;
    for (int dx = 1; dx <= static_cast<int>(style.subTickLength); ++dx) {
        if (isNotWhite(buf[subTickY * W + (barRight + dx)])) {
            foundOutside = true;
            break;
        }
    }
    REQUIRE_FALSE(foundOutside);
}

TEST_CASE("drawTicks renders label text near expected anchor", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 250, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{50.0f, "50"}}; // single label at midpoint
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font        = font;
    style.tickLength  = 5.0f;

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    // Label anchor: (barRight + tickLength + 3, tickY)
    // Text is left-aligned, alphabetic baseline → glyph extends right + up from anchor
    int tickY    = barBounds.y + barBounds.height / 2;
    int anchorX  = barBounds.x + barBounds.width + static_cast<int>(style.tickLength) + 3;

    // Search a patch: anchorX..anchorX+20, tickY-14..tickY  (conservative glyph bounds)
    int darkCount = 0;
    for (int dy = -14; dy <= 0; ++dy) {
        for (int dx = 0; dx <= 20; ++dx) {
            int px = (tickY + dy) * W + (anchorX + dx);
            if (px >= 0 && px < static_cast<int>(buf.size())) {
                if (isDarkPixel(buf[px])) darkCount++;
            }
        }
    }
    // At least some dark pixels from glyph rendering
    REQUIRE(darkCount > 3);
}

TEST_CASE("drawTicks empty label produces no glyph pixels", "[draw][pixel]") {
    // Negative control: without a label, the label patch should have negligible
    // dark pixels (at most tick-line anti-aliasing bleed, much less than a glyph).
    scibar::Font font;  // uses embedded Inter font

    const int W = 250, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, WHITE_PIXEL);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{50.0f, ""}}; // empty label
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font        = font;
    style.tickLength  = 5.0f;

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    int tickY    = barBounds.y + barBounds.height / 2;
    int anchorX  = barBounds.x + barBounds.width + static_cast<int>(style.tickLength) + 3;

    int darkCount = 0;
    for (int dy = -14; dy <= 0; ++dy) {
        for (int dx = 0; dx <= 20; ++dx) {
            int px = (tickY + dy) * W + (anchorX + dx);
            if (px >= 0 && px < static_cast<int>(buf.size())) {
                if (isDarkPixel(buf[px])) darkCount++;
            }
        }
    }
    // Without a label, there should be very few dark pixels (only tick bleed)
    REQUIRE(darkCount <= 3);
}

TEST_CASE("drawTicks label rendered in dark colors for light mode", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 250, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{50.0f, "50"}};
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultLight();
    style.font       = font;
    style.tickLength = 5.0f;

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    // Tick line pixel should be dark (near black)
    int tickY    = barBounds.y + barBounds.height / 2;
    int barRight = barBounds.x + barBounds.width;
    int midTickX = barRight + static_cast<int>(style.tickLength) / 2;

    REQUIRE(isDarkPixel(buf[tickY * W + midTickX]));
}

TEST_CASE("drawTicks label rendered in light colors for dark mode", "[draw][pixel]") {
    scibar::Font font;  // uses embedded Inter font

    const int W = 250, H = 300;
    // Use black canvas so white ticks are detectable
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFF000000);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{50.0f, "50"}};
    spec.colormap   = scibar::ColorMapView(testColormap());

    scibar::Style style = scibar::Style::defaultDark();
    style.font       = font;
    style.tickLength = 5.0f;

    scibar::Rect barBounds{40, 20, 30, 200};
    scibar::drawTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);

    // Tick line pixel should be light (near white)
    int tickY    = barBounds.y + barBounds.height / 2;
    int barRight = barBounds.x + barBounds.width;
    int midTickX = barRight + static_cast<int>(style.tickLength) / 2;

    REQUIRE(isLightPixel(buf[tickY * W + midTickX], 100));
}

TEST_CASE("drawColorBar respects showFrame", "[draw][pixel]") {
    auto cmap = testColormap();

    const int W = 200, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.colormap   = scibar::ColorMapView(cmap);

    // --- With frame: top edge pixel should be non-white ---
    scibar::Style styleFrame = scibar::Style::defaultLight();
    styleFrame.showFrame  = true;
    styleFrame.frameColor = scibar::Color{0, 0, 0, 255};

    scibar::Rect bounds{40, 20, 30, 200};
    scibar::drawColorBar(cv, bounds, spec, styleFrame);

    // Frame line at top edge midpoint — anti-aliased black on white
    int frameY = bounds.y;
    int frameX = bounds.x + bounds.width / 2;
    REQUIRE(isNotWhite(buf[frameY * W + frameX]));

    // --- Without frame: a pixel just above the bar should be untouched white ---
    std::vector<uint32_t> buf2(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv2{buf2.data(), W, H};

    scibar::Style styleNoFrame = scibar::Style::defaultLight();
    styleNoFrame.showFrame = false;

    scibar::drawColorBar(cv2, bounds, spec, styleNoFrame);

    // Pixel at (frameX, frameY - 1) is above bar — should remain white
    REQUIRE(buf2[(frameY - 1) * W + frameX] == WHITE_PIXEL);
}

TEST_CASE("drawColorBar frame detection not confused by gradient", "[draw][pixel]") {
    // Negative control: with an all-white colormap, the bar itself is white.
    // A non-white pixel at the top edge can ONLY come from the frame line.
    auto whiteCmap = whiteColormap();

    const int W = 200, H = 300;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.colormap   = scibar::ColorMapView(whiteCmap);

    scibar::Rect bounds{40, 20, 30, 200};
    int frameY = bounds.y;
    int frameX = bounds.x + bounds.width / 2;

    // --- With frame: top edge midpoint should be non-white (black frame line) ---
    scibar::Style styleFrame = scibar::Style::defaultLight();
    styleFrame.showFrame  = true;
    styleFrame.frameColor = scibar::Color{0, 0, 0, 255};
    scibar::drawColorBar(cv, bounds, spec, styleFrame);
    REQUIRE(isNotWhite(buf[frameY * W + frameX]));

    // --- Without frame: top edge should be pure white bar background ---
    std::vector<uint32_t> buf2(static_cast<size_t>(W) * H, 0xFFFFFFFF);
    scibar::Canvas cv2{buf2.data(), W, H};

    scibar::Style styleNoFrame = scibar::Style::defaultLight();
    styleNoFrame.showFrame = false;
    scibar::drawColorBar(cv2, bounds, spec, styleNoFrame);
    REQUIRE(buf2[frameY * W + frameX] == WHITE_PIXEL);
}

TEST_CASE("exportToSVG respects ticksInward in line direction", "[svg]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};
    spec.colormap   = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.totalWidth     = 400;
    opts.totalHeight    = 300;
    opts.colorbarBounds = {100, 50, 40, 200};

    // --- Outward ticks: vertical x2 > x1 ---
    {
        scibar::Style style = scibar::Style::defaultLight();
        style.ticksInward = false;
        std::string svg = scibar::exportToSVG(spec, style, opts,
                                             scibar::Orientation::Vertical);
        auto lines = extractSvgLines(svg);
        REQUIRE(lines.size() >= 3); // at least 3 tick lines
        for (const auto& l : lines) {
            REQUIRE(l[2] > l[0]); // x2 > x1 (tick extends right)
        }
    }

    // --- Inward ticks: vertical x2 < x1 ---
    {
        scibar::Style style = scibar::Style::defaultLight();
        style.ticksInward = true;
        std::string svg = scibar::exportToSVG(spec, style, opts,
                                             scibar::Orientation::Vertical);
        auto lines = extractSvgLines(svg);
        REQUIRE(lines.size() >= 3);
        for (const auto& l : lines) {
            REQUIRE(l[2] < l[0]); // x2 < x1 (tick extends left)
        }
    }

    // --- Outward ticks: horizontal y2 > y1 ---
    {
        scibar::Style style = scibar::Style::defaultLight();
        style.ticksInward = false;
        std::string svg = scibar::exportToSVG(spec, style, opts,
                                             scibar::Orientation::Horizontal);
        auto lines = extractSvgLines(svg);
        REQUIRE(lines.size() >= 3);
        for (const auto& l : lines) {
            REQUIRE(l[3] > l[1]); // y2 > y1 (tick extends down)
        }
    }

    // --- Inward ticks: horizontal y2 < y1 ---
    {
        scibar::Style style = scibar::Style::defaultLight();
        style.ticksInward = true;
        std::string svg = scibar::exportToSVG(spec, style, opts,
                                             scibar::Orientation::Horizontal);
        auto lines = extractSvgLines(svg);
        REQUIRE(lines.size() >= 3);
        for (const auto& l : lines) {
            REQUIRE(l[3] < l[1]); // y2 < y1 (tick extends up)
        }
    }
}

TEST_CASE("exportToSVG label position unchanged by ticksInward", "[svg]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.ticks      = {{0.0f, "0"}, {50.0f, "50"}, {100.0f, "100"}};
    spec.colormap   = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.totalWidth     = 400;
    opts.totalHeight    = 300;
    opts.colorbarBounds = {100, 50, 40, 200};

    // Collect text x positions with outward ticks
    scibar::Style styleOut = scibar::Style::defaultLight();
    styleOut.ticksInward = false;
    std::string svgOut = scibar::exportToSVG(spec, styleOut, opts,
                                            scibar::Orientation::Vertical);
    auto textXOut = extractSvgTextX(svgOut);

    // Collect text x positions with inward ticks
    scibar::Style styleIn = scibar::Style::defaultLight();
    styleIn.ticksInward = true;
    std::string svgIn = scibar::exportToSVG(spec, styleIn, opts,
                                           scibar::Orientation::Vertical);
    auto textXIn = extractSvgTextX(svgIn);

    // Label x positions must be identical regardless of tick direction
    REQUIRE(textXOut.size() == textXIn.size());
    REQUIRE(textXOut.size() >= 3);
    for (size_t i = 0; i < textXOut.size(); ++i) {
        REQUIRE(textXOut[i] == textXIn[i]);
    }
}

TEST_CASE("drawColorBar diverging", "[draw]") {
    auto cmap = scibar::util::vik();

    const int W = 200, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};
    scibar::fillCanvas(cv, scibar::Color{255, 255, 255, 255});

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Diverging;
    spec.scale.min  = -3.0f;
    spec.scale.max  = 7.0f;
    spec.scale.midpoint = 2.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();
    scibar::Rect bounds{50, 50, 40, 500};
    scibar::Rect result = scibar::drawColorBar(cv, bounds, spec, style);

    REQUIRE(result.width == bounds.width);
    REQUIRE(result.height == bounds.height);

    // Pixels in the bar area should be non-zero
    bool hasColor = false;
    for (int y = 60; y < 540 && !hasColor; ++y) {
        if (buf[y * W + 55] != 0xFFFFFFFF) hasColor = true;
    }
    REQUIRE(hasColor);
}

TEST_CASE("exportToSVG basic", "[svg]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.label = "Value";
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.totalWidth = 400;
    opts.totalHeight = 600;
    opts.colorbarBounds = {50, 50, 40, 500};

    std::string svg = scibar::exportToSVG(spec, scibar::Style::defaultLight(), opts);

    REQUIRE_FALSE(svg.empty());
    REQUIRE(svg.find("<svg") != std::string::npos);
    REQUIRE(svg.find("linearGradient") != std::string::npos);
    REQUIRE(svg.find("</svg>") != std::string::npos);
}

TEST_CASE("exportToSVG with hybrid image", "[svg]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 1.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.mainImageHref = "mesh.png";
    opts.mainImageBounds = {0, 0, 400, 400};

    std::string svg = scibar::exportToSVG(spec, scibar::Style::defaultLight(), opts);

    REQUIRE(svg.find("<image") != std::string::npos);
    REQUIRE(svg.find("crisp-edges") != std::string::npos);
}

TEST_CASE("exportToSVG categorical", "[svg]") {
    // Small categorical colormap
    std::vector<scibar::Color> catCmap = {
        scibar::Color{255, 0, 0, 255},
        scibar::Color{0, 255, 0, 255},
        scibar::Color{0, 0, 255, 255},
    };

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Categorical;
    spec.colormap = scibar::ColorMapView(catCmap);

    std::string svg = scibar::exportToSVG(spec);

    // Categorical should NOT have a linearGradient
    REQUIRE(svg.find("linearGradient") == std::string::npos);
    // Should have individual rects
    REQUIRE(svg.find("<rect") != std::string::npos);
    REQUIRE(svg.find("</svg>") != std::string::npos);
}

TEST_CASE("drawColorBar reversed continuous", "[draw][reverse]") {
    auto cmap = testColormap();

    const int W = 200, H = 600;
    auto makeBuf = [&]() {
        std::vector<uint32_t> b(static_cast<size_t>(W) * H, 0);
        return b;
    };

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    // Forward
    auto fwdBuf = makeBuf();
    scibar::Canvas fwdCv{fwdBuf.data(), W, H};
    scibar::Style fwdStyle = scibar::Style::defaultDark();
    fwdStyle.reverseColors = false;
    scibar::Rect bounds{50, 50, 40, 500};
    scibar::drawColorBar(fwdCv, bounds, spec, fwdStyle);

    // Reversed
    auto revBuf = makeBuf();
    scibar::Canvas revCv{revBuf.data(), W, H};
    scibar::Style revStyle = scibar::Style::defaultDark();
    revStyle.reverseColors = true;
    scibar::drawColorBar(revCv, bounds, spec, revStyle);

    // Top and bottom of forward bar should differ from each other
    int topY = 60, botY = 530, midX = 70;
    REQUIRE(fwdBuf[topY * W + midX] != fwdBuf[botY * W + midX]);

    // Top and bottom of reversed bar should differ from each other
    REQUIRE(revBuf[topY * W + midX] != revBuf[botY * W + midX]);

    // Reversed bar should swap the color order: fwd top ≠ rev top, fwd bot ≠ rev bot
    REQUIRE(fwdBuf[topY * W + midX] != revBuf[topY * W + midX]);
    REQUIRE(fwdBuf[botY * W + midX] != revBuf[botY * W + midX]);
}

TEST_CASE("drawColorBar reversed categorical", "[draw][reverse]") {
    std::vector<scibar::Color> catCmap = {
        scibar::Color{255, 0, 0, 255},
        scibar::Color{0, 255, 0, 255},
        scibar::Color{0, 0, 255, 255},
    };

    const int W = 100, H = 300;
    auto makeBuf = [&]() {
        std::vector<uint32_t> b(static_cast<size_t>(W) * H, 0);
        return b;
    };

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Categorical;
    spec.colormap = scibar::ColorMapView(catCmap);

    // Forward
    auto fwdBuf = makeBuf();
    scibar::Canvas fwdCv{fwdBuf.data(), W, H};
    scibar::Style fwdStyle = scibar::Style::defaultDark();
    fwdStyle.reverseColors = false;
    scibar::Rect bounds{25, 0, 50, 300};
    scibar::drawColorBar(fwdCv, bounds, spec, fwdStyle);

    // Reversed
    auto revBuf = makeBuf();
    scibar::Canvas revCv{revBuf.data(), W, H};
    scibar::Style revStyle = scibar::Style::defaultDark();
    revStyle.reverseColors = true;
    scibar::drawColorBar(revCv, bounds, spec, revStyle);

    // Top block of forward (red) should match bottom block of reversed (red)
    int midX = 50;
    int topY = 50, midY = 150, botY = 250;
    REQUIRE(fwdBuf[topY * W + midX] == revBuf[botY * W + midX]);  // fwd top = rev bottom
    REQUIRE(fwdBuf[botY * W + midX] == revBuf[topY * W + midX]);  // fwd bottom = rev top
    // Middle should stay the same (it's the middle category)
    REQUIRE(fwdBuf[midY * W + midX] == revBuf[midY * W + midX]);
}

TEST_CASE("exportToSVG reversed continuous", "[svg][reverse]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.label = "Value";
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.colorbarBounds = {50, 50, 40, 500};

    // Forward: y1="1" ... y2="0" (top-to-bottom gradient)
    scibar::Style fwdStyle = scibar::Style::defaultLight();
    fwdStyle.reverseColors = false;
    std::string fwdSvg = scibar::exportToSVG(spec, fwdStyle, opts);

    // Reversed: y1="0" ... y2="1" (bottom-to-top gradient)
    scibar::Style revStyle = scibar::Style::defaultLight();
    revStyle.reverseColors = true;
    std::string revSvg = scibar::exportToSVG(spec, revStyle, opts);

    // Forward has y1="1" (gradient starts at bottom), reversed has y1="0" (starts at top)
    REQUIRE(fwdSvg.find("y1=\"1\"") != std::string::npos);
    REQUIRE(revSvg.find("y1=\"0\"") != std::string::npos);
}

TEST_CASE("exportToSVG reversed horizontal", "[svg][reverse]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 1.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.colorbarBounds = {50, 50, 500, 40};

    // Forward: x1="0" ... x2="1" (left-to-right)
    scibar::Style fwdStyle = scibar::Style::defaultLight();
    fwdStyle.reverseColors = false;
    std::string fwdSvg = scibar::exportToSVG(spec, fwdStyle, opts,
                                               scibar::Orientation::Horizontal);

    // Reversed: x1="1" ... x2="0" (right-to-left)
    scibar::Style revStyle = scibar::Style::defaultLight();
    revStyle.reverseColors = true;
    std::string revSvg = scibar::exportToSVG(spec, revStyle, opts,
                                               scibar::Orientation::Horizontal);

    // Forward has x1="0" (gradient starts at left), reversed has x1="1" (starts at right)
    REQUIRE(fwdSvg.find("x1=\"0\"") != std::string::npos);
    REQUIRE(revSvg.find("x1=\"1\"") != std::string::npos);
}

// =========================================================================
// Raster vs. Vector placement comparison tests
// =========================================================================
// These tests render the same spec via both the raster (canvas) and vector
// (SVG) pathways, write both outputs to disk for visual inspection, and
// compare key element positions between the two renderers.

static const char* OUTPUT_DIR = "test_output";

// Shared spec: linear 0–100, viridis, label "Temperature (°C)", light style
static scibar::Spec makeComparisonSpec() {
    scibar::Spec spec;
    spec.scale    = scibar::Scale{scibar::ScaleType::Linear, 0.0f, 100.0f};
    spec.label    = "Temperature (°C)";
    spec.colormap = scibar::ColorMapView(testColormap());
    return spec;
}

static scibar::Style makeComparisonStyle() {
    scibar::Style style = scibar::Style::defaultLight();
    style.font.size = 14.0f;  // explicit for reproducibility
    return style;
}

// Helper: ensure output directory exists
static void ensureOutputDir() {
    std::error_code ec;
    std::filesystem::create_directories(OUTPUT_DIR, ec);
}

// Helper: count dark (non-background) pixels in a rectangular region
static int countDarkPixels(const scibar::Canvas& canvas, int x0, int y0,
                           int x1, int y1, int threshold = 200) {
    int count = 0;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            uint32_t px = canvas.pixels[y * canvas.width + x];
            uint8_t r = px & 0xFF, g = (px >> 8) & 0xFF, b = (px >> 16) & 0xFF;
            if (r < threshold && g < threshold && b < threshold)
                ++count;
        }
    }
    return count;
}

// ── Test 1: High-level API, Vertical ─────────────────────────────────────

TEST_CASE("raster-vs-vector: high-level vertical placement", "[raster-vs-vector][vertical][highlevel]") {
    ensureOutputDir();

    auto spec  = makeComparisonSpec();
    auto style = makeComparisonStyle();

    // ── Raster: drawLegend (shared layout) ──
    const int W = 300, H = 600;
    std::vector<uint32_t> rastBuf(static_cast<size_t>(W) * H, WHITE_PIXEL);
    scibar::Canvas rastCanvas{rastBuf.data(), W, H};

    scibar::LayoutResult layout = scibar::drawLegend(rastCanvas, spec, style,
                                                      scibar::Orientation::Vertical);
    scibar::writePPM(rastCanvas, (std::string(OUTPUT_DIR) + "/test01_highlevel_vertical.ppm").c_str());

    INFO("Raster bar bounds:   x=" << layout.colorbarBoundingBox.x
         << " y=" << layout.colorbarBoundingBox.y
         << " w=" << layout.colorbarBoundingBox.width
         << " h=" << layout.colorbarBoundingBox.height);

    // ── SVG: exportLegendToSVG (shared layout, same as drawLegend) ──
    std::string svgStr = scibar::exportLegendToSVG(spec, style, W, H,
                                                    scibar::Orientation::Vertical);
    {
        std::ofstream out(std::string(OUTPUT_DIR) + "/test01_highlevel_vertical.svg");
        out << svgStr;
    }

    // ── Compare positions ──
    auto svgBar = extractSvgBarRect(svgStr);
    INFO("SVG bar rect: x=" << svgBar[0] << " y=" << svgBar[1]
         << " w=" << svgBar[2] << " h=" << svgBar[3]);

    // Bar position should match (both use computeLegendLayout)
    CHECK(std::abs(svgBar[0] - layout.colorbarBoundingBox.x) <= 1.0f);
    CHECK(std::abs(svgBar[1] - layout.colorbarBoundingBox.y) <= 1.0f);
    CHECK(std::abs(svgBar[2] - layout.colorbarBoundingBox.width)  <= 1.0f);
    CHECK(std::abs(svgBar[3] - layout.colorbarBoundingBox.height) <= 1.0f);

    // SVG label position — should now match the raster label position
    // because exportLegendToSVG uses computeLegendLayout for label placement
    auto svgLabel = extractSvgLabelPosition(svgStr, spec.label);
    INFO("SVG label: x=" << svgLabel[0] << " y=" << svgLabel[1]);

    // Title X should be centered on canvas
    CHECK(svgLabel[0] == Catch::Approx(static_cast<float>(W) / 2.0f).margin(1.0f));

    // Title Y should be the same as raster (both use computeLegendLayout)
    int rasterLabelY = findTitleTopY(rastCanvas);
    INFO("Raster label first dark pixel Y: " << rasterLabelY);
    INFO("SVG label Y: " << svgLabel[1]);
    CHECK(rasterLabelY >= 0);
    // The SVG label Y should be close to the raster label Y (within font height)
    CHECK(std::abs(svgLabel[1] - static_cast<float>(rasterLabelY)) <= 20.0f);
}

// ── Test 2: High-level API, Horizontal ───────────────────────────────────
// Uses drawLegend with Orientation::Horizontal and exportLegendToSVG,
// both backed by computeLegendLayout for consistent placement.

TEST_CASE("raster-vs-vector: high-level horizontal placement", "[raster-vs-vector][horizontal][highlevel]") {
    ensureOutputDir();

    auto spec  = makeComparisonSpec();
    auto style = makeComparisonStyle();

    const int W = 700, H = 180;

    // ── Raster: drawLegend (shared layout, horizontal) ──
    std::vector<uint32_t> rastBuf(static_cast<size_t>(W) * H, WHITE_PIXEL);
    scibar::Canvas rastCanvas{rastBuf.data(), W, H};

    scibar::LayoutResult layout = scibar::drawLegend(rastCanvas, spec, style,
                                                      scibar::Orientation::Horizontal);
    scibar::writePPM(rastCanvas, (std::string(OUTPUT_DIR) + "/test02_highlevel_horizontal.ppm").c_str());

    INFO("Raster bar bounds:   x=" << layout.colorbarBoundingBox.x
         << " y=" << layout.colorbarBoundingBox.y
         << " w=" << layout.colorbarBoundingBox.width
         << " h=" << layout.colorbarBoundingBox.height);

    // ── SVG: exportLegendToSVG (shared layout) ──
    std::string svgStr = scibar::exportLegendToSVG(spec, style, W, H,
                                                    scibar::Orientation::Horizontal);
    {
        std::ofstream out(std::string(OUTPUT_DIR) + "/test02_highlevel_horizontal.svg");
        out << svgStr;
    }

    // ── Compare ──
    auto svgBar = extractSvgBarRect(svgStr);
    INFO("SVG bar rect: x=" << svgBar[0] << " y=" << svgBar[1]
         << " w=" << svgBar[2] << " h=" << svgBar[3]);

    CHECK(std::abs(svgBar[0] - layout.colorbarBoundingBox.x) <= 1.0f);
    CHECK(std::abs(svgBar[1] - layout.colorbarBoundingBox.y) <= 1.0f);
    CHECK(std::abs(svgBar[2] - layout.colorbarBoundingBox.width)  <= 1.0f);
    CHECK(std::abs(svgBar[3] - layout.colorbarBoundingBox.height) <= 1.0f);

    // Title X should be centered on canvas
    auto svgLabel = extractSvgLabelPosition(svgStr, spec.label);
    CHECK(svgLabel[0] == Catch::Approx(static_cast<float>(W) / 2.0f).margin(1.0f));

    // Title Y should be close between raster and SVG (shared layout)
    int rasterLabelY = findTitleTopY(rastCanvas);
    INFO("Raster label Y: " << rasterLabelY << "  SVG label Y: " << svgLabel[1]);
    CHECK(rasterLabelY >= 0);
    CHECK(std::abs(svgLabel[1] - static_cast<float>(rasterLabelY)) <= 20.0f);
}

// ── Test 3: Low-level API, Vertical ──────────────────────────────────────

TEST_CASE("raster-vs-vector: low-level vertical placement", "[raster-vs-vector][vertical][lowlevel]") {
    ensureOutputDir();

    auto spec  = makeComparisonSpec();
    auto style = makeComparisonStyle();

    const int W = 300, H = 600;
    scibar::Rect barBounds{60, 80, 40, 440};  // same for both renderers

    // ── Raster ──
    std::vector<uint32_t> rastBuf(static_cast<size_t>(W) * H, WHITE_PIXEL);
    scibar::Canvas rastCanvas{rastBuf.data(), W, H};

    // Title above bar: centered on bar
    scibar::Rect labelRect{barBounds.x, 10, barBounds.width, 40};
    scibar::Rect actualTitle = scibar::drawLabel(rastCanvas, labelRect, spec.label, style);

    scibar::Rect actualBar = scibar::drawColorBar(rastCanvas, barBounds, spec, style,
                                                   scibar::Orientation::Vertical);
    scibar::Rect tickBounds = scibar::drawTicks(rastCanvas, barBounds, spec, style,
                                                 scibar::Orientation::Vertical);
    scibar::drawSubTicks(rastCanvas, barBounds, spec, style,
                          scibar::Orientation::Vertical);

    scibar::writePPM(rastCanvas, (std::string(OUTPUT_DIR) + "/test03_lowlevel_vertical.ppm").c_str());

    INFO("Raster bar bounds:   x=" << actualBar.x << " y=" << actualBar.y
         << " w=" << actualBar.width << " h=" << actualBar.height);
    INFO("Raster label bounds: x=" << actualTitle.x << " y=" << actualTitle.y
         << " w=" << actualTitle.width << " h=" << actualTitle.height);
    INFO("Raster tick bounds:  x=" << tickBounds.x << " y=" << tickBounds.y
         << " w=" << tickBounds.width << " h=" << tickBounds.height);

    // ── SVG: identical colorbarBounds ──
    scibar::SVGOptions svgOpts;
    svgOpts.totalWidth     = W;
    svgOpts.totalHeight    = H;
    svgOpts.colorbarBounds = barBounds;

    std::string svgStr = scibar::exportToSVG(spec, style, svgOpts, scibar::Orientation::Vertical);
    {
        std::ofstream out(std::string(OUTPUT_DIR) + "/test03_lowlevel_vertical.svg");
        out << svgStr;
    }

    // ── Compare ──
    auto svgBar = extractSvgBarRect(svgStr);
    INFO("SVG bar rect: x=" << svgBar[0] << " y=" << svgBar[1]
         << " w=" << svgBar[2] << " h=" << svgBar[3]);

    CHECK(std::abs(svgBar[0] - barBounds.x) <= 1.0f);
    CHECK(std::abs(svgBar[1] - barBounds.y) <= 1.0f);
    CHECK(std::abs(svgBar[2] - barBounds.width)  <= 1.0f);
    CHECK(std::abs(svgBar[3] - barBounds.height) <= 1.0f);

    // SVG label: (totalWidth/2, cb.y - 10) = (150, 70)
    auto svgLabel = extractSvgLabelPosition(svgStr, spec.label);
    INFO("SVG label: x=" << svgLabel[0] << " y=" << svgLabel[1]);

    // Raster label: drawn centered on labelRect (x=60, w=40, center=80)
    float expectedRasterTitleX = static_cast<float>(labelRect.x) + static_cast<float>(labelRect.width) * 0.5f;
    INFO("Expected raster label X (centered on labelRect): " << expectedRasterTitleX);
    INFO("SVG label X: " << svgLabel[0] << " (expected: " << W/2.0f << ")");
    INFO("Raster labelRect.y: " << labelRect.y << "  SVG label Y: " << svgLabel[1]);

    // Tick label positions: compare SVG text Y positions (major ticks only)
    auto svgTickLabelsY = extractSvgTickLabelY(svgStr, spec.label);
    INFO("SVG tick label Y positions (major ticks): " << svgTickLabelsY.size());
    for (size_t i = 0; i < svgTickLabelsY.size(); ++i) {
        INFO("  tick label " << i << ": y=" << svgTickLabelsY[i]);
    }
    CHECK(svgTickLabelsY.size() >= 3);

    // Verify tick label Y positions match generated tick values
    auto generatedTicks = scibar::generateTicks(spec.scale, 5, style.tickPrecision);
    float range = spec.scale.max - spec.scale.min;
    INFO("Generated " << generatedTicks.size() << " major ticks");
    for (size_t i = 0; i < generatedTicks.size() && i < svgTickLabelsY.size(); ++i) {
        float fraction = (generatedTicks[i].value - spec.scale.min) / range;
        float expectedY = static_cast<float>(barBounds.y + barBounds.height) -
                          fraction * static_cast<float>(barBounds.height);
        INFO("  tick " << generatedTicks[i].value << " → expectedY=" << expectedY
             << " actualY=" << svgTickLabelsY[i]);
        CHECK(svgTickLabelsY[i] == Catch::Approx(expectedY).margin(1.5f));
    }
}

// ── Test 4: Low-level API, Horizontal ────────────────────────────────────

TEST_CASE("raster-vs-vector: low-level horizontal placement", "[raster-vs-vector][horizontal][lowlevel]") {
    ensureOutputDir();

    auto spec  = makeComparisonSpec();
    auto style = makeComparisonStyle();

    const int W = 700, H = 180;
    scibar::Rect barBounds{80, 70, 540, 30};

    // ── Raster ──
    std::vector<uint32_t> rastBuf(static_cast<size_t>(W) * H, WHITE_PIXEL);
    scibar::Canvas rastCanvas{rastBuf.data(), W, H};

    scibar::Rect labelRect{barBounds.x, 20, barBounds.width, 30};
    scibar::Rect actualTitle = scibar::drawLabel(rastCanvas, labelRect, spec.label, style);

    scibar::Rect actualBar = scibar::drawColorBar(rastCanvas, barBounds, spec, style,
                                                   scibar::Orientation::Horizontal);
    scibar::Rect tickBounds = scibar::drawTicks(rastCanvas, barBounds, spec, style,
                                                 scibar::Orientation::Horizontal);
    scibar::drawSubTicks(rastCanvas, barBounds, spec, style,
                          scibar::Orientation::Horizontal);

    scibar::writePPM(rastCanvas, (std::string(OUTPUT_DIR) + "/test04_lowlevel_horizontal.ppm").c_str());

    INFO("Raster bar bounds:   x=" << actualBar.x << " y=" << actualBar.y
         << " w=" << actualBar.width << " h=" << actualBar.height);
    INFO("Raster label bounds: x=" << actualTitle.x << " y=" << actualTitle.y
         << " w=" << actualTitle.width << " h=" << actualTitle.height);
    INFO("Raster tick bounds:  x=" << tickBounds.x << " y=" << tickBounds.y
         << " w=" << tickBounds.width << " h=" << tickBounds.height);

    // ── SVG ──
    scibar::SVGOptions svgOpts;
    svgOpts.totalWidth     = W;
    svgOpts.totalHeight    = H;
    svgOpts.colorbarBounds = barBounds;

    std::string svgStr = scibar::exportToSVG(spec, style, svgOpts,
                                              scibar::Orientation::Horizontal);
    {
        std::ofstream out(std::string(OUTPUT_DIR) + "/test04_lowlevel_horizontal.svg");
        out << svgStr;
    }

    // ── Compare ──
    auto svgBar = extractSvgBarRect(svgStr);
    CHECK(std::abs(svgBar[0] - barBounds.x) <= 1.0f);
    CHECK(std::abs(svgBar[1] - barBounds.y) <= 1.0f);
    CHECK(std::abs(svgBar[2] - barBounds.width)  <= 1.0f);
    CHECK(std::abs(svgBar[3] - barBounds.height) <= 1.0f);

    // Title position: SVG uses (totalWidth/2, cb.y - 15) = (350, 55)
    auto svgLabel = extractSvgLabelPosition(svgStr, spec.label);
    float expectedSvgTitleX = static_cast<float>(W) / 2.0f;
    float expectedSvgTitleY = static_cast<float>(barBounds.y) - 15.0f;
    INFO("SVG label: x=" << svgLabel[0] << " y=" << svgLabel[1]
         << " (expected x=" << expectedSvgTitleX << " y=" << expectedSvgTitleY << ")");
    CHECK(svgLabel[0] == Catch::Approx(expectedSvgTitleX).margin(0.5f));
    CHECK(svgLabel[1] == Catch::Approx(expectedSvgTitleY).margin(0.5f));

    // Raster label was drawn centered on labelRect (x=80, w=540, center=350)
    float expectedRasterTitleX = static_cast<float>(labelRect.x) +
                                 static_cast<float>(labelRect.width) * 0.5f;
    INFO("Expected raster label X (centered on labelRect): " << expectedRasterTitleX);
    INFO("Raster labelRect.y: " << labelRect.y << "  SVG label Y: " << svgLabel[1]);

    // Tick labels: compare SVG text X positions with generated tick values
    auto svgTickLabelsX = extractSvgTickLabelX(svgStr, spec.label);
    INFO("SVG tick label X positions (major ticks): " << svgTickLabelsX.size());
    auto generatedTicks = scibar::generateTicks(spec.scale, 5, style.tickPrecision);
    float range = spec.scale.max - spec.scale.min;
    for (size_t i = 0; i < generatedTicks.size() && i < svgTickLabelsX.size(); ++i) {
        float fraction = (generatedTicks[i].value - spec.scale.min) / range;
        float expectedX = static_cast<float>(barBounds.x) +
                          fraction * static_cast<float>(barBounds.width);
        INFO("  tick " << generatedTicks[i].value << " → expectedX=" << expectedX
             << " actualX=" << svgTickLabelsX[i]);
        CHECK(svgTickLabelsX[i] == Catch::Approx(expectedX).margin(1.5f));
    }
}

TEST_CASE("exportToSVG reversed categorical", "[svg][reverse]") {
    std::vector<scibar::Color> catCmap = {
        scibar::Color{255, 0, 0, 255},
        scibar::Color{0, 255, 0, 255},
        scibar::Color{0, 0, 255, 255},
    };

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Categorical;
    spec.colormap = scibar::ColorMapView(catCmap);

    scibar::Style revStyle = scibar::Style::defaultLight();
    revStyle.reverseColors = true;
    std::string svg = scibar::exportToSVG(spec, revStyle);

    // First rect should be blue (last color in colormap when reversed)
    size_t firstRect = svg.find("<rect");
    REQUIRE(firstRect != std::string::npos);
    // blue = rgb(0,0,255)
    REQUIRE(svg.find("rgb(0,0,255)", firstRect) != std::string::npos);
}

TEST_CASE("generateTicks inverted", "[ticks][inverted]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min   = 0.0f;
    scale.max   = 10.0f;

    auto fwd = scibar::generateTicks(scale, 5, 2);
    REQUIRE(fwd.size() >= 3);
    REQUIRE(fwd.front().value < fwd.back().value);

    scale.inverted = true;
    auto inv = scibar::generateTicks(scale, 5, 2);
    REQUIRE(inv.size() == fwd.size());
    // Inverted ticks should be in descending order
    REQUIRE(inv.front().value > inv.back().value);
}

TEST_CASE("drawColorBar inverted continuous", "[draw][inverted]") {
    auto cmap = testColormap();

    const int W = 200, H = 600;
    auto makeBuf = [&]() {
        std::vector<uint32_t> b(static_cast<size_t>(W) * H, 0);
        return b;
    };

    scibar::Spec fwdSpec;
    fwdSpec.scale.min = 0.0f;
    fwdSpec.scale.max = 100.0f;
    fwdSpec.colormap = scibar::ColorMapView(cmap);

    scibar::Spec invSpec = fwdSpec;
    invSpec.scale.inverted = true;

    scibar::Style style = scibar::Style::defaultDark();
    scibar::Rect bounds{50, 50, 40, 500};

    auto fwdBuf = makeBuf();
    scibar::Canvas fwdCv{fwdBuf.data(), W, H};
    scibar::drawColorBar(fwdCv, bounds, fwdSpec, style);

    auto invBuf = makeBuf();
    scibar::Canvas invCv{invBuf.data(), W, H};
    scibar::drawColorBar(invCv, bounds, invSpec, style);

    int topY = 60, botY = 530, midX = 70;
    // Inverted bar should have colors swapped: top ≠ fwd top, bottom ≠ fwd bottom
    REQUIRE(fwdBuf[topY * W + midX] != invBuf[topY * W + midX]);
    REQUIRE(fwdBuf[botY * W + midX] != invBuf[botY * W + midX]);
}

TEST_CASE("exportToSVG inverted", "[svg][inverted]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.colorbarBounds = {50, 50, 40, 500};
    scibar::Style style = scibar::Style::defaultLight();

    std::string fwdSvg = scibar::exportToSVG(spec, style, opts);

    spec.scale.inverted = true;
    std::string invSvg = scibar::exportToSVG(spec, style, opts);

    // Forward has y1="1" (gradient starts at bottom), inverted has y1="0" (starts at top)
    REQUIRE(fwdSvg.find("y1=\"1\"") != std::string::npos);
    REQUIRE(invSvg.find("y1=\"0\"") != std::string::npos);
}

TEST_CASE("inverted plus reverseColors cancel on continuous", "[draw][inverted]") {
    auto cmap = testColormap();

    const int W = 200, H = 600;
    auto makeBuf = [&]() {
        std::vector<uint32_t> b(static_cast<size_t>(W) * H, 0);
        return b;
    };

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Rect bounds{50, 50, 40, 500};

    // Default: neither flag set
    scibar::Style defStyle = scibar::Style::defaultDark();
    auto defBuf = makeBuf();
    scibar::Canvas defCv{defBuf.data(), W, H};
    scibar::drawColorBar(defCv, bounds, spec, defStyle);

    // Both: inverted + reverseColors — should cancel on continuous scales
    spec.scale.inverted = true;
    scibar::Style bothStyle = scibar::Style::defaultDark();
    bothStyle.reverseColors = true;
    auto bothBuf = makeBuf();
    scibar::Canvas bothCv{bothBuf.data(), W, H};
    scibar::drawColorBar(bothCv, bounds, spec, bothStyle);

    // Continuous: inverted + reverseColors produces same gradient direction as default
    int midX = 70;
    int topY = 60, midY = 325, botY = 530;
    REQUIRE(defBuf[topY * W + midX] == bothBuf[topY * W + midX]);
    REQUIRE(defBuf[midY * W + midX] == bothBuf[midY * W + midX]);
    REQUIRE(defBuf[botY * W + midX] == bothBuf[botY * W + midX]);
}

TEST_CASE("sampleColormap basic", "[util]") {
    // Build a simple 4-entry colormap: black, red, green, white
    std::vector<scibar::Color> entries = {
        scibar::Color{0, 0, 0, 255},       // 0: black
        scibar::Color{255, 0, 0, 255},     // 1: red
        scibar::Color{0, 255, 0, 255},     // 2: green
        scibar::Color{255, 255, 255, 255}   // 3: white
    };
    scibar::ColorMapView cmap(entries);

    // t = 0.0 → first entry (black)
    auto c0 = scibar::util::sampleColormap(cmap, 0.0f);
    REQUIRE(c0.r == 0);
    REQUIRE(c0.g == 0);
    REQUIRE(c0.b == 0);

    // t = 1.0 → last entry (white)
    auto c1 = scibar::util::sampleColormap(cmap, 1.0f);
    REQUIRE(c1.r == 255);
    REQUIRE(c1.g == 255);
    REQUIRE(c1.b == 255);

    // t = 1.0/3.0 → exactly at index 1 (red)
    auto cRed = scibar::util::sampleColormap(cmap, 1.0f / 3.0f);
    REQUIRE(cRed.r == 255);
    REQUIRE(cRed.g == 0);
    REQUIRE(cRed.b == 0);

    // t = 2.0/3.0 → exactly at index 2 (green)
    auto cGreen = scibar::util::sampleColormap(cmap, 2.0f / 3.0f);
    REQUIRE(cGreen.r == 0);
    REQUIRE(cGreen.g == 255);
    REQUIRE(cGreen.b == 0);

    // t = 0.5 → halfway between red (1) and green (2): (128, 128, 0)
    auto cMid = scibar::util::sampleColormap(cmap, 0.5f);
    REQUIRE(cMid.r == 128);
    REQUIRE(cMid.g == 128);
    REQUIRE(cMid.b == 0);
}

TEST_CASE("sampleColormap clamping", "[util]") {
    std::vector<scibar::Color> entries = {
        scibar::Color{10, 20, 30, 255},
        scibar::Color{100, 200, 250, 255}
    };
    scibar::ColorMapView cmap(entries);

    // t < 0 → clamped to first entry
    auto cNeg = scibar::util::sampleColormap(cmap, -0.5f);
    REQUIRE(cNeg.r == 10);
    REQUIRE(cNeg.g == 20);
    REQUIRE(cNeg.b == 30);

    // t > 1 → clamped to last entry
    auto cOver = scibar::util::sampleColormap(cmap, 2.0f);
    REQUIRE(cOver.r == 100);
    REQUIRE(cOver.g == 200);
    REQUIRE(cOver.b == 250);
}

TEST_CASE("sampleColormap edge cases", "[util]") {
    // Empty colormap → returns black
    scibar::ColorMapView emptyMap;
    auto cEmpty = scibar::util::sampleColormap(emptyMap, 0.5f);
    REQUIRE(cEmpty.r == 0);
    REQUIRE(cEmpty.g == 0);
    REQUIRE(cEmpty.b == 0);
    REQUIRE(cEmpty.a == 255);

    // Single-entry colormap → always returns that color
    std::vector<scibar::Color> single = {scibar::Color{42, 99, 200, 255}};
    scibar::ColorMapView singleMap(single);
    auto cS0 = scibar::util::sampleColormap(singleMap, 0.0f);
    auto cS1 = scibar::util::sampleColormap(singleMap, 1.0f);
    auto cS5 = scibar::util::sampleColormap(singleMap, 0.5f);
    REQUIRE(cS0.r == 42); REQUIRE(cS0.g == 99); REQUIRE(cS0.b == 200);
    REQUIRE(cS1.r == 42); REQUIRE(cS1.g == 99); REQUIRE(cS1.b == 200);
    REQUIRE(cS5.r == 42); REQUIRE(cS5.g == 99); REQUIRE(cS5.b == 200);
}

TEST_CASE("sampleColormap with viridis", "[util]") {
    auto viridis = scibar::util::viridis();

    // t=0 should match first viridis entry (dark purple)
    auto cFirst = scibar::util::sampleColormap(viridis, 0.0f);
    REQUIRE(cFirst.r < 80);
    REQUIRE(cFirst.g < 10);
    REQUIRE(cFirst.b > 80);

    // t=1 should match last viridis entry (bright yellow)
    auto cLast = scibar::util::sampleColormap(viridis, 1.0f);
    REQUIRE(cLast.r > 230);
    REQUIRE(cLast.g > 220);
    REQUIRE(cLast.b < 60);

    // All samples should be fully opaque
    for (float t = 0.0f; t <= 1.0f; t += 0.1f) {
        auto c = scibar::util::sampleColormap(viridis, t);
        REQUIRE(c.a == 255);
    }
}
