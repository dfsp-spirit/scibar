// scibar example: reversed categorical vertical colorbar
// Demonstrates high-level API (vertical layout) with reversed categorical scale.

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
    const int W = 300, H = 600;
    std::vector<uint32_t> buffer(static_cast<size_t>(W) * H);
    scibar::Canvas canvas{buffer.data(), W, H};

    scibar::fillCanvas(canvas, scibar::Color{255, 255, 255, 255});

    // --- Build a qualitative colormap (5 categories) ---
    // Using Tableau 10-inspired distinct colors
    std::vector<scibar::Color> qualCmap = {
        scibar::Color{ 31, 119, 180, 255}, // blue
        scibar::Color{255, 127,  14, 255}, // orange
        scibar::Color{ 44, 160,  44, 255}, // green
        scibar::Color{214,  39,  40, 255}, // red
        scibar::Color{148, 103, 189, 255}, // purple
    };

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Categorical;
    spec.scale.min  = 0.0f;
    spec.scale.max  = 5.0f;
    spec.title      = "Category (reversed)";
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
    style.reverseColors = true;

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
