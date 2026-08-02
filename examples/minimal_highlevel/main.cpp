/// scibar minimal high-level example — vertical colorbar to PPM (no external deps).
/// Uses the high-level drawLegend API for automatic layout.

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar/scibar.hpp"

#include <vector>

int main() {
    using namespace scibar;

    // Canvas: tall and narrow for a vertical bar (drawLegend hardcodes vertical layout)
    const int W = 200, H = 500;
    std::vector<uint32_t> buf(W * H);
    Canvas canvas{buf.data(), W, H};
    fillCanvas(canvas, Color{255, 255, 255, 255});

    // Data + colormap
    auto cmap = util::viridis();   // store locally — ColorMapView is non-owning
    Spec spec;
    spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
    spec.title    = "Temperature (°C)";
    spec.colormap = cmap;

    // Render with the high-level API — handles title, colorbar, ticks, sub-ticks automatically
    Style style = Style::defaultLight();
    drawLegend(canvas, spec, style);

    // Save as PPM (no external dependencies)
    writePPM(canvas, "colorbar.ppm");

    return 0;
}
