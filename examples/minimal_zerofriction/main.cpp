/// scibar minimal zero-friction example — four colorbars in four calls.
///
/// Demonstrates the simplest possible API: one ExportOpts struct, one call
/// per file. Produces a standard linear viridis colorbar in both orientations
/// (vertical and horizontal) and both formats (PNG raster, SVG vector).

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar/scibar.hpp"

int main() {
    using namespace scibar;

    std::vector<Color> cmap = util::viridis();  // built-in 256-entry colormap

    ExportOpts opts;
    opts.scale    = {ScaleType::Linear, 0.0f, 100.0f};
    opts.label    = "Temperature (°C)";
    opts.colormap = cmap;

    // Vertical — PNG raster
    opts.orientation = Orientation::Vertical;
    exportColorbar(opts, "vertical.png");

    // Vertical — SVG vector (same layout, identical placement)
    exportColorbar(opts, "vertical.svg");

    // Horizontal — PNG raster (auto canvas: 500×200)
    opts.orientation = Orientation::Horizontal;
    exportColorbar(opts, "horizontal.png");

    // Horizontal — SVG vector
    exportColorbar(opts, "horizontal.svg");

    return 0;
}
