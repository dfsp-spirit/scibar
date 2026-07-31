/// scibar gallery — renders 9 horizontal colorbars showcasing diverse features
/// for the README showcase image. Outputs 9 PPM files, composed into a 3×3
/// grid by dev_tools/make_gallery.sh using ImageMagick montage.

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar.hpp"

#include <string>
#include <vector>

using namespace scibar;

static constexpr int kW = 600, kH = 120;

/// Render one panel and write it as PPM.
static void render(const char* filename, const Spec& spec, const Style& style) {
    std::vector<uint32_t> buf(kW * kH);
    Canvas canvas{buf.data(), kW, kH};
    Color bg = (style.textColor.r > 128) ? Color{30, 30, 30, 255}
                                         : Color{255, 255, 255, 255};
    fillCanvas(canvas, bg);

    Rect barRect{50, 45, 500, 30};
    drawColorBar(canvas, barRect, spec, style, Orientation::Horizontal);
    drawTicks(canvas, barRect, spec, style, Orientation::Horizontal);
    if (style.showSubTicks)
        drawSubTicks(canvas, barRect, spec, style, Orientation::Horizontal);

    writePPM(canvas, filename);
}

int main() {
    // ---- Panel 0: Linear, viridis, forward (default) ----
    {
        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
        spec.title    = "Linear · viridis";
        spec.colormap = cmap;
        render("gallery_00.ppm", spec, Style::defaultLight());
    }

    // ---- Panel 1: Linear, viridis, reversed colors ----
    {
        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
        spec.title    = "Linear · reversed";
        spec.colormap = cmap;
        auto style = Style::defaultLight();
        style.reverseColors = true;
        render("gallery_01.ppm", spec, style);
    }

    // ---- Panel 2: Linear, no sub-ticks, ticks outward ----
    {
        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
        spec.title    = "Linear · no sub-ticks";
        spec.colormap = cmap;
        auto style = Style::defaultLight();
        style.showSubTicks = false;
        render("gallery_02.ppm", spec, style);
    }

    // ---- Panel 3: Logarithmic scale ----
    {
        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Logarithmic, 1.0f, 1000.0f};
        spec.title    = "Log · viridis";
        spec.colormap = cmap;
        render("gallery_03.ppm", spec, Style::defaultLight());
    }

    // ---- Panel 4: Diverging, vik, forward, ticks inward ----
    {
        auto cmap = util::vik();
        Spec spec;
        spec.scale    = Scale{ScaleType::Diverging, -5.0f, 5.0f, 0.0f};
        spec.title    = "Diverging · vik";
        spec.colormap = cmap;
        auto style = Style::defaultLight();
        style.ticksInward = true;
        render("gallery_04.ppm", spec, style);
    }

    // ---- Panel 5: Diverging, vik, reversed ----
    {
        auto cmap = util::vik();
        Spec spec;
        spec.scale    = Scale{ScaleType::Diverging, -5.0f, 5.0f, 0.0f};
        spec.title    = "Diverging · reversed";
        spec.colormap = cmap;
        auto style = Style::defaultLight();
        style.reverseColors = true;
        render("gallery_05.ppm", spec, style);
    }

    // ---- Panel 6: Categorical, forward ----
    {
        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Categorical, 0.0f, 8.0f};
        spec.title    = "Categorical · forward";
        spec.colormap = cmap;
        spec.ticks    = {{1.0f, "A"}, {3.0f, "B"}, {5.0f, "C"}, {7.0f, "D"}};
        auto style = Style::defaultLight();
        style.showSubTicks = false;
        render("gallery_06.ppm", spec, style);
    }

    // ---- Panel 7: Categorical, reversed ----
    {
        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Categorical, 0.0f, 8.0f};
        spec.title    = "Categorical · reversed";
        spec.colormap = cmap;
        spec.ticks    = {{1.0f, "A"}, {3.0f, "B"}, {5.0f, "C"}, {7.0f, "D"}};
        auto style = Style::defaultLight();
        style.showSubTicks = false;
        style.reverseColors = true;
        render("gallery_07.ppm", spec, style);
    }

    // ---- Panel 8: Linear, dark theme ----
    {
        auto cmap = util::viridis();
        Spec spec;
        spec.scale    = Scale{ScaleType::Linear, 0.0f, 100.0f};
        spec.title    = "Linear · dark theme";
        spec.colormap = cmap;
        render("gallery_08.ppm", spec, Style::defaultDark());
    }

    return 0;
}
