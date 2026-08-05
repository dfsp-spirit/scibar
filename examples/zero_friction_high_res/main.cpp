/// scibar zero_friction_high_res example — high-resolution colorbars from the
/// zero-friction exportColorbar() API.
///
/// Demonstrates the three ways you can size the canvas via ExportOpts without
/// touching any lower API level:
///   1. vertical  — only the long side (canvasH) given; width auto-fills to 200
///   2. horizontal — only the long side (canvasW) given; height auto-fills to 200
///   3. both sides given — used as-is
///
/// Each case is exported to BOTH a raster PNG and a vector SVG, to show that
/// the SVG path handles arbitrary canvas sizes just as well as the raster
/// path (nothing breaks, layouts match). The font (and tick marks) are scaled
/// by the same factor as the canvas so text stays proportionally sized at
/// higher resolution — the recipe the FAQ documents.

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar/scibar.hpp"

#include <cstdio>
#include <string>

using namespace scibar;

/// Render one case to both .png and .svg, reporting success for each.
static void renderCase(const char* stem, Orientation orientation,
                       int canvasW, int canvasH, float fontScale,
                       const std::vector<Color>& cmap)
{
    ExportOpts opts;
    opts.scale       = {ScaleType::Linear, 0.0f, 100.0f};
    opts.label       = "Temperature (°C)";
    opts.colormap    = cmap;
    opts.orientation = orientation;
    opts.canvasW     = canvasW;   // 0 = auto
    opts.canvasH     = canvasH;   // 0 = auto

    // Scale font and tick marks by the same factor as the canvas, so the
    // output keeps the default proportions at higher resolution.
    opts.style.font.size     = 14.0f * fontScale;
    opts.style.tickLength    = 5.0f  * fontScale;
    opts.style.subTickLength = 3.0f  * fontScale;

    std::string png = std::string(stem) + ".png";
    std::string svg = std::string(stem) + ".svg";
    bool pngOk = exportColorbar(opts, png.c_str());
    bool svgOk = exportColorbar(opts, svg.c_str());

    // Resolve the canvas the same way exportColorbar() does, so the console
    // output shows the auto-filled dimensions (short side = default).
    int rw = canvasW, rh = canvasH;
    if (rw <= 0 || rh <= 0) {
        if (orientation == Orientation::Horizontal) {
            if (rw <= 0) rw = 500;
            if (rh <= 0) rh = 200;
        } else {
            if (rw <= 0) rw = 200;
            if (rh <= 0) rh = 500;
        }
    }
    std::printf("  %-26s  %4d×%-5d  png=%s  svg=%s\n",
                stem, rw, rh, pngOk ? "ok" : "FAIL", svgOk ? "ok" : "FAIL");
}

int main() {
    std::printf("scibar — zero-friction high-resolution example\n");
    std::printf("===============================================\n\n");

    std::vector<Color> cmap = util::viridis();  // built-in 256-entry colormap

    // 1. Vertical, long side only: canvasH = 1500, canvasW auto → 200.
    //    The short side keeps its narrow default, so there is no room to scale
    //    the text up — keep the default font so nothing is clipped.
    std::printf("Case 1 — vertical, height only (1500, width auto=200)\n");
    renderCase("vertical_height_only", Orientation::Vertical,
               0, 1500, 1.0f, cmap);

    // 2. Horizontal, long side only: canvasW = 1500, canvasH auto → 200.
    //    The long side is wide, so the text CAN be scaled up 3× to match.
    std::printf("Case 2 — horizontal, width only (1500, height auto=200)\n");
    renderCase("horizontal_width_only", Orientation::Horizontal,
               1500, 0, 3.0f, cmap);

    // 3. Both sides given: 900×1500 vertical. Full high-res recipe — a wide
    //    enough canvas and text scaled 3× for a print-quality result.
    std::printf("Case 3 — both sides given (900×1500)\n");
    renderCase("both_specified", Orientation::Vertical,
               900, 1500, 3.0f, cmap);

    std::printf("\nDone. Check the .png files; open the .svg files in a browser.\n");
    return 0;
}
