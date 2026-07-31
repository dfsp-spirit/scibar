// scibar example: logarithmic horizontal colorbar with viridis colormap
// Demonstrates low-level API for manual horizontal layout with logarithmic scale.

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "../../src/core/scibar.hpp"
#include "../../src/third_party/canvas_ity.hpp"
#include "../../src/third_party/stb_truetype.h"
#include "../../src/third_party/stb_image_write.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main() {
    int errors = 0;

    // --- Load font ---
    scibar::Font font = scibar::loadFont("../../../fonts/Inter-Regular.ttf", 14.0f);

    // --- Setup canvas (wide and short for horizontal bar) ---
    const int W = 700, H = 200;
    std::vector<uint32_t> buffer(static_cast<size_t>(W) * H);
    scibar::Canvas canvas{buffer.data(), W, H};

    scibar::fillCanvas(canvas, scibar::Color{255, 255, 255, 255});

    // --- Define data (log scale: positive range) ---
    auto viridisCmap = scibar::util::viridis();

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Logarithmic;
    spec.scale.min  = 1.0f;
    spec.scale.max  = 1000.0f;
    spec.title      = "Value (log)";
    spec.colormap   = viridisCmap;

    // --- Style ---
    scibar::Style style = scibar::Style::defaultLight();
    style.font = font;

    // --- Manual horizontal layout ---
    int barWidth  = 500;
    int barHeight = 30;
    int barX = (W - barWidth) / 2;
    int barY = 80;

    scibar::Rect barRect{barX, barY, barWidth, barHeight};

    // Title: centered above the bar
    scibar::Rect titleRect{barX, barY - 35, barWidth, 30};
    scibar::drawTitle(canvas, titleRect, spec.title, style);

    // Colorbar
    scibar::drawColorBar(canvas, barRect, spec, style, scibar::Orientation::Horizontal);

    // Ticks below the bar (horizontal orientation)
    scibar::drawTicks(canvas, barRect, spec, style, scibar::Orientation::Horizontal);
    scibar::drawSubTicks(canvas, barRect, spec, style, scibar::Orientation::Horizontal);

    // --- Write PNG ---
    int pngOk = stbi_write_png("colorbar.png", W, H, 4, buffer.data(), W * 4);
    if (pngOk) {
        printf("Wrote colorbar.png (%dx%d)\n", W, H);
    } else {
        fprintf(stderr, "Failed to write colorbar.png\n");
        errors++;
    }

    // --- Write SVG ---
    scibar::SVGOptions svgOpts;
    svgOpts.totalWidth  = 700;
    svgOpts.totalHeight = 250;
    svgOpts.colorbarBounds = {100, 150, 500, 30};

    std::string svg = scibar::exportToSVG(spec, style, svgOpts,
                                        scibar::Orientation::Horizontal);

    FILE* svgFile = fopen("colorbar.svg", "w");
    if (svgFile) {
        fputs(svg.c_str(), svgFile);
        fclose(svgFile);
        printf("Wrote colorbar.svg\n");
    } else {
        fprintf(stderr, "Failed to write colorbar.svg\n");
        errors++;
    }

    printf("Done.\n");
    return errors > 0 ? 1 : 0;
}
