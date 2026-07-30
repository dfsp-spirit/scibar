// scibar implementation — development phase
// Will be folded into scibar.hpp under #ifdef SCIBAR_IMPLEMENTATION later.

#define SCIBAR_IMPLEMENTATION

// Enable implementations for vendored single-header libraries
#define CANVAS_ITY_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "scibar.hpp"
#include "third_party/canvas_ity.hpp"
#include "third_party/stb_truetype.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>
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

    return ticks;
}

} // namespace scibar
