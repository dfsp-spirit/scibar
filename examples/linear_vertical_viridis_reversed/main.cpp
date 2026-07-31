// scibar example: reversed vertical viridis colorbar
// Demonstrates high-level API (vertical layout) with reversed colormap.

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

    // --- Setup canvas ---
    const int W = 300, H = 600;
    std::vector<uint32_t> buffer(static_cast<size_t>(W) * H);
    scibar::Canvas canvas{buffer.data(), W, H};

    // White background (via fillCanvas helper)
    scibar::fillCanvas(canvas, scibar::Color{255, 255, 255, 255});

    // --- Define data ---
    auto viridisCmap = scibar::util::viridis();

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 2.0f;
    spec.scale.max  = 5.0f;
    spec.title      = "Random Value (reversed)";
    spec.colormap   = viridisCmap;

    // --- Style ---
    scibar::Style style = scibar::Style::defaultLight();
    style.font = font;
    style.reversed = true;

    // --- Draw (high-level vertical layout) ---
    scibar::LayoutResult result = scibar::drawLegend(canvas, spec, style);

    printf("Colorbar bounds: (%d,%d) %dx%d\n",
           result.colorbarBoundingBox.x, result.colorbarBoundingBox.y,
           result.colorbarBoundingBox.width, result.colorbarBoundingBox.height);
    printf("Total bounds:    (%d,%d) %dx%d\n",
           result.totalBoundingBox.x, result.totalBoundingBox.y,
           result.totalBoundingBox.width, result.totalBoundingBox.height);
    printf("Generated ticks: %d\n", result.generatedTickCount);

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
    svgOpts.totalWidth  = 400;
    svgOpts.totalHeight = 650;
    svgOpts.colorbarBounds = {60, 50, 40, 500};

    std::string svg = scibar::exportToSVG(spec, style, svgOpts,
                                        scibar::Orientation::Vertical);

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
