/// Pixel-level diagnostic: compare border and tick rendering at the bar edge.
/// Verifies that the frame border and major/minor tick marks share the same
/// anti-aliased coverage (no darkening from path accumulation in canvas_ity).

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar/scibar.hpp"

#include <vector>
#include <cstdio>
#include <cmath>

using namespace scibar;

static void unpackRGBA(uint32_t pixel, int& r, int& g, int& b, int& a) {
    a = static_cast<int>((pixel >> 24) & 0xFF);
    r = static_cast<int>((pixel >> 16) & 0xFF);
    g = static_cast<int>((pixel >>  8) & 0xFF);
    b = static_cast<int>( pixel        & 0xFF);
}

static void scanVertical(const Canvas& canvas, int x, int yStart, int yEnd,
                          const char* label) {
    printf("\n--- %s (x=%d, y=%d..%d) ---\n", label, x, yStart, yEnd);
    for (int y = yStart; y <= yEnd; ++y) {
        uint32_t p = canvas.pixels[y * canvas.width + x];
        int r, g, b, a;
        unpackRGBA(p, r, g, b, a);
        printf("  y=%3d: rgba(%3d,%3d,%3d,%3d)\n", y, r, g, b, a);
    }
}

int main() {
    // =====================================================================
    //  HORIZONTAL COLORBAR
    // =====================================================================
    printf("========================================\n");
    printf("  HORIZONTAL COLORBAR DIAGNOSTICS\n");
    printf("========================================\n");

    {
        const int W = 500, H = 200;
        std::vector<uint32_t> buf(W * H);
        Canvas canvas{buf.data(), W, H};
        fillCanvas(canvas, Color{255, 255, 255, 255});

        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
        spec.label    = "";
        spec.colormap = cmap;

        Style style = Style::defaultLight();
        style.showFrame = true;

        LegendLayout layout = computeLegendLayout(W, H, spec, style,
                                                   Orientation::Horizontal);
        Rect bar = layout.barBounds;
        printf("\nCanvas: %dx%d\n", W, H);
        printf("Bar bounds: x=%d y=%d w=%d h=%d\n", bar.x, bar.y, bar.width, bar.height);
        printf("Left edge: x=%d  Bottom edge: y=%d\n", bar.x, bar.y + bar.height);

        drawColorBar(canvas, bar, spec, style, Orientation::Horizontal);

        auto generatedTicks = generateTicks(spec.scale, 5, style.tickPrecision);
        Spec specWithTicks = spec;
        specWithTicks.ticks = generatedTicks;
        drawTicks(canvas, bar, specWithTicks, style, Orientation::Horizontal);
        drawSubTicks(canvas, bar, specWithTicks, style, Orientation::Horizontal);

        printf("Generated ticks:\n");
        for (const auto& t : generatedTicks) {
            float fraction = (t.value - spec.scale.min) / (spec.scale.max - spec.scale.min);
            float tickX = static_cast<float>(bar.x) +
                          fraction * static_cast<float>(bar.width);
            printf("  value=%.1f label='%s' x_pos=%.1f\n", t.value, t.label.c_str(), tickX);
        }

        // Compare the frame LEFT edge vs the leftmost major tick.
        // Both are vertical lines at x = bar.x. The anti-aliased pixel at
        // x = bar.x - 1 should be identical for both if coverage matches.
        scanVertical(canvas, bar.x - 1,
                     bar.y - 2, bar.y + bar.height + (int)style.tickLength + 2,
                     "ANTI-ALIASED column x=bar.x-1 (frame edge above y=bottom, tick below)");
        scanVertical(canvas, bar.x,
                     bar.y - 2, bar.y + bar.height + (int)style.tickLength + 2,
                     "LINE column x=bar.x (frame edge above y=bottom, tick below)");

        writePPM(canvas, "diagnostic_horizontal.ppm");
        printf("\nWrote diagnostic_horizontal.ppm\n");
    }

    // =====================================================================
    //  VERTICAL COLORBAR
    // =====================================================================
    printf("\n\n========================================\n");
    printf("  VERTICAL COLORBAR DIAGNOSTICS\n");
    printf("========================================\n");

    {
        const int W = 200, H = 500;
        std::vector<uint32_t> buf(W * H);
        Canvas canvas{buf.data(), W, H};
        fillCanvas(canvas, Color{255, 255, 255, 255});

        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
        spec.label    = "";
        spec.colormap = cmap;

        Style style = Style::defaultLight();
        style.showFrame = true;

        LegendLayout layout = computeLegendLayout(W, H, spec, style,
                                                   Orientation::Vertical);
        Rect bar = layout.barBounds;
        printf("\nCanvas: %dx%d\n", W, H);
        printf("Bar bounds: x=%d y=%d w=%d h=%d\n", bar.x, bar.y, bar.width, bar.height);
        printf("Bottom edge: y=%d  Right edge: x=%d\n", bar.y + bar.height, bar.x + bar.width);

        drawColorBar(canvas, bar, spec, style, Orientation::Vertical);

        auto generatedTicks = generateTicks(spec.scale, 5, style.tickPrecision);
        Spec specWithTicks = spec;
        specWithTicks.ticks = generatedTicks;
        drawTicks(canvas, bar, specWithTicks, style, Orientation::Vertical);
        drawSubTicks(canvas, bar, specWithTicks, style, Orientation::Vertical);

        for (const auto& t : generatedTicks) {
            float fraction = (t.value - spec.scale.min) / (spec.scale.max - spec.scale.min);
            float tickY = static_cast<float>(bar.y + bar.height) -
                          fraction * static_cast<float>(bar.height);
            printf("  value=%.1f label='%s' y_pos=%.1f\n", t.value, t.label.c_str(), tickY);
        }

        // Compare the frame BOTTOM edge vs the bottom major tick.
        // Both are horizontal lines at y = bar.y + bar.height.
        int tickLen = (int)style.tickLength;
        printf("\n--- BOTTOM FRAME EDGE + TICK, row y=bar.y+bar.height (x=%d..%d) ---\n",
               bar.x - 2, bar.x + bar.width + tickLen + 2);
        for (int x = bar.x - 2; x <= bar.x + bar.width + tickLen + 2; ++x) {
            uint32_t p = canvas.pixels[(bar.y + bar.height) * canvas.width + x];
            int r, g, b, a;
            unpackRGBA(p, r, g, b, a);
            printf("  x=%3d: rgba(%3d,%3d,%3d,%3d)\n", x, r, g, b, a);
        }

        writePPM(canvas, "diagnostic_vertical.ppm");
        printf("\nWrote diagnostic_vertical.ppm\n");
    }

    printf("\nDone.\n");
    return 0;
}
