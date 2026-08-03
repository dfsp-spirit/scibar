// scibar example: horizontal viridis colorbar
// Demonstrates low-level API for manual horizontal layout with PNG + SVG output.

#define SCIBAR_IMPLEMENTATION

#include "../../src/core/scibar/scibar.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

int main() {
    int errors = 0;

    // --- Load font ---
    scibar::Font font;  // nullptr handle → uses embedded Inter font

    // --- Setup canvas (wide and short for horizontal bar) ---
    const int W = 700, H = 200;
    std::vector<uint32_t> buffer(static_cast<size_t>(W) * H);
    scibar::Canvas canvas{buffer.data(), W, H};

    scibar::fillCanvas(canvas, scibar::Color{255, 255, 255, 255});

    // --- Define data ---
    auto viridisCmap = scibar::util::viridis();

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Linear;
    spec.scale.min  = 2.0f;
    spec.scale.max  = 5.0f;
    spec.label      = "Random Value";
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
    scibar::Rect labelRect{barX, barY - 35, barWidth, 30};
    scibar::Rect labelBounds = scibar::drawLabel(canvas, labelRect, spec.label, style);

    // Colorbar
    scibar::Rect colorBounds = scibar::drawColorBar(canvas, barRect, spec, style,
                                                      scibar::Orientation::Horizontal);

    // Ticks below the bar (horizontal orientation)
    scibar::Rect tickBounds = scibar::drawTicks(canvas, barRect, spec, style,
                                                scibar::Orientation::Horizontal);
    scibar::drawSubTicks(canvas, barRect, spec, style, scibar::Orientation::Horizontal);

    printf("Colorbar bounds: (%d,%d) %dx%d\n",
           colorBounds.x, colorBounds.y, colorBounds.width, colorBounds.height);

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
