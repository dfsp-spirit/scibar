/// scibar minimal example — horizontal colorbar to PPM (no external deps).
/// This is the shortest runnable example; ideal for the README.

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar.hpp"

#include <vector>

int main() {
    using namespace scibar;

    // Canvas: wide and short for a horizontal bar
    const int W = 600, H = 120;
    std::vector<uint32_t> buf(W * H);
    Canvas canvas{buf.data(), W, H};
    fillCanvas(canvas, Color{255, 255, 255, 255});

    // Data + colormap
    auto cmap = util::viridis();   // store locally — ColorMapView is non-owning
    Spec spec;
    spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
    spec.title    = "Temperature (°C)";
    spec.colormap = cmap;

    // Render
    Style style = Style::defaultLight();
    Rect barRect{50, 45, 500, 30};
    drawColorBar(canvas, barRect, spec, style, Orientation::Horizontal);
    drawTicks(canvas, barRect, spec, style, Orientation::Horizontal);

    // Save as PPM (no external dependencies)
    writePPM(canvas, "colorbar.ppm");

    return 0;
}
