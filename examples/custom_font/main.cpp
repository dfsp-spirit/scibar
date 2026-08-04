// scibar example: custom font
// Demonstrates loading an external .ttf font via loadFont() and using it
// with the low-level API.  This is the primary way to use a non-default
// typeface for publication-quality output.
//
// Writes three outputs:
//   1. colorbar_custom_font.png              — raster, rendered with the font
//   2. colorbar_custom_font.svg              — SVG referencing the font family
//      (auto-derived from the TTF); the viewer needs the font installed
//   3. colorbar_custom_font_embedded.svg     — SVG with the font embedded as a
//      base64 @font-face, so text is identical in @font-face-capable viewers
//      (browsers, Inkscape) without requiring the font to be installed.
//
// The bundled Libertinus Serif (OFL license) is a serif font designed for
// academic publishing — a natural complement to the embedded Inter sans-serif.

#define SCIBAR_IMPLEMENTATION

#include "../../src/core/scibar/scibar.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main() {
    int errors = 0;

    // --- Load the external serif font (copied to build dir by CMake) ---
    scibar::Font serifFont = scibar::loadFont("LibertinusSerif-Regular.ttf", 18.0f);

    // --- Setup canvas ---
    const int W = 400, H = 650;
    std::vector<uint32_t> buffer(static_cast<size_t>(W) * H);
    scibar::Canvas canvas{buffer.data(), W, H};
    scibar::fillCanvas(canvas, scibar::Color{255, 255, 255, 255});

    // --- Build a viridis colormap ---
    std::vector<scibar::Color> viridisLut = scibar::util::viridis();

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 100.0f;
    spec.label      = "Intensity (a.u.)";
    spec.colormap   = viridisLut;

    scibar::Style style = scibar::Style::defaultLight();
    style.font = serifFont;
    style.tickLength = 7.0f;
    style.tickPrecision = 1;

    // --- Manual layout (low-level API) ---
    int barWidth  = 40;
    int barHeight = 520;
    int barX = 60;
    int barY = 80;

    scibar::Rect barRect{barX, barY, barWidth, barHeight};

    // Title: centered above the bar
    scibar::Rect labelRect{barX, barY - 60, barWidth, 50};
    scibar::drawLabel(canvas, labelRect, spec.label, style);

    // Colorbar
    scibar::drawColorBar(canvas, barRect, spec, style, scibar::Orientation::Vertical);

    // Ticks and sub-ticks
    scibar::drawTicks(canvas, barRect, spec, style, scibar::Orientation::Vertical);
    scibar::drawSubTicks(canvas, barRect, spec, style, scibar::Orientation::Vertical);

    // --- Report font metrics ---
    scibar::FontMetrics fm = scibar::fontMetrics(serifFont);
    printf("Libertinus Serif @ %.0fpx: ascender=%.1f descender=%.1f lineHeight=%.1f\n",
           serifFont.size, fm.ascender, fm.descender, fm.lineHeight);

    // --- Write PNG ---
    if (stbi_write_png("colorbar_custom_font.png", W, H, 4, buffer.data(), W * 4)) {
        printf("Wrote colorbar_custom_font.png (%dx%d)\n", W, H);
    } else {
        fprintf(stderr, "Failed to write colorbar_custom_font.png\n");
        errors++;
    }

    // --- Write SVGs (referenced font + embedded font) ---
    scibar::SVGOptions svgOpts;
    svgOpts.totalWidth   = 300;
    svgOpts.totalHeight  = 600;
    svgOpts.colorbarBounds = {40, 50, 40, 500};

    auto writeSvg = [](const char* path, const std::string& content) -> bool {
        FILE* f = fopen(path, "w");
        if (!f) return false;
        fputs(content.c_str(), f);
        fclose(f);
        return true;
    };

    // (1) Referenced font: the SVG names the family (auto-derived from the TTF
    //     name table), so the viewer must have Libertinus Serif installed or it
    //     falls back to sans-serif.
    std::string svg = scibar::exportToSVG(spec, style, svgOpts,
                                          scibar::Orientation::Vertical);
    if (writeSvg("colorbar_custom_font.svg", svg)) {
        printf("Wrote colorbar_custom_font.svg (font referenced)\n");
    } else {
        fprintf(stderr, "Failed to write colorbar_custom_font.svg\n");
        errors++;
    }

    // (2) Embedded font: the TTF is embedded as a base64 @font-face, so text is
    //     pixel-identical in any @font-face-capable viewer (browsers, Inkscape)
    //     even if the font is not installed. Not supported by librsvg.
    scibar::Style embedStyle = style;
    embedStyle.embedFontInSvg = true;
    std::string svgEmbedded = scibar::exportToSVG(spec, embedStyle, svgOpts,
                                                  scibar::Orientation::Vertical);
    if (writeSvg("colorbar_custom_font_embedded.svg", svgEmbedded)) {
        printf("Wrote colorbar_custom_font_embedded.svg (font embedded, ~%.0f KB)\n",
               svgEmbedded.size() / 1024.0);
    } else {
        fprintf(stderr, "Failed to write colorbar_custom_font_embedded.svg\n");
        errors++;
    }

    if (errors == 0) {
        printf("Done.\n");
    }
    return errors;
}
