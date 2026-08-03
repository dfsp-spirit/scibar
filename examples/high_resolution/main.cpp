/// scibar high_resolution example — demonstrates rendering colorbars at high
/// pixel density with scaled-up fonts using the low-level API.  Produces both
/// raster (PPM) and vector (SVG) output for horizontal and vertical bars from
/// a single, shared layout computed by computeLegendLayout().
///
/// This is the go-to example for print-resolution or large-canvas rendering.

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar/scibar.hpp"

#include <vector>
#include <fstream>
#include <cstdio>

using namespace scibar;

// ---------------------------------------------------------------------------
// Helper: render raster + SVG from a shared layout on a large canvas.
// ---------------------------------------------------------------------------
static void renderDual(const std::string& name,
                       const Spec&        spec,
                       const Style&       style,
                       int                canvasW,
                       int                canvasH,
                       Orientation        orientation)
{
    printf("  Rendering %-12s  canvas=%d×%d  font=%.0fpx\n",
           name.c_str(), canvasW, canvasH, style.font.size);

    // 1. Compute layout ONCE — shared between both backends.
    LegendLayout layout = computeLegendLayout(canvasW, canvasH,
                                               spec, style, orientation);

    // 2. Raster output — allocate a large pixel buffer.
    std::vector<uint32_t> buf(canvasW * canvasH);
    Canvas canvas{buf.data(), canvasW, canvasH};
    fillCanvas(canvas, Color{255, 255, 255, 255});

    drawColorBar(canvas,  layout.barBounds,   spec, style, orientation);
    drawTicks(canvas,     layout.barBounds,   spec, style, orientation);
    drawSubTicks(canvas,  layout.barBounds,   spec, style, orientation);
    drawLabel(canvas,     layout.labelBounds, spec.label, style);

    std::string ppmPath = name + ".ppm";
    writePPM(canvas, ppmPath.c_str());
    printf("    → %s  (%d×%d px)\n", ppmPath.c_str(), canvasW, canvasH);

    // 3. SVG output — identical placement via the same layout bounds.
    SVGOptions svgOpts;
    svgOpts.totalWidth     = canvasW;
    svgOpts.totalHeight    = canvasH;
    svgOpts.colorbarBounds = layout.barBounds;
    svgOpts.labelBounds    = layout.labelBounds;

    std::string svgStr = exportToSVG(spec, style, svgOpts, orientation);

    std::string svgPath = name + ".svg";
    std::ofstream out(svgPath);
    out << svgStr;
    out.close();
    printf("    → %s  (%zu bytes)\n", svgPath.c_str(), svgStr.size());
}

// ---------------------------------------------------------------------------
int main() {
    printf("scibar — high-resolution example\n");
    printf("================================\n\n");

    // --- Shared data ---
    auto cmap = util::viridis();
    Style style = Style::defaultLight();
    style.showFrame = true;

    // --- Scale font up for large-canvas rendering ---
    // Default 14 px looks tiny on a 2000-pixel canvas.
    // 32 px keeps text readable at print resolution (~300 DPI → ~12 pt).
    style.font.size = 32.0f;

    // =========================================================================
    // Horizontal — wide bar on a 2000×400 canvas
    // =========================================================================
    {
        Spec spec;
        spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
        spec.label    = "Horizontal · 2000×400 · 32 px font";
        spec.colormap = cmap;

        renderDual("horizontal_hires", spec, style,
                    2000, 400, Orientation::Horizontal);
    }

    // =========================================================================
    // Vertical — tall bar on a 400×2000 canvas
    // =========================================================================
    {
        Spec spec;
        spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
        spec.label    = "Vertical · 400×2000 · 32 px font";
        spec.colormap = cmap;

        renderDual("vertical_hires", spec, style,
                    400, 2000, Orientation::Vertical);
    }

    printf("\nDone.  Open the .ppm files with an image viewer or convert:\n");
    printf("  convert horizontal_hires.ppm horizontal_hires.png\n");
    printf("  convert vertical_hires.ppm   vertical_hires.png\n");
    printf("  The .svg files can be opened directly in a browser.\n");

    return 0;
}
