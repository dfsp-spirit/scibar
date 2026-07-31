// scibar example: axis-inversed horizontal diverging colorbar with vik colormap
//
// Demonstrates row 4 from the reverseColors / inverted documentation table:
//   "Colors flipped + data range inverted" = scale.inverted + style.reverseColors
//
// Compare this with:
//   - diverging_horizontal_vik           (default:  -5 blue → 5 red)
//   - diverging_horizontal_vik_reversed  (colors only: -5 red → 5 blue)
//
// This example: left=5 (red) → right=-5 (blue)
//   - scale.inverted flips the data direction (labels run 5 → -5)
//   - style.reverseColors flips the colormap (red=high stays on high-value side)
//   - On continuous scales, the two flags cancel for colors but NOT for labels.
//     The gradient pixels are identical to default, but tick positions are flipped.

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define SCIBAR_IMPLEMENTATION

#include "../../src/core/scibar.hpp"
#include "../../src/third_party/stb_image_write.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main() {
    int errors = 0;

    // --- Load font ---
    scibar::Font font;  // nullptr handle → uses embedded Inter font

    // --- Setup canvas ---
    const int W = 700, H = 200;
    std::vector<uint32_t> buffer(static_cast<size_t>(W) * H);
    scibar::Canvas canvas{buffer.data(), W, H};

    scibar::fillCanvas(canvas, scibar::Color{255, 255, 255, 255});

    // --- Define diverging data ---
    // scale.inverted = true flips the data direction so max runs visually at
    // the start of the bar and min at the end. The min < max invariant is kept.
    auto vikCmap = scibar::util::vik();

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Diverging;
    spec.scale.min  = -5.0f;
    spec.scale.max  = 5.0f;
    spec.scale.midpoint = 0.0f;
    spec.scale.inverted = true;   // data runs 5 → -5 visually
    spec.title      = "Anomaly (Vik, axis inverted)";
    spec.colormap   = vikCmap;

    // --- Style ---
    // Combine scale.inverted with style.reverseColors to fully mirror the bar:
    //   - inverted flips the data direction (labels run 5 → -5)
    //   - reverseColors flips the colormap so red still means warm / high end
    scibar::Style style = scibar::Style::defaultLight();
    style.font = font;
    style.reverseColors = true;

    // --- Manual horizontal layout ---
    int barWidth  = 500;
    int barHeight = 30;
    int barX = (W - barWidth) / 2;
    int barY = 80;

    scibar::Rect barRect{barX, barY, barWidth, barHeight};

    // Title
    scibar::Rect titleRect{barX, barY - 35, barWidth, 30};
    scibar::drawTitle(canvas, titleRect, spec.title, style);

    // Colorbar
    scibar::drawColorBar(canvas, barRect, spec, style, scibar::Orientation::Horizontal);

    // Ticks below the bar
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
