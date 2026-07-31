#include "catch_amalgamated.hpp"
#include "scibar.hpp"

#include <cmath>

TEST_CASE("Color fromHex", "[data]") {
    auto c = scibar::Color::fromHex(0x12345678);
    REQUIRE(c.r == 0x12);
    REQUIRE(c.g == 0x34);
    REQUIRE(c.b == 0x56);
    REQUIRE(c.a == 0x78);
}

TEST_CASE("Color fromFloat and asFloat roundtrip", "[data]") {
    // Roundtrip: float → uint8_t → float should be lossless at endpoints.
    {
        auto c = scibar::Color::fromFloat(0.0f, 0.5f, 1.0f, 1.0f);
        REQUIRE(c.r == 0);
        REQUIRE(c.g == 128);
        REQUIRE(c.b == 255);
        REQUIRE(c.a == 255);

        auto cf = c.asFloat();
        REQUIRE(cf.r == Catch::Approx(0.0f).margin(0.005f));
        REQUIRE(cf.g == Catch::Approx(0.5f).margin(0.005f));
        REQUIRE(cf.b == Catch::Approx(1.0f).margin(0.005f));
        REQUIRE(cf.a == Catch::Approx(1.0f).margin(0.005f));
    }

    // Identity roundtrip for all 256 gray levels.
    for (int i = 0; i <= 255; ++i) {
        float v = i / 255.0f;
        auto c  = scibar::Color::fromFloat(v, v, v);
        auto cf = c.asFloat();
        REQUIRE(c.r == i);
        REQUIRE(cf.r == Catch::Approx(v).margin(0.005f));
    }

    // Default alpha.
    {
        auto c = scibar::Color::fromFloat(0.2f, 0.4f, 0.6f);
        REQUIRE(c.a == 255); // default opaque
    }

    // ColorF default values.
    {
        scibar::ColorF cf;
        REQUIRE(cf.r == 0.0f);
        REQUIRE(cf.g == 0.0f);
        REQUIRE(cf.b == 0.0f);
        REQUIRE(cf.a == 1.0f);
    }
}

TEST_CASE("ColorMapView from vector", "[data]") {
    std::vector<scibar::Color> colors = {
        scibar::Color{255, 0, 0, 255},
        scibar::Color{0, 255, 0, 255},
    };

    scibar::ColorMapView view(colors);
    REQUIRE(view.size == 2);
    REQUIRE(view.data[0].r == 255);
    REQUIRE(view.data[1].g == 255);
}

TEST_CASE("unionRect", "[data]") {
    scibar::Rect a{0, 0, 10, 10};
    scibar::Rect b{5, 5, 10, 10};
    auto u = scibar::unionRect(a, b);
    REQUIRE(u.x == 0);
    REQUIRE(u.y == 0);
    REQUIRE(u.width == 15);
    REQUIRE(u.height == 15);
}

TEST_CASE("Style defaults", "[style]") {
    auto light = scibar::Style::defaultLight();
    REQUIRE(light.frameColor.r == 0x00);
    REQUIRE(light.frameColor.g == 0x00);
    REQUIRE(light.frameColor.b == 0x00);

    auto dark = scibar::Style::defaultDark();
    REQUIRE(dark.frameColor.r == 0xFF);
    REQUIRE(dark.frameColor.g == 0xFF);
    REQUIRE(dark.frameColor.b == 0xFF);
}

TEST_CASE("loadFont and metrics", "[font]") {
    auto font = scibar::loadFont("../fonts/Inter-Regular.ttf", 14.0f);
    REQUIRE(font.handle != nullptr);
    REQUIRE(font.size == 14.0f);

    auto metrics = scibar::fontMetrics(font);
    REQUIRE(metrics.ascender > 0.0f);
    REQUIRE(metrics.descender < 0.0f);
    REQUIRE(metrics.lineHeight > 0.0f);
    REQUIRE(metrics.lineHeight > metrics.ascender);
}

TEST_CASE("measureText", "[font]") {
    auto font = scibar::loadFont("../fonts/Inter-Regular.ttf", 14.0f);

    auto dims = scibar::measureText("Hello", font);
    REQUIRE(dims[0] > 0.0f);  // width
    REQUIRE(dims[1] > 0.0f);  // height
}

TEST_CASE("textAdvance", "[font]") {
    auto font = scibar::loadFont("../fonts/Inter-Regular.ttf", 14.0f);

    // Advance up to "He" should be roughly half of "Hello"
    float advance2  = scibar::textAdvance(font, "Hello", 2);
    float advance5  = scibar::textAdvance(font, "Hello", 5);
    float fullWidth = scibar::measureText("Hello", font)[0];

    REQUIRE(advance2 > 0.0f);
    REQUIRE(advance2 < advance5);
    REQUIRE(advance5 == Catch::Approx(fullWidth).margin(0.5f));
}

TEST_CASE("codepointAdvance", "[font]") {
    auto font = scibar::loadFont("../fonts/Inter-Regular.ttf", 14.0f);

    float adv = scibar::codepointAdvance(font, 'A', 'V');
    REQUIRE(adv > 0.0f);
}

TEST_CASE("generateTicks linear", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min  = 0.0f;
    scale.max  = 10.0f;

    auto ticks = scibar::generateTicks(scale, 6, 2);
    REQUIRE(ticks.size() >= 4);
    REQUIRE(ticks.size() <= 8);

    // First tick should be >= min
    REQUIRE(ticks.front().value >= scale.min);
    // Last tick should be <= max
    REQUIRE(ticks.back().value <= scale.max);

    // Tick values should be monotonically increasing
    for (size_t i = 1; i < ticks.size(); ++i) {
        REQUIRE(ticks[i].value > ticks[i - 1].value);
    }

    // Labels should not be empty
    for (const auto& t : ticks) {
        REQUIRE_FALSE(t.label.empty());
    }
}

TEST_CASE("generateTicks logarithmic", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Logarithmic;
    scale.min  = 1.0f;
    scale.max  = 1000.0f;

    auto ticks = scibar::generateTicks(scale, 5, 3);
    REQUIRE(ticks.size() >= 2);

    // Should include powers of 10: 1, 10, 100, 1000
    bool has1    = false;
    bool has10   = false;
    bool has100  = false;
    bool has1000 = false;
    for (const auto& t : ticks) {
        if (std::abs(t.value - 1.0f)    < 0.01f) has1    = true;
        if (std::abs(t.value - 10.0f)   < 0.01f) has10   = true;
        if (std::abs(t.value - 100.0f)  < 0.01f) has100  = true;
        if (std::abs(t.value - 1000.0f) < 0.01f) has1000 = true;
        REQUIRE_FALSE(t.label.empty());
    }
    REQUIRE(has1);
    REQUIRE(has10);
    REQUIRE(has100);
    REQUIRE(has1000);
}

// Test colormap: use the built-in viridis
static std::vector<scibar::Color> testColormap() {
    return scibar::util::viridis();
}

TEST_CASE("drawColorBar basic", "[draw]") {
    auto cmap = testColormap();

    const int W = 200, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();

    scibar::Rect bounds{50, 50, 40, 500};
    scibar::Rect result = scibar::drawColorBar(cv, bounds, spec, style);

    REQUIRE(result.x == bounds.x);
    REQUIRE(result.y == bounds.y);
    REQUIRE(result.width == bounds.width);
    REQUIRE(result.height == bounds.height);

    // Pixels in the bar area should be non-zero (not all black)
    bool hasColor = false;
    for (int y = 60; y < 540 && !hasColor; ++y) {
        for (int x = 55; x < 85 && !hasColor; ++x) {
            if (buf[y * W + x] != 0) hasColor = true;
        }
    }
    REQUIRE(hasColor);
}

TEST_CASE("drawTitle basic", "[draw]") {
    auto cmap = testColormap();
    auto font = scibar::loadFont("../fonts/Inter-Regular.ttf", 14.0f);

    const int W = 400, H = 100;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.colormap = scibar::ColorMapView(cmap);
    spec.title = "Test Title";

    scibar::Style style = scibar::Style::defaultLight();
    style.font = font;

    scibar::Rect bounds{0, 0, W, H};
    scibar::Rect result = scibar::drawTitle(cv, bounds, spec.title, style);

    REQUIRE(result.width > 0);
    REQUIRE(result.height > 0);
}

TEST_CASE("drawTicks basic", "[draw]") {
    auto cmap = testColormap();
    auto font = scibar::loadFont("../fonts/Inter-Regular.ttf", 14.0f);

    const int W = 400, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();
    style.font = font;

    scibar::Rect barBounds{50, 50, 40, 500};
    scibar::Rect result = scibar::drawTicks(cv, barBounds, spec, style);

    // Tick bounds should extend beyond the bar to the right
    REQUIRE(result.x <= barBounds.x);
    REQUIRE(result.width >= barBounds.width);
}

TEST_CASE("drawLegend basic", "[draw]") {
    auto cmap = testColormap();
    auto font = scibar::loadFont("../fonts/Inter-Regular.ttf", 14.0f);

    const int W = 300, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.title = "Value";
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();
    style.font = font;

    scibar::LayoutResult result = scibar::drawLegend(cv, spec, style);

    REQUIRE(result.totalBoundingBox.width > 0);
    REQUIRE(result.totalBoundingBox.height > 0);
    REQUIRE(result.colorbarBoundingBox.width > 0);
    REQUIRE(result.colorbarBoundingBox.height > 0);
    REQUIRE(result.generatedTickCount > 0);

    // Verify pixels were written (not all zero)
    bool hasContent = false;
    for (size_t i = 0; i < buf.size() && !hasContent; ++i) {
        if (buf[i] != 0) hasContent = true;
    }
    REQUIRE(hasContent);
}

TEST_CASE("viridis colormap", "[util]") {
    auto cmap = scibar::util::viridis();
    REQUIRE(cmap.size() == 256);

    // All entries should be fully opaque
    for (const auto& c : cmap) {
        REQUIRE(c.a == 255);
    }

    // First entry is dark purple
    REQUIRE(cmap[0].r < 80);
    REQUIRE(cmap[0].g < 10);
    REQUIRE(cmap[0].b > 80);

    // Last entry is bright yellow
    REQUIRE(cmap[255].r > 230);
    REQUIRE(cmap[255].g > 220);
    REQUIRE(cmap[255].b < 60);
}

TEST_CASE("vik diverging colormap", "[util]") {
    auto cmap = scibar::util::vik();
    REQUIRE(cmap.size() == 256);

    for (const auto& c : cmap) {
        REQUIRE(c.a == 255);
    }

    // First entry is dark blue
    REQUIRE(cmap[0].b > cmap[0].r);
    // Middle is yellowish (midpoint ~128)
    REQUIRE(cmap[128].g > cmap[128].b);
    // Last entry is reddish
    REQUIRE(cmap[255].r > cmap[255].b);
}

TEST_CASE("generateTicks diverging", "[ticks]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Diverging;
    scale.min  = -3.0f;
    scale.max  = 7.0f;
    scale.midpoint = 2.0f;

    auto ticks = scibar::generateTicks(scale, 5, 2);
    REQUIRE(ticks.size() >= 3);

    // Midpoint should be included
    bool hasMidpoint = false;
    for (const auto& t : ticks) {
        if (std::abs(t.value - 2.0f) < 0.01f) hasMidpoint = true;
    }
    REQUIRE(hasMidpoint);
}

TEST_CASE("ticksInward", "[style]") {
    // Default is outward
    {
        scibar::Style s = scibar::Style::defaultLight();
        REQUIRE(s.ticksInward == false);
    }

    // Verify it can be set to true
    {
        scibar::Style s = scibar::Style::defaultLight();
        s.ticksInward = true;
        REQUIRE(s.ticksInward == true);
    }
}

TEST_CASE("drawTicks inward does not crash and produces bounds", "[draw]") {
    auto cmap = testColormap();
    auto font = scibar::loadFont("../fonts/Inter-Regular.ttf", 14.0f);

    const int W = 400, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();
    style.font = font;
    style.ticksInward = true;

    scibar::Rect barBounds{50, 50, 40, 500};

    // Should not crash with vertical orientation
    scibar::Rect resultV = scibar::drawTicks(cv, barBounds, spec, style,
                                              scibar::Orientation::Vertical);
    REQUIRE(resultV.width >= barBounds.width);

    // Should not crash with horizontal orientation
    scibar::Rect resultH = scibar::drawTicks(cv, barBounds, spec, style,
                                              scibar::Orientation::Horizontal);
    REQUIRE(resultH.height >= barBounds.height);

    // Sub-ticks inward should also not crash
    scibar::drawSubTicks(cv, barBounds, spec, style, scibar::Orientation::Vertical);
    scibar::drawSubTicks(cv, barBounds, spec, style, scibar::Orientation::Horizontal);
}

TEST_CASE("drawColorBar diverging", "[draw]") {
    auto cmap = scibar::util::vik();

    const int W = 200, H = 600;
    std::vector<uint32_t> buf(static_cast<size_t>(W) * H, 0);
    scibar::Canvas cv{buf.data(), W, H};
    scibar::fillCanvas(cv, scibar::Color{255, 255, 255, 255});

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Diverging;
    spec.scale.min  = -3.0f;
    spec.scale.max  = 7.0f;
    spec.scale.midpoint = 2.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Style style = scibar::Style::defaultDark();
    scibar::Rect bounds{50, 50, 40, 500};
    scibar::Rect result = scibar::drawColorBar(cv, bounds, spec, style);

    REQUIRE(result.width == bounds.width);
    REQUIRE(result.height == bounds.height);

    // Pixels in the bar area should be non-zero
    bool hasColor = false;
    for (int y = 60; y < 540 && !hasColor; ++y) {
        if (buf[y * W + 55] != 0xFFFFFFFF) hasColor = true;
    }
    REQUIRE(hasColor);
}

TEST_CASE("exportToSVG basic", "[svg]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.title = "Value";
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.totalWidth = 400;
    opts.totalHeight = 600;
    opts.colorbarBounds = {50, 50, 40, 500};

    std::string svg = scibar::exportToSVG(spec, scibar::Style::defaultLight(), opts);

    REQUIRE_FALSE(svg.empty());
    REQUIRE(svg.find("<svg") != std::string::npos);
    REQUIRE(svg.find("linearGradient") != std::string::npos);
    REQUIRE(svg.find("</svg>") != std::string::npos);
}

TEST_CASE("exportToSVG with hybrid image", "[svg]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 1.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.mainImageHref = "mesh.png";
    opts.mainImageBounds = {0, 0, 400, 400};

    std::string svg = scibar::exportToSVG(spec, scibar::Style::defaultLight(), opts);

    REQUIRE(svg.find("<image") != std::string::npos);
    REQUIRE(svg.find("crisp-edges") != std::string::npos);
}

TEST_CASE("exportToSVG categorical", "[svg]") {
    // Small categorical colormap
    std::vector<scibar::Color> catCmap = {
        scibar::Color{255, 0, 0, 255},
        scibar::Color{0, 255, 0, 255},
        scibar::Color{0, 0, 255, 255},
    };

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Categorical;
    spec.colormap = scibar::ColorMapView(catCmap);

    std::string svg = scibar::exportToSVG(spec);

    // Categorical should NOT have a linearGradient
    REQUIRE(svg.find("linearGradient") == std::string::npos);
    // Should have individual rects
    REQUIRE(svg.find("<rect") != std::string::npos);
    REQUIRE(svg.find("</svg>") != std::string::npos);
}

TEST_CASE("drawColorBar reversed continuous", "[draw][reverse]") {
    auto cmap = testColormap();

    const int W = 200, H = 600;
    auto makeBuf = [&]() {
        std::vector<uint32_t> b(static_cast<size_t>(W) * H, 0);
        return b;
    };

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    // Forward
    auto fwdBuf = makeBuf();
    scibar::Canvas fwdCv{fwdBuf.data(), W, H};
    scibar::Style fwdStyle = scibar::Style::defaultDark();
    fwdStyle.reverseColors = false;
    scibar::Rect bounds{50, 50, 40, 500};
    scibar::drawColorBar(fwdCv, bounds, spec, fwdStyle);

    // Reversed
    auto revBuf = makeBuf();
    scibar::Canvas revCv{revBuf.data(), W, H};
    scibar::Style revStyle = scibar::Style::defaultDark();
    revStyle.reverseColors = true;
    scibar::drawColorBar(revCv, bounds, spec, revStyle);

    // Top and bottom of forward bar should differ from each other
    int topY = 60, botY = 530, midX = 70;
    REQUIRE(fwdBuf[topY * W + midX] != fwdBuf[botY * W + midX]);

    // Top and bottom of reversed bar should differ from each other
    REQUIRE(revBuf[topY * W + midX] != revBuf[botY * W + midX]);

    // Reversed bar should swap the color order: fwd top ≠ rev top, fwd bot ≠ rev bot
    REQUIRE(fwdBuf[topY * W + midX] != revBuf[topY * W + midX]);
    REQUIRE(fwdBuf[botY * W + midX] != revBuf[botY * W + midX]);
}

TEST_CASE("drawColorBar reversed categorical", "[draw][reverse]") {
    std::vector<scibar::Color> catCmap = {
        scibar::Color{255, 0, 0, 255},
        scibar::Color{0, 255, 0, 255},
        scibar::Color{0, 0, 255, 255},
    };

    const int W = 100, H = 300;
    auto makeBuf = [&]() {
        std::vector<uint32_t> b(static_cast<size_t>(W) * H, 0);
        return b;
    };

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Categorical;
    spec.colormap = scibar::ColorMapView(catCmap);

    // Forward
    auto fwdBuf = makeBuf();
    scibar::Canvas fwdCv{fwdBuf.data(), W, H};
    scibar::Style fwdStyle = scibar::Style::defaultDark();
    fwdStyle.reverseColors = false;
    scibar::Rect bounds{25, 0, 50, 300};
    scibar::drawColorBar(fwdCv, bounds, spec, fwdStyle);

    // Reversed
    auto revBuf = makeBuf();
    scibar::Canvas revCv{revBuf.data(), W, H};
    scibar::Style revStyle = scibar::Style::defaultDark();
    revStyle.reverseColors = true;
    scibar::drawColorBar(revCv, bounds, spec, revStyle);

    // Top block of forward (red) should match bottom block of reversed (red)
    int midX = 50;
    int topY = 50, midY = 150, botY = 250;
    REQUIRE(fwdBuf[topY * W + midX] == revBuf[botY * W + midX]);  // fwd top = rev bottom
    REQUIRE(fwdBuf[botY * W + midX] == revBuf[topY * W + midX]);  // fwd bottom = rev top
    // Middle should stay the same (it's the middle category)
    REQUIRE(fwdBuf[midY * W + midX] == revBuf[midY * W + midX]);
}

TEST_CASE("exportToSVG reversed continuous", "[svg][reverse]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.title = "Value";
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.colorbarBounds = {50, 50, 40, 500};

    // Forward: y1="1" ... y2="0" (top-to-bottom gradient)
    scibar::Style fwdStyle = scibar::Style::defaultLight();
    fwdStyle.reverseColors = false;
    std::string fwdSvg = scibar::exportToSVG(spec, fwdStyle, opts);

    // Reversed: y1="0" ... y2="1" (bottom-to-top gradient)
    scibar::Style revStyle = scibar::Style::defaultLight();
    revStyle.reverseColors = true;
    std::string revSvg = scibar::exportToSVG(spec, revStyle, opts);

    // Forward has y1="1" (gradient starts at bottom), reversed has y1="0" (starts at top)
    REQUIRE(fwdSvg.find("y1=\"1\"") != std::string::npos);
    REQUIRE(revSvg.find("y1=\"0\"") != std::string::npos);
}

TEST_CASE("exportToSVG reversed horizontal", "[svg][reverse]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 1.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.colorbarBounds = {50, 50, 500, 40};

    // Forward: x1="0" ... x2="1" (left-to-right)
    scibar::Style fwdStyle = scibar::Style::defaultLight();
    fwdStyle.reverseColors = false;
    std::string fwdSvg = scibar::exportToSVG(spec, fwdStyle, opts,
                                               scibar::Orientation::Horizontal);

    // Reversed: x1="1" ... x2="0" (right-to-left)
    scibar::Style revStyle = scibar::Style::defaultLight();
    revStyle.reverseColors = true;
    std::string revSvg = scibar::exportToSVG(spec, revStyle, opts,
                                               scibar::Orientation::Horizontal);

    // Forward has x1="0" (gradient starts at left), reversed has x1="1" (starts at right)
    REQUIRE(fwdSvg.find("x1=\"0\"") != std::string::npos);
    REQUIRE(revSvg.find("x1=\"1\"") != std::string::npos);
}

TEST_CASE("exportToSVG reversed categorical", "[svg][reverse]") {
    std::vector<scibar::Color> catCmap = {
        scibar::Color{255, 0, 0, 255},
        scibar::Color{0, 255, 0, 255},
        scibar::Color{0, 0, 255, 255},
    };

    scibar::Spec spec;
    spec.scale.type = scibar::ScaleType::Categorical;
    spec.colormap = scibar::ColorMapView(catCmap);

    scibar::Style revStyle = scibar::Style::defaultLight();
    revStyle.reverseColors = true;
    std::string svg = scibar::exportToSVG(spec, revStyle);

    // First rect should be blue (last color in colormap when reversed)
    size_t firstRect = svg.find("<rect");
    REQUIRE(firstRect != std::string::npos);
    // blue = rgb(0,0,255)
    REQUIRE(svg.find("rgb(0,0,255)", firstRect) != std::string::npos);
}

TEST_CASE("generateTicks inverted", "[ticks][inverted]") {
    scibar::Scale scale;
    scale.type = scibar::ScaleType::Linear;
    scale.min   = 0.0f;
    scale.max   = 10.0f;

    auto fwd = scibar::generateTicks(scale, 5, 2);
    REQUIRE(fwd.size() >= 3);
    REQUIRE(fwd.front().value < fwd.back().value);

    scale.inverted = true;
    auto inv = scibar::generateTicks(scale, 5, 2);
    REQUIRE(inv.size() == fwd.size());
    // Inverted ticks should be in descending order
    REQUIRE(inv.front().value > inv.back().value);
}

TEST_CASE("drawColorBar inverted continuous", "[draw][inverted]") {
    auto cmap = testColormap();

    const int W = 200, H = 600;
    auto makeBuf = [&]() {
        std::vector<uint32_t> b(static_cast<size_t>(W) * H, 0);
        return b;
    };

    scibar::Spec fwdSpec;
    fwdSpec.scale.min = 0.0f;
    fwdSpec.scale.max = 100.0f;
    fwdSpec.colormap = scibar::ColorMapView(cmap);

    scibar::Spec invSpec = fwdSpec;
    invSpec.scale.inverted = true;

    scibar::Style style = scibar::Style::defaultDark();
    scibar::Rect bounds{50, 50, 40, 500};

    auto fwdBuf = makeBuf();
    scibar::Canvas fwdCv{fwdBuf.data(), W, H};
    scibar::drawColorBar(fwdCv, bounds, fwdSpec, style);

    auto invBuf = makeBuf();
    scibar::Canvas invCv{invBuf.data(), W, H};
    scibar::drawColorBar(invCv, bounds, invSpec, style);

    int topY = 60, botY = 530, midX = 70;
    // Inverted bar should have colors swapped: top ≠ fwd top, bottom ≠ fwd bottom
    REQUIRE(fwdBuf[topY * W + midX] != invBuf[topY * W + midX]);
    REQUIRE(fwdBuf[botY * W + midX] != invBuf[botY * W + midX]);
}

TEST_CASE("exportToSVG inverted", "[svg][inverted]") {
    auto cmap = testColormap();

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::SVGOptions opts;
    opts.colorbarBounds = {50, 50, 40, 500};
    scibar::Style style = scibar::Style::defaultLight();

    std::string fwdSvg = scibar::exportToSVG(spec, style, opts);

    spec.scale.inverted = true;
    std::string invSvg = scibar::exportToSVG(spec, style, opts);

    // Forward has y1="1" (gradient starts at bottom), inverted has y1="0" (starts at top)
    REQUIRE(fwdSvg.find("y1=\"1\"") != std::string::npos);
    REQUIRE(invSvg.find("y1=\"0\"") != std::string::npos);
}

TEST_CASE("inverted plus reverseColors cancel on continuous", "[draw][inverted]") {
    auto cmap = testColormap();

    const int W = 200, H = 600;
    auto makeBuf = [&]() {
        std::vector<uint32_t> b(static_cast<size_t>(W) * H, 0);
        return b;
    };

    scibar::Spec spec;
    spec.scale.min = 0.0f;
    spec.scale.max = 100.0f;
    spec.colormap = scibar::ColorMapView(cmap);

    scibar::Rect bounds{50, 50, 40, 500};

    // Default: neither flag set
    scibar::Style defStyle = scibar::Style::defaultDark();
    auto defBuf = makeBuf();
    scibar::Canvas defCv{defBuf.data(), W, H};
    scibar::drawColorBar(defCv, bounds, spec, defStyle);

    // Both: inverted + reverseColors — should cancel on continuous scales
    spec.scale.inverted = true;
    scibar::Style bothStyle = scibar::Style::defaultDark();
    bothStyle.reverseColors = true;
    auto bothBuf = makeBuf();
    scibar::Canvas bothCv{bothBuf.data(), W, H};
    scibar::drawColorBar(bothCv, bounds, spec, bothStyle);

    // Continuous: inverted + reverseColors produces same gradient direction as default
    int midX = 70;
    int topY = 60, midY = 325, botY = 530;
    REQUIRE(defBuf[topY * W + midX] == bothBuf[topY * W + midX]);
    REQUIRE(defBuf[midY * W + midX] == bothBuf[midY * W + midX]);
    REQUIRE(defBuf[botY * W + midX] == bothBuf[botY * W + midX]);
}
