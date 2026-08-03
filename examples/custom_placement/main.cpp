/// scibar custom_placement example — demonstrates placing a colorbar at an
/// arbitrary position on a large canvas using the low-level API, with both
/// raster (PPM) and vector (SVG) output from a single, shared layout.
///
/// This is the canonical example for the dual-export workflow: compute the
/// layout once, then feed the same bounds to both backends for identical
/// placement across PNG and SVG output.

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar/scibar.hpp"

#include <vector>
#include <fstream>

int main() {
    using namespace scibar;

    // =========================================================================
    // 1. Define the colorbar specification (shared by both backends)
    // =========================================================================

    auto cmap = util::viridis();   // store locally — ColorMapView is non-owning
    Spec spec;
    spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
    spec.label    = "Temperature (°C)";
    spec.colormap = cmap;

    Style style = Style::defaultLight();
    style.showFrame = true;

    // =========================================================================
    // 2. Compute the layout — ONCE
    // =========================================================================
    // We want a small vertical colorbar. The layout is computed for a compact
    // canvas that would just fit the colorbar. We'll then place it at an
    // arbitrary offset on a larger canvas.

    const int barCanvasW = 150, barCanvasH = 400;
    LegendLayout layout = computeLegendLayout(barCanvasW, barCanvasH,
                                               spec, style,
                                               Orientation::Vertical);

    // =========================================================================
    // 3. Place on a large canvas at a custom offset (raster output)
    // =========================================================================

    const int bigW = 800, bigH = 600;
    const int offsetX = 600;  // place colorbar in right portion of canvas
    const int offsetY = 50;   // 50px down from top

    std::vector<uint32_t> buf(bigW * bigH);
    Canvas bigCanvas{buf.data(), bigW, bigH};
    fillCanvas(bigCanvas, Color{255, 255, 255, 255});

    // Shift the layout bounds by the offset
    Rect shiftedBar   = {layout.barBounds.x   + offsetX,
                         layout.barBounds.y   + offsetY,
                         layout.barBounds.width,
                         layout.barBounds.height};
    Rect shiftedLabel = {layout.labelBounds.x + offsetX,
                         layout.labelBounds.y + offsetY,
                         layout.labelBounds.width,
                         layout.labelBounds.height};

    // Draw using the low-level raster functions
    drawColorBar(bigCanvas, shiftedBar, spec, style, Orientation::Vertical);
    drawTicks(bigCanvas, shiftedBar, spec, style, Orientation::Vertical);
    drawSubTicks(bigCanvas, shiftedBar, spec, style, Orientation::Vertical);
    drawLabel(bigCanvas, shiftedLabel, spec.label, style);

    writePPM(bigCanvas, "custom_placement.ppm");

    // =========================================================================
    // 4. SVG output — identical placement from the same layout
    // =========================================================================

    SVGOptions svgOpts;
    svgOpts.totalWidth     = bigW;
    svgOpts.totalHeight    = bigH;
    svgOpts.colorbarBounds = shiftedBar;
    svgOpts.labelBounds    = shiftedLabel;

    std::string svg = exportToSVG(spec, style, svgOpts, Orientation::Vertical);

    std::ofstream out("custom_placement.svg");
    out << svg;
    out.close();

    return 0;
}
