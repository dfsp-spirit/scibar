/// scibar example: disable built-in PNG support via SCIBAR_NO_PNG.
/// Demonstrates that PPM and SVG still work, and shows how to write PNG
/// manually using your own stb_image_write if needed.

#define SCIBAR_NO_PNG
#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar/scibar.hpp"

#include <cstdio>

int main() {
    using namespace scibar;

    // ── exportColorbar: PPM and SVG still work ──
    auto cmap = util::viridis();  // store locally — ColorMapView is non-owning
    ExportOpts opts;
    opts.colormap = cmap;
    opts.label    = "Value";

    // PNG will fail — SCIBAR_NO_PNG disables built-in stb support
    bool pngOk = exportColorbar(opts, "no_png_test.png");
    printf("PNG: %s (expected: fail)\n", pngOk ? "OK" : "FAIL");

    // PPM always works (zero deps)
    bool ppmOk = exportColorbar(opts, "no_png_test.ppm");
    printf("PPM: %s\n", ppmOk ? "OK" : "FAIL");

    // SVG always works (zero deps)
    bool svgOk = exportColorbar(opts, "no_png_test.svg");
    printf("SVG: %s\n", svgOk ? "OK" : "FAIL");

    // If you need PNG with SCIBAR_NO_PNG, you can still render to a Canvas
    // and call stbi_write_png yourself — scibar's Canvas exposes the raw
    // pixel buffer:

    // const int W = 300, H = 600;
    // std::vector<uint32_t> buf(W * H);
    // Canvas canvas{buf.data(), W, H};
    // fillCanvas(canvas, Color{255, 255, 255, 255});
    //
    // auto cmap = util::viridis();
    // Spec spec;
    // spec.scale    = {ScaleType::Linear, 0.0f, 100.0f};
    // spec.label    = "Value";
    // spec.colormap = cmap;
    // drawLegend(canvas, spec);
    //
    // // Call your own stbi_write_png:
    // // stbi_write_png("manual.png", W, H, 4, canvas.pixels, W * 4);

    return 0;
}
