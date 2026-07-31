// scibar implementation — development phase
// Will be folded into scibar.hpp under #ifdef SCIBAR_IMPLEMENTATION later.

#define SCIBAR_IMPLEMENTATION

// Enable implementations for vendored single-header libraries
#define CANVAS_ITY_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "scibar.hpp"
#include "../third_party/canvas_ity.hpp"
#include "../third_party/stb_truetype.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace scibar {

// =========================================================================
// Internal: font storage
// =========================================================================

struct FontEntry {
    std::vector<uint8_t> ttfData;   // Raw TTF bytes (for canvas_ity)
    stbtt_fontinfo       stbInfo;   // Parsed font (for metrics)
};

static std::unordered_map<const void*, FontEntry> s_fonts;

// =========================================================================
// Style defaults
// =========================================================================

Style Style::defaultLight() {
    Style s;
    s.frameColor = Color::fromHex(0x000000FF);
    s.tickColor  = Color::fromHex(0x000000FF);
    s.textColor  = Color::fromHex(0x000000FF);
    return s;
}

Style Style::defaultDark() {
    Style s;
    s.frameColor = Color::fromHex(0xFFFFFFFF);
    s.tickColor  = Color::fromHex(0xFFFFFFFF);
    s.textColor  = Color::fromHex(0xFFFFFFFF);
    return s;
}

// =========================================================================
// Font loading
// =========================================================================

Font loadFont(const char* ttfFilePath, float size) {
    assert(ttfFilePath && "ttfFilePath must not be null");

    // Read file
    FILE* f = fopen(ttfFilePath, "rb");
    assert(f && "Failed to open font file");

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    assert(fileSize > 0 && "Font file is empty");

    std::vector<uint8_t> ttfData(static_cast<size_t>(fileSize));
    size_t read = fread(ttfData.data(), 1, static_cast<size_t>(fileSize), f);
    fclose(f);
    assert(read == static_cast<size_t>(fileSize) && "Failed to read font file");

    // Insert into static storage first, then get a stable handle
    FontEntry entry;
    entry.ttfData = std::move(ttfData);

    int offset = stbtt_GetFontOffsetForIndex(entry.ttfData.data(), 0);
    assert(offset >= 0 && "Failed to find font data offset");

    int ok = stbtt_InitFont(&entry.stbInfo, entry.ttfData.data(), offset);
    assert(ok && "Failed to initialize font");

    // Use a unique key — address of the FontEntry in the static map is stable
    static int fontIdCounter = 0;
    const void* key = reinterpret_cast<const void*>(static_cast<intptr_t>(++fontIdCounter));
    const void* handle = &s_fonts[key].stbInfo; // placeholder, will be overwritten after insert
    s_fonts[key] = std::move(entry);
    handle = &s_fonts[key].stbInfo; // now stable

    Font font;
    font.handle = handle;
    font.size   = size;
    return font;
}

// =========================================================================
// Internal: UTF-8 decoding
// =========================================================================

// Decode the next UTF-8 codepoint from a string. Returns the codepoint
// and advances *pos past the consumed bytes. Returns -1 on end or error.
static int decodeUtf8(const std::string& text, size_t* pos) {
    if (*pos >= text.size()) return -1;

    unsigned char c = static_cast<unsigned char>(text[*pos]);

    if (c < 0x80) {
        (*pos)++;
        return c;
    }

    int len = 0;
    int codepoint = 0;
    if ((c & 0xE0) == 0xC0)       { len = 2; codepoint = c & 0x1F; }
    else if ((c & 0xF0) == 0xE0)  { len = 3; codepoint = c & 0x0F; }
    else if ((c & 0xF8) == 0xF0)  { len = 4; codepoint = c & 0x07; }
    else { (*pos)++; return 0xFFFD; } // Invalid, skip

    for (int j = 1; j < len; ++j) {
        if (*pos + j >= text.size()) { (*pos)++; return 0xFFFD; }
        unsigned char next = static_cast<unsigned char>(text[*pos + j]);
        if ((next & 0xC0) != 0x80) { (*pos)++; return 0xFFFD; }
        codepoint = (codepoint << 6) | (next & 0x3F);
    }

    *pos += len;
    return codepoint;
}

// =========================================================================
// Text measurement utilities (stb_truetype backend)
// =========================================================================

static float fontScale(const Font& font) {
    const auto* info = static_cast<const stbtt_fontinfo*>(font.handle);
    return stbtt_ScaleForPixelHeight(info, font.size);
}

FontMetrics fontMetrics(const Font& font) {
    assert(font.handle && "Font handle is null — loadFont() not called or embedded font not available");

    const auto* info = static_cast<const stbtt_fontinfo*>(font.handle);
    float scale = fontScale(font);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(info, &ascent, &descent, &lineGap);

    FontMetrics m;
    m.ascender   = static_cast<float>(ascent)  * scale;
    m.descender  = static_cast<float>(descent) * scale;
    m.lineHeight = static_cast<float>(ascent - descent + lineGap) * scale;
    return m;
}

std::array<float, 2> measureText(const std::string& text, const Font& font) {
    assert(font.handle && "Font handle is null");

    const auto* info = static_cast<const stbtt_fontinfo*>(font.handle);
    float scale = fontScale(font);

    float width = 0.0f;
    int prevCodepoint = 0;
    size_t pos = 0;
    while (true) {
        int codepoint = decodeUtf8(text, &pos);
        if (codepoint < 0) break;

        int ax;
        stbtt_GetCodepointHMetrics(info, codepoint, &ax, nullptr);

        if (prevCodepoint > 0 && codepoint > 0) {
            width += static_cast<float>(stbtt_GetCodepointKernAdvance(info, prevCodepoint, codepoint)) * scale;
        }

        width += static_cast<float>(ax) * scale;
        prevCodepoint = codepoint;
    }

    FontMetrics m = fontMetrics(font);
    return { width, m.lineHeight };
}

float textAdvance(const Font& font, const std::string& text, int upToIndex) {
    assert(font.handle && "Font handle is null");
    assert(upToIndex >= 0 && static_cast<size_t>(upToIndex) <= text.size() && "upToIndex out of range");

    const auto* info = static_cast<const stbtt_fontinfo*>(font.handle);
    float scale = fontScale(font);

    float width = 0.0f;
    int prevCodepoint = 0;
    size_t pos = 0;
    while (pos < static_cast<size_t>(upToIndex)) {
        int codepoint = decodeUtf8(text, &pos);
        if (codepoint < 0) break;

        int ax;
        stbtt_GetCodepointHMetrics(info, codepoint, &ax, nullptr);

        if (prevCodepoint > 0 && codepoint > 0) {
            width += static_cast<float>(stbtt_GetCodepointKernAdvance(info, prevCodepoint, codepoint)) * scale;
        }

        width += static_cast<float>(ax) * scale;
        prevCodepoint = codepoint;
    }

    return width;
}

float codepointAdvance(const Font& font, int leftCodepoint, int rightCodepoint) {
    assert(font.handle && "Font handle is null");

    const auto* info = static_cast<const stbtt_fontinfo*>(font.handle);
    float scale = fontScale(font);

    int ax;
    stbtt_GetCodepointHMetrics(info, rightCodepoint, &ax, nullptr);

    float adv = static_cast<float>(ax) * scale;

    if (leftCodepoint > 0 && rightCodepoint > 0) {
        adv += static_cast<float>(stbtt_GetCodepointKernAdvance(info, leftCodepoint, rightCodepoint)) * scale;
    }

    return adv;
}

// =========================================================================
// Tick generation (nice-numbers algorithm)
// =========================================================================

std::vector<Tick> generateTicks(const Scale& scale, int targetCount, int precision) {
    assert(targetCount >= 2 && "targetCount must be at least 2");
    assert(scale.min < scale.max && "scale.min must be less than scale.max");
    assert(precision >= 0 && "precision must be non-negative");

    std::vector<Tick> ticks;

    if (scale.type == ScaleType::Logarithmic) {
        assert(scale.min > 0.0f && "Logarithmic scale requires min > 0");

        float logMin = std::log10(scale.min);
        float logMax = std::log10(scale.max);

        int firstPow = static_cast<int>(std::floor(logMin));
        int lastPow  = static_cast<int>(std::ceil(logMax));

        char buf[64];
        for (int p = firstPow; p <= lastPow; ++p) {
            float value = std::pow(10.0f, static_cast<float>(p));
            if (value < scale.min || value > scale.max) continue;

            Tick t;
            t.value = value;
            snprintf(buf, sizeof(buf), "%.*g", precision, static_cast<double>(value));
            t.label = buf;
            ticks.push_back(std::move(t));
        }
        return ticks;
    }

    // Linear / Categorical: compute nice step size
    float range = scale.max - scale.min;
    float roughStep = range / static_cast<float>(targetCount - 1);

    // Snap to a "nice" step: 1, 2, 5, 10, 20, 50, ...
    float exponent = std::floor(std::log10(roughStep));
    float fraction = roughStep / std::pow(10.0f, exponent);

    float niceFraction;
    if (fraction <= 1.0f)       niceFraction = 1.0f;
    else if (fraction <= 2.0f)  niceFraction = 2.0f;
    else if (fraction <= 5.0f)  niceFraction = 5.0f;
    else                        niceFraction = 10.0f;

    float niceStep = niceFraction * std::pow(10.0f, exponent);

    float start = std::ceil(scale.min / niceStep) * niceStep;
    float end   = scale.max;

    char buf[64];
    for (float value = start; value <= end + niceStep * 0.5f; value += niceStep) {
        Tick t;
        t.value = value;
        snprintf(buf, sizeof(buf), "%.*g", precision, static_cast<double>(value));
        t.label = buf;
        ticks.push_back(std::move(t));
    }

    // For diverging scales, add midpoint tick if it falls between nice tick positions
    if (scale.type == ScaleType::Diverging) {
        // Check if midpoint is already covered by a tick
        bool hasMidpoint = false;
        for (const auto& t : ticks) {
            if (std::abs(t.value - scale.midpoint) < niceStep * 0.01f) {
                hasMidpoint = true;
                break;
            }
        }
        if (!hasMidpoint && scale.midpoint > scale.min && scale.midpoint < scale.max) {
            Tick t;
            t.value = scale.midpoint;
            snprintf(buf, sizeof(buf), "%.*g", precision, static_cast<double>(scale.midpoint));
            t.label = buf;
            ticks.push_back(std::move(t));
            // Re-sort by value
            std::sort(ticks.begin(), ticks.end(),
                      [](const Tick& a, const Tick& b) { return a.value < b.value; });
        }
    }

    if (scale.inverted) {
        std::reverse(ticks.begin(), ticks.end());
    }

    return ticks;
}

// =========================================================================
// Sub-tick generation
// =========================================================================

std::vector<SubTick> generateSubTicks(const Scale& scale,
                                       const std::vector<Tick>& majorTicks,
                                       int subTicksPerInterval) {
    assert(subTicksPerInterval >= 1 && "subTicksPerInterval must be at least 1");

    std::vector<SubTick> subTicks;

    // Categorical scales: no sub-ticks — categories are discrete blocks.
    if (scale.type == ScaleType::Categorical) return subTicks;
    if (majorTicks.size() < 2) return subTicks;

    if (scale.type == ScaleType::Logarithmic) {
        // Logarithmic sub-ticks: fixed at 2,3,4,5,6,7,8,9 × 10ⁿ between decade boundaries.
        // subTicksPerInterval is ignored — log sub-ticks have an inherent structure.
        for (size_t i = 0; i + 1 < majorTicks.size(); ++i) {
            float low  = majorTicks[i].value;
            float high = majorTicks[i + 1].value;
            if (low > high) std::swap(low, high);

            // Find the decade boundary: for log scales, major ticks are at powers of 10.
            // Sub-ticks go at 2×10^p, 3×10^p, ..., 9×10^p between 10^p and 10^(p+1).
            float decadeLow = std::pow(10.0f, std::floor(std::log10(low)));
            while (decadeLow < high) {
                float decadeHigh = decadeLow * 10.0f;
                for (int k = 2; k <= 9; ++k) {
                    float val = decadeLow * static_cast<float>(k);
                    if (val > low && val < high && val > scale.min && val < scale.max) {
                        subTicks.push_back({val});
                    }
                }
                decadeLow = decadeHigh;
            }
        }
    } else {
        // Linear / Diverging: evenly-spaced sub-ticks between each pair of adjacent major ticks.
        for (size_t i = 0; i + 1 < majorTicks.size(); ++i) {
            float low  = majorTicks[i].value;
            float high = majorTicks[i + 1].value;
            if (low > high) std::swap(low, high);

            float interval = (high - low) / static_cast<float>(subTicksPerInterval + 1);
            for (int k = 1; k <= subTicksPerInterval; ++k) {
                float val = low + interval * static_cast<float>(k);
                if (val > scale.min && val < scale.max) {
                    subTicks.push_back({val});
                }
            }
        }
    }

    // Sort by value for consistent ordering.
    std::sort(subTicks.begin(), subTicks.end(),
              [](const SubTick& a, const SubTick& b) { return a.value < b.value; });

    if (scale.inverted) {
        std::reverse(subTicks.begin(), subTicks.end());
    }

    return subTicks;
}

// =========================================================================
// Internal: find font entry from handle
// =========================================================================

static const FontEntry* findFontEntry(const void* handle) {
    for (const auto& [key, entry] : s_fonts) {
        if (&entry.stbInfo == handle) return &entry;
    }
    return nullptr;
}

// =========================================================================
// Internal: scibar Color → canvas_ity float color
// =========================================================================

static void setCanvasColor(canvas_ity::canvas& cv, canvas_ity::brush_type type, const Color& c) {
    cv.set_color(type, c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
}

// =========================================================================
// Internal: sync scibar Canvas ↔ canvas_ity canvas
// =========================================================================

static void loadCanvasToCV(canvas_ity::canvas& cv, const Canvas& canvas) {
    cv.put_image_data(reinterpret_cast<const unsigned char*>(canvas.pixels),
                      canvas.width, canvas.height, canvas.width * 4, 0, 0);
}

static void storeCVToCanvas(Canvas& canvas, canvas_ity::canvas& cv) {
    std::vector<unsigned char> row(static_cast<size_t>(canvas.width) * 4);
    for (int y = 0; y < canvas.height; ++y) {
        cv.get_image_data(row.data(), canvas.width, 1, canvas.width * 4, 0, y);
        for (int x = 0; x < canvas.width; ++x) {
            unsigned char* src = row.data() + x * 4;
            canvas.pixels[y * canvas.width + x] =
                (static_cast<uint32_t>(src[3]) << 24) |  // A
                (static_cast<uint32_t>(src[2]) << 16) |  // B
                (static_cast<uint32_t>(src[1]) << 8)  |  // G
                (static_cast<uint32_t>(src[0]));         // R
        }
    }
}

// =========================================================================
// Pixel drawing: fillCanvas
// =========================================================================

void fillCanvas(Canvas& canvas, Color color) {
    assert(canvas.pixels && "Canvas pixels must not be null");
    uint32_t packed = (static_cast<uint32_t>(color.a) << 24) |
                      (static_cast<uint32_t>(color.b) << 16) |
                      (static_cast<uint32_t>(color.g) << 8)  |
                      (static_cast<uint32_t>(color.r));
    size_t count = static_cast<size_t>(canvas.width) * static_cast<size_t>(canvas.height);
    for (size_t i = 0; i < count; ++i) {
        canvas.pixels[i] = packed;
    }
}

// =========================================================================
// Pixel drawing: drawColorBar
// =========================================================================

Rect drawColorBar(Canvas& canvas, Rect bounds, const Spec& spec, const Style& style,
                  Orientation orientation) {
    assert(canvas.pixels && "Canvas pixels must not be null");
    assert(bounds.width > 0 && bounds.height > 0 && "Bounds must have positive dimensions");
    assert(spec.colormap.data && spec.colormap.size > 0 && "Colormap must not be empty");

    canvas_ity::canvas cv(canvas.width, canvas.height);
    loadCanvasToCV(cv, canvas);
    bool isVertical = (orientation == Orientation::Vertical);

    if (spec.scale.type == ScaleType::Categorical && spec.colormap.size > 0) {
        bool flipIdx = (spec.scale.inverted != style.reverseColors);
        if (isVertical) {
            float blockH = static_cast<float>(bounds.height) / static_cast<float>(spec.colormap.size);
            for (size_t i = 0; i < spec.colormap.size; ++i) {
                size_t idx = flipIdx ? (spec.colormap.size - 1 - i) : i;
                const Color& c = spec.colormap.data[idx];
                float y0 = static_cast<float>(bounds.y) + static_cast<float>(i) * blockH;
                float y1 = y0 + blockH + 0.5f;
                setCanvasColor(cv, canvas_ity::fill_style, c);
                cv.fill_rectangle(static_cast<float>(bounds.x), y0,
                                  static_cast<float>(bounds.width), y1 - y0);
            }
        } else {
            float blockW = static_cast<float>(bounds.width) / static_cast<float>(spec.colormap.size);
            for (size_t i = 0; i < spec.colormap.size; ++i) {
                size_t idx = flipIdx ? (spec.colormap.size - 1 - i) : i;
                const Color& c = spec.colormap.data[idx];
                float x0 = static_cast<float>(bounds.x) + static_cast<float>(i) * blockW;
                float x1 = x0 + blockW + 0.5f;
                setCanvasColor(cv, canvas_ity::fill_style, c);
                cv.fill_rectangle(x0, static_cast<float>(bounds.y),
                                  x1 - x0, static_cast<float>(bounds.height));
            }
        }
    } else {
        bool gradientFlipped = (spec.scale.inverted != style.reverseColors);
        if (isVertical) {
            if (gradientFlipped) {
                cv.set_linear_gradient(canvas_ity::fill_style,
                                       static_cast<float>(bounds.x),
                                       static_cast<float>(bounds.y),
                                       static_cast<float>(bounds.x),
                                       static_cast<float>(bounds.y + bounds.height));
            } else {
                cv.set_linear_gradient(canvas_ity::fill_style,
                                       static_cast<float>(bounds.x),
                                       static_cast<float>(bounds.y + bounds.height),
                                       static_cast<float>(bounds.x),
                                       static_cast<float>(bounds.y));
            }
        } else {
            if (gradientFlipped) {
                cv.set_linear_gradient(canvas_ity::fill_style,
                                       static_cast<float>(bounds.x + bounds.width),
                                       static_cast<float>(bounds.y),
                                       static_cast<float>(bounds.x),
                                       static_cast<float>(bounds.y));
            } else {
                cv.set_linear_gradient(canvas_ity::fill_style,
                                       static_cast<float>(bounds.x),
                                       static_cast<float>(bounds.y),
                                       static_cast<float>(bounds.x + bounds.width),
                                       static_cast<float>(bounds.y));
            }
        }

        for (size_t i = 0; i < spec.colormap.size; ++i) {
            const Color& c = spec.colormap.data[i];
            float offset = (spec.colormap.size > 1)
                ? static_cast<float>(i) / static_cast<float>(spec.colormap.size - 1)
                : 0.5f;

            cv.add_color_stop(canvas_ity::fill_style, offset,
                              c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f);
        }

        cv.fill_rectangle(static_cast<float>(bounds.x), static_cast<float>(bounds.y),
                          static_cast<float>(bounds.width), static_cast<float>(bounds.height));
    }

    // Draw frame if enabled
    if (style.showFrame) {
        setCanvasColor(cv, canvas_ity::stroke_style, style.frameColor);
        cv.set_line_width(1.0f);
        cv.stroke_rectangle(static_cast<float>(bounds.x), static_cast<float>(bounds.y),
                            static_cast<float>(bounds.width), static_cast<float>(bounds.height));
    }

    storeCVToCanvas(canvas, cv);
    return bounds;
}

// =========================================================================
// Pixel drawing: drawTicks
// =========================================================================

Rect drawTicks(Canvas& canvas, Rect barBounds, const Spec& spec, const Style& style,
               Orientation orientation) {
    assert(canvas.pixels && "Canvas pixels must not be null");
    assert(barBounds.width > 0 && barBounds.height > 0 && "Bar bounds must have positive dimensions");

    // Auto-generate ticks if not provided
    const std::vector<Tick>* ticks = &spec.ticks;
    std::vector<Tick> generated;
    if (ticks->empty()) {
        generated = generateTicks(spec.scale, 5, style.tickPrecision);
        ticks = &generated;
    }

    if (ticks->empty()) return barBounds;

    canvas_ity::canvas cv(canvas.width, canvas.height);
    loadCanvasToCV(cv, canvas);

    // Set font for labels
    const Font* font = &style.font;
    const FontEntry* entry = findFontEntry(font->handle);
    if (entry && !entry->ttfData.empty()) {
        cv.set_font(entry->ttfData.data(), static_cast<int>(entry->ttfData.size()), font->size);
    }

    float range = spec.scale.max - spec.scale.min;
    assert(range > 0.0f && "Scale range must be positive");

    Rect totalBounds = barBounds;
    bool isVertical = (orientation == Orientation::Vertical);

    for (const auto& tick : *ticks) {
        float fraction = 0.0f;

        if (spec.scale.type == ScaleType::Logarithmic) {
            assert(spec.scale.min > 0.0f && tick.value > 0.0f);
            float logMin = std::log10(spec.scale.min);
            float logMax = std::log10(spec.scale.max);
            float logVal = std::log10(tick.value);
            fraction = (logVal - logMin) / (logMax - logMin);
        } else {
            fraction = (tick.value - spec.scale.min) / range;
        }

        if (spec.scale.inverted) fraction = 1.0f - fraction;

        setCanvasColor(cv, canvas_ity::stroke_style, style.tickColor);
        cv.set_line_width(1.0f);

        if (isVertical) {
            // Vertical: ticks to the right, labels left-aligned
            float y = static_cast<float>(barBounds.y + barBounds.height) -
                      fraction * static_cast<float>(barBounds.height);

            cv.move_to(static_cast<float>(barBounds.x + barBounds.width), y);
            cv.line_to(static_cast<float>(barBounds.x + barBounds.width) + style.tickLength, y);
            cv.stroke();

            setCanvasColor(cv, canvas_ity::fill_style, style.textColor);
            cv.text_align = canvas_ity::leftward;
            cv.text_baseline = canvas_ity::alphabetic;

            if (entry && !entry->ttfData.empty()) {
                cv.fill_text(tick.label.c_str(),
                             static_cast<float>(barBounds.x + barBounds.width) + style.tickLength + 3.0f,
                             y);
            }

            int rightExtent = barBounds.x + barBounds.width +
                              static_cast<int>(style.tickLength) + 100;
            if (rightExtent > totalBounds.x + totalBounds.width) {
                totalBounds.width = rightExtent - totalBounds.x;
            }
        } else {
            // Horizontal: ticks below, labels centered
            float x = static_cast<float>(barBounds.x) +
                      fraction * static_cast<float>(barBounds.width);

            cv.move_to(x, static_cast<float>(barBounds.y + barBounds.height));
            cv.line_to(x, static_cast<float>(barBounds.y + barBounds.height) + style.tickLength);
            cv.stroke();

            setCanvasColor(cv, canvas_ity::fill_style, style.textColor);
            cv.text_align = canvas_ity::center;
            cv.text_baseline = canvas_ity::top;

            if (entry && !entry->ttfData.empty()) {
                cv.fill_text(tick.label.c_str(), x,
                             static_cast<float>(barBounds.y + barBounds.height) + style.tickLength + 3.0f);
            }

            int bottomExtent = barBounds.y + barBounds.height +
                               static_cast<int>(style.tickLength) + 20;
            if (bottomExtent > totalBounds.y + totalBounds.height) {
                totalBounds.height = bottomExtent - totalBounds.y;
            }
        }
    }

    storeCVToCanvas(canvas, cv);
    return totalBounds;
}

// =========================================================================
// Pixel drawing: drawSubTicks
// =========================================================================

Rect drawSubTicks(Canvas& canvas, Rect barBounds, const Spec& spec, const Style& style,
                  Orientation orientation) {
    assert(canvas.pixels && "Canvas pixels must not be null");
    assert(barBounds.width > 0 && barBounds.height > 0 && "Bar bounds must have positive dimensions");

    if (!style.showSubTicks) return barBounds;

    // Resolve major ticks for auto-generation reference.
    const std::vector<Tick>* majorTicks = &spec.ticks;
    std::vector<Tick> generatedMajor;
    if (majorTicks->empty()) {
        generatedMajor = generateTicks(spec.scale, 5, style.tickPrecision);
        majorTicks = &generatedMajor;
    }

    // Auto-generate sub-ticks if not provided.
    const std::vector<SubTick>* subTicks = &spec.subTicks;
    std::vector<SubTick> generated;
    if (subTicks->empty()) {
        generated = generateSubTicks(spec.scale, *majorTicks, style.subTicksPerInterval);
        subTicks = &generated;
    }

    if (subTicks->empty()) return barBounds;

    canvas_ity::canvas cv(canvas.width, canvas.height);
    loadCanvasToCV(cv, canvas);

    float range = spec.scale.max - spec.scale.min;
    assert(range > 0.0f && "Scale range must be positive");

    bool isVertical = (orientation == Orientation::Vertical);

    setCanvasColor(cv, canvas_ity::stroke_style, style.tickColor);
    cv.set_line_width(1.0f);

    for (const auto& st : *subTicks) {
        float fraction = 0.0f;

        if (spec.scale.type == ScaleType::Logarithmic) {
            assert(spec.scale.min > 0.0f && st.value > 0.0f);
            float logMin = std::log10(spec.scale.min);
            float logMax = std::log10(spec.scale.max);
            float logVal = std::log10(st.value);
            fraction = (logVal - logMin) / (logMax - logMin);
        } else {
            fraction = (st.value - spec.scale.min) / range;
        }

        if (spec.scale.inverted) fraction = 1.0f - fraction;

        if (isVertical) {
            float y = static_cast<float>(barBounds.y + barBounds.height) -
                      fraction * static_cast<float>(barBounds.height);
            cv.move_to(static_cast<float>(barBounds.x + barBounds.width), y);
            cv.line_to(static_cast<float>(barBounds.x + barBounds.width) + style.subTickLength, y);
            cv.stroke();
        } else {
            float x = static_cast<float>(barBounds.x) +
                      fraction * static_cast<float>(barBounds.width);
            cv.move_to(x, static_cast<float>(barBounds.y + barBounds.height));
            cv.line_to(x, static_cast<float>(barBounds.y + barBounds.height) + style.subTickLength);
            cv.stroke();
        }
    }

    storeCVToCanvas(canvas, cv);
    return barBounds; // Sub-ticks are shorter than major ticks — bounding box unchanged.
}

// =========================================================================
// Pixel drawing: drawTitle
// =========================================================================

Rect drawTitle(Canvas& canvas, Rect bounds, const std::string& title, const Style& style) {
    assert(canvas.pixels && "Canvas pixels must not be null");

    if (title.empty()) return bounds;

    canvas_ity::canvas cv(canvas.width, canvas.height);
    loadCanvasToCV(cv, canvas);

    const FontEntry* entry = findFontEntry(style.font.handle);
    if (!entry || entry->ttfData.empty()) return bounds;

    cv.set_font(entry->ttfData.data(), static_cast<int>(entry->ttfData.size()), style.font.size);

    setCanvasColor(cv, canvas_ity::fill_style, style.textColor);
    cv.text_align = canvas_ity::center;
    cv.text_baseline = canvas_ity::top;

    float textX = static_cast<float>(bounds.x) + static_cast<float>(bounds.width) * 0.5f;
    float textY = static_cast<float>(bounds.y);
    cv.fill_text(title.c_str(), textX, textY);

    // Measure actual text width for accurate bounds
    float textWidth = cv.measure_text(title.c_str());
    int actualWidth = static_cast<int>(std::ceil(textWidth));
    FontMetrics fm = fontMetrics(style.font);
    int actualHeight = static_cast<int>(std::ceil(fm.lineHeight));

    Rect result;
    result.x = static_cast<int>(textX) - actualWidth / 2;
    result.y = bounds.y;
    result.width = actualWidth;
    result.height = actualHeight;

    storeCVToCanvas(canvas, cv);
    return result;
}

// =========================================================================
// High-level convenience: drawLegend
// =========================================================================

LayoutResult drawLegend(Canvas& canvas, const Spec& spec, const Style& style) {
    assert(canvas.pixels && "Canvas pixels must not be null");
    assert(canvas.width > 0 && canvas.height > 0 && "Canvas must have positive dimensions");
    assert(spec.colormap.data && spec.colormap.size > 0 && "Colormap must not be empty");

    // Hardcoded layout for a vertical colorbar filling the canvas
    // with rough defaults. For publication figures, use the low-level API.

    int marginX = canvas.width / 10;
    int titleHeight = 40;
    int barWidth = canvas.width / 6;
    int barHeight = canvas.height - titleHeight - marginX;
    int barX = marginX;
    int barY = titleHeight;

    LayoutResult result;

    // Title — centered above the colorbar, not the entire canvas
    Rect titleRect{barX, 0, barWidth, titleHeight};
    Rect actualTitle = drawTitle(canvas, titleRect, spec.title, style);

    // Color bar
    Rect barRect{barX, barY, barWidth, barHeight};
    result.colorbarBoundingBox = drawColorBar(canvas, barRect, spec, style);

    // Ticks
    auto generatedTicks = generateTicks(spec.scale, 5, style.tickPrecision);
    Spec specWithTicks = spec;
    if (spec.ticks.empty()) {
        specWithTicks.ticks = generatedTicks;
    }
    result.generatedTickCount = static_cast<int>(specWithTicks.ticks.size());
    Rect tickBounds = drawTicks(canvas, barRect, specWithTicks, style);

    // Sub-ticks
    drawSubTicks(canvas, barRect, specWithTicks, style);

    // Total bounds
    result.totalBoundingBox = unionRect(unionRect(actualTitle, barRect), tickBounds);
    return result;
}

// =========================================================================
// SVG export
// =========================================================================

std::string exportToSVG(const Spec& spec, const Style& style, const SVGOptions& options,
                        Orientation orientation) {
    assert(spec.colormap.data && spec.colormap.size > 0 && "Colormap must not be empty");
    assert(options.totalWidth > 0 && options.totalHeight > 0 && "SVG dimensions must be positive");

    const Rect& cb = options.colorbarBounds;
    bool isVertical = (orientation == Orientation::Vertical);

    std::ostringstream svg;
    svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
        << "width=\"" << options.totalWidth << "\" "
        << "height=\"" << options.totalHeight << "\" "
        << "viewBox=\"0 0 " << options.totalWidth << " " << options.totalHeight << "\">\n";

    // Embedded raster image (hybrid figure)
    if (!options.mainImageHref.empty()) {
        svg << "  <image href=\"" << options.mainImageHref << "\" "
            << "x=\"" << options.mainImageBounds.x << "\" "
            << "y=\"" << options.mainImageBounds.y << "\" "
            << "width=\"" << options.mainImageBounds.width << "\" "
            << "height=\"" << options.mainImageBounds.height << "\" "
            << "image-rendering=\"crisp-edges\" "
            << "preserveAspectRatio=\"none\" />\n";
    }

    // Colormap
    if (spec.scale.type == ScaleType::Categorical) {
        bool flipIdx = (spec.scale.inverted != style.reverseColors);
        if (isVertical) {
            float blockH = static_cast<float>(cb.height) / static_cast<float>(spec.colormap.size);
            for (size_t i = 0; i < spec.colormap.size; ++i) {
                size_t idx = flipIdx ? (spec.colormap.size - 1 - i) : i;
                const Color& c = spec.colormap.data[idx];
                float y0 = static_cast<float>(cb.y) + static_cast<float>(i) * blockH;
                svg << "  <rect x=\"" << cb.x << "\" y=\"" << y0
                    << "\" width=\"" << cb.width << "\" height=\"" << blockH
                    << "\" fill=\"rgb(" << static_cast<int>(c.r) << ","
                    << static_cast<int>(c.g) << "," << static_cast<int>(c.b)
                    << ")\" shape-rendering=\"crispEdges\" />\n";
            }
        } else {
            float blockW = static_cast<float>(cb.width) / static_cast<float>(spec.colormap.size);
            for (size_t i = 0; i < spec.colormap.size; ++i) {
                size_t idx = flipIdx ? (spec.colormap.size - 1 - i) : i;
                const Color& c = spec.colormap.data[idx];
                float x0 = static_cast<float>(cb.x) + static_cast<float>(i) * blockW;
                svg << "  <rect x=\"" << x0 << "\" y=\"" << cb.y
                    << "\" width=\"" << blockW << "\" height=\"" << cb.height
                    << "\" fill=\"rgb(" << static_cast<int>(c.r) << ","
                    << static_cast<int>(c.g) << "," << static_cast<int>(c.b)
                    << ")\" shape-rendering=\"crispEdges\" />\n";
            }
        }
    } else {
        // Continuous: linear gradient
        bool gradientFlipped = (spec.scale.inverted != style.reverseColors);
        svg << "  <defs>\n";
        if (isVertical) {
            if (gradientFlipped) {
                svg << "    <linearGradient id=\"scibarGrad\" x1=\"0\" y1=\"0\" x2=\"0\" y2=\"1\">\n";
            } else {
                svg << "    <linearGradient id=\"scibarGrad\" x1=\"0\" y1=\"1\" x2=\"0\" y2=\"0\">\n";
            }
        } else {
            if (gradientFlipped) {
                svg << "    <linearGradient id=\"scibarGrad\" x1=\"1\" y1=\"0\" x2=\"0\" y2=\"0\">\n";
            } else {
                svg << "    <linearGradient id=\"scibarGrad\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">\n";
            }
        }
        for (size_t i = 0; i < spec.colormap.size; ++i) {
            const Color& c = spec.colormap.data[i];
            float offset = (spec.colormap.size > 1)
                ? static_cast<float>(i) / static_cast<float>(spec.colormap.size - 1)
                : 0.5f;
            svg << "      <stop offset=\"" << offset
                << "\" stop-color=\"rgb(" << static_cast<int>(c.r) << ","
                << static_cast<int>(c.g) << "," << static_cast<int>(c.b)
                << ")\" stop-opacity=\"" << (c.a / 255.0f) << "\" />\n";
        }
        svg << "    </linearGradient>\n";
        svg << "  </defs>\n";

        svg << "  <rect x=\"" << cb.x << "\" y=\"" << cb.y
            << "\" width=\"" << cb.width << "\" height=\"" << cb.height
            << "\" fill=\"url(#scibarGrad)\" />\n";
    }

    // Frame
    if (style.showFrame) {
        svg << "  <rect x=\"" << cb.x << "\" y=\"" << cb.y
            << "\" width=\"" << cb.width << "\" height=\"" << cb.height
            << "\" fill=\"none\" stroke=\"rgb("
            << static_cast<int>(style.frameColor.r) << ","
            << static_cast<int>(style.frameColor.g) << ","
            << static_cast<int>(style.frameColor.b)
            << ")\" stroke-width=\"1\" />\n";
    }

    // Ticks
    const std::vector<Tick>* ticks = &spec.ticks;
    std::vector<Tick> generated;
    if (ticks->empty()) {
        generated = generateTicks(spec.scale, 5, style.tickPrecision);
        ticks = &generated;
    }

    float range = spec.scale.max - spec.scale.min;
    if (range > 0.0f) {
        for (const auto& tick : *ticks) {
            float fraction = 0.0f;
            if (spec.scale.type == ScaleType::Logarithmic) {
                if (spec.scale.min > 0.0f && tick.value > 0.0f) {
                    float logMin = std::log10(spec.scale.min);
                    float logMax = std::log10(spec.scale.max);
                    fraction = (std::log10(tick.value) - logMin) / (logMax - logMin);
                }
            } else {
                fraction = (tick.value - spec.scale.min) / range;
            }

            if (spec.scale.inverted) fraction = 1.0f - fraction;

            if (isVertical) {
                float y = static_cast<float>(cb.y + cb.height) -
                          fraction * static_cast<float>(cb.height);
                float tickStartX = static_cast<float>(cb.x + cb.width);
                float tickEndX = tickStartX + style.tickLength;
                svg << "  <line x1=\"" << tickStartX << "\" y1=\"" << y
                    << "\" x2=\"" << tickEndX << "\" y2=\"" << y
                    << "\" stroke=\"rgb("
                    << static_cast<int>(style.tickColor.r) << ","
                    << static_cast<int>(style.tickColor.g) << ","
                    << static_cast<int>(style.tickColor.b)
                    << ")\" stroke-width=\"1\" />\n";
                float labelX = tickEndX + 3.0f;
                svg << "  <text x=\"" << labelX << "\" y=\"" << y
                    << "\" font-family=\"Inter, sans-serif\" font-size=\""
                    << style.font.size << "\" fill=\"rgb("
                    << static_cast<int>(style.textColor.r) << ","
                    << static_cast<int>(style.textColor.g) << ","
                    << static_cast<int>(style.textColor.b)
                    << ")\" text-anchor=\"start\" dominant-baseline=\"central\">"
                    << tick.label << "</text>\n";
            } else {
                float x = static_cast<float>(cb.x) +
                          fraction * static_cast<float>(cb.width);
                float tickStartY = static_cast<float>(cb.y + cb.height);
                float tickEndY = tickStartY + style.tickLength;
                svg << "  <line x1=\"" << x << "\" y1=\"" << tickStartY
                    << "\" x2=\"" << x << "\" y2=\"" << tickEndY
                    << "\" stroke=\"rgb("
                    << static_cast<int>(style.tickColor.r) << ","
                    << static_cast<int>(style.tickColor.g) << ","
                    << static_cast<int>(style.tickColor.b)
                    << ")\" stroke-width=\"1\" />\n";
                float labelY = tickEndY + style.font.size + 2.0f;
                svg << "  <text x=\"" << x << "\" y=\"" << labelY
                    << "\" font-family=\"Inter, sans-serif\" font-size=\""
                    << style.font.size << "\" fill=\"rgb("
                    << static_cast<int>(style.textColor.r) << ","
                    << static_cast<int>(style.textColor.g) << ","
                    << static_cast<int>(style.textColor.b)
                    << ")\" text-anchor=\"middle\" dominant-baseline=\"hanging\">"
                    << tick.label << "</text>\n";
            }
        }
    }

    // Sub-ticks
    if (style.showSubTicks) {
        const std::vector<SubTick>* subTicks = &spec.subTicks;
        std::vector<SubTick> generatedSub;
        if (subTicks->empty()) {
            generatedSub = generateSubTicks(spec.scale, *ticks, style.subTicksPerInterval);
            subTicks = &generatedSub;
        }

        for (const auto& st : *subTicks) {
            float fraction = 0.0f;
            if (spec.scale.type == ScaleType::Logarithmic) {
                if (spec.scale.min > 0.0f && st.value > 0.0f) {
                    float logMin = std::log10(spec.scale.min);
                    float logMax = std::log10(spec.scale.max);
                    fraction = (std::log10(st.value) - logMin) / (logMax - logMin);
                }
            } else {
                fraction = (st.value - spec.scale.min) / range;
            }

            if (spec.scale.inverted) fraction = 1.0f - fraction;

            if (isVertical) {
                float y = static_cast<float>(cb.y + cb.height) -
                          fraction * static_cast<float>(cb.height);
                float tickStartX = static_cast<float>(cb.x + cb.width);
                float tickEndX = tickStartX + style.subTickLength;
                svg << "  <line x1=\"" << tickStartX << "\" y1=\"" << y
                    << "\" x2=\"" << tickEndX << "\" y2=\"" << y
                    << "\" stroke=\"rgb("
                    << static_cast<int>(style.tickColor.r) << ","
                    << static_cast<int>(style.tickColor.g) << ","
                    << static_cast<int>(style.tickColor.b)
                    << ")\" stroke-width=\"1\" />\n";
            } else {
                float x = static_cast<float>(cb.x) +
                          fraction * static_cast<float>(cb.width);
                float tickStartY = static_cast<float>(cb.y + cb.height);
                float tickEndY = tickStartY + style.subTickLength;
                svg << "  <line x1=\"" << x << "\" y1=\"" << tickStartY
                    << "\" x2=\"" << x << "\" y2=\"" << tickEndY
                    << "\" stroke=\"rgb("
                    << static_cast<int>(style.tickColor.r) << ","
                    << static_cast<int>(style.tickColor.g) << ","
                    << static_cast<int>(style.tickColor.b)
                    << ")\" stroke-width=\"1\" />\n";
            }
        }
    }

    // Title
    if (!spec.title.empty()) {
        int titleX = cb.x + cb.width / 2;
        int titleY = isVertical ? (cb.y - 10) : (cb.y - 15);
        svg << "  <text x=\"" << titleX << "\" y=\"" << titleY
            << "\" font-family=\"Inter, sans-serif\" font-size=\""
            << style.font.size
            << "\" fill=\"rgb("
            << static_cast<int>(style.textColor.r) << ","
            << static_cast<int>(style.textColor.g) << ","
            << static_cast<int>(style.textColor.b)
            << ")\" text-anchor=\"middle\">"
            << spec.title << "</text>\n";
    }

    svg << "</svg>\n";
    return svg.str();
}

} // namespace scibar
