// scibar example: custom font
// Demonstrates loading an external .ttf font via loadFont() and using it
// with the low-level API.  This is the primary way to use a non-default
// typeface for publication-quality output.
//
// The bundled Libertinus Serif (OFL license) is a serif font designed for
// academic publishing — a natural complement to the embedded Inter sans-serif.

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define SCIBAR_IMPLEMENTATION

#include "../../src/core/scibar/scibar.hpp"
#include "../../src/third_party/stb_image_write.h"

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

    // --- Write SVG ---
    scibar::SVGOptions svgOpts;
    svgOpts.totalWidth   = 300;
    svgOpts.totalHeight  = 600;
    svgOpts.colorbarBounds = {40, 50, 40, 500};

    std::string svg = scibar::exportToSVG(spec, style, svgOpts,
                                          scibar::Orientation::Vertical);

    FILE* svgFile = fopen("colorbar_custom_font.svg", "w");
    if (svgFile) {
        fputs(svg.c_str(), svgFile);
        fclose(svgFile);
        printf("Wrote colorbar_custom_font.svg\n");
    } else {
        fprintf(stderr, "Failed to write colorbar_custom_font.svg\n");
        errors++;
    }

    if (errors == 0) {
        printf("Done.\n");
    }
    return errors;
}
