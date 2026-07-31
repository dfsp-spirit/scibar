// scibar example: categorical horizontal — both axis inverted + colors reversed
//
// Demonstrates scale.inverted + style.reverseColors together on a categorical
// (qualitative) scale. Unlike continuous scales where the two flags cancel,
// on categorical scales they compose differently:
//   - inverted reverses block order (E→A left to right)
//   - reverseColors additionally flips which color each block gets
//
// The net effect: blocks are in original order (A→E, same as default) but
// blue ends up on Type E and purple on Type A — each block carries the
// color of the opposite category from the original.
//
// Compare with:
//   - categorical_horizontal                  (default)
//   - categorical_horizontal_reversed         (reverseColors only)
//   - categorical_horizontal_axisinversed     (inverted only)

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

    // --- Build a qualitative colormap (5 categories) ---
    std::vector<scibar::Color> qualCmap = {
        scibar::Color{ 31, 119, 180, 255}, // blue   (Type A)
        scibar::Color{255, 127,  14, 255}, // orange (Type B)
        scibar::Color{ 44, 160,  44, 255}, // green  (Type C)
        scibar::Color{214,  39,  40, 255}, // red    (Type D)
        scibar::Color{148, 103, 189, 255}, // purple (Type E)
    };

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Categorical;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 5.0f;
    spec.scale.inverted = true;   // axis inverted: data runs E→A visually
    spec.title      = "Category (inverted + reversed)";
    spec.colormap   = qualCmap;

    // Custom ticks: label each category at its center position
    spec.ticks = {
        {0.5f, "Type A"},
        {1.5f, "Type B"},
        {2.5f, "Type C"},
        {3.5f, "Type D"},
        {4.5f, "Type E"},
    };

    // --- Style ---
    scibar::Style style = scibar::Style::defaultLight();
    style.font = font;
    style.reverseColors = true;  // flip colors on top of axis inversion

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
