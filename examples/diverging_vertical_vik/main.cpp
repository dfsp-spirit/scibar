// scibar example: vertical diverging colorbar with vik colormap
// Demonstrates high-level API (vertical layout) with a diverging scale.

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define SCIBAR_IMPLEMENTATION

#include "../../src/core/scibar/scibar.hpp"
#include "../../src/third_party/stb_image_write.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main() {
    int errors = 0;

    // --- Load font ---
    scibar::Font font;  // nullptr handle → uses embedded Inter font

    // --- Setup canvas ---
    const int W = 300, H = 600;
    std::vector<uint32_t> buffer(static_cast<size_t>(W) * H);
    scibar::Canvas canvas{buffer.data(), W, H};

    scibar::fillCanvas(canvas, scibar::Color{255, 255, 255, 255});

    // --- Define diverging data ---
    auto vikCmap = scibar::util::vik();

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Diverging;
    spec.scale.min  = -5.0f;
    spec.scale.max  = 5.0f;
    spec.scale.midpoint = 0.0f;
    spec.label      = "Anomaly (Vik)";
    spec.colormap   = vikCmap;

    // --- Style ---
    scibar::Style style = scibar::Style::defaultLight();
    style.font = font;

    // --- Draw ---
    scibar::LayoutResult result = scibar::drawLegend(canvas, spec, style);

    printf("Colorbar bounds: (%d,%d) %dx%d\n",
           result.colorbarBoundingBox.x, result.colorbarBoundingBox.y,
           result.colorbarBoundingBox.width, result.colorbarBoundingBox.height);
    printf("Generated ticks: %d (includes midpoint 0.0)\n", result.generatedTickCount);

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
    svgOpts.colorbarBounds = {180, 50, 40, 500};

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
