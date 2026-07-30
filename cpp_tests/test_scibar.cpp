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
