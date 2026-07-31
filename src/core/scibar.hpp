// scibar — Single-Header Scientific Colorbar Rendering for C++17
// See PLAN3.md for architecture and design rationale.
//
// Usage:
//   In exactly one .cpp file:
//     #define SCIBAR_IMPLEMENTATION
//     #include "scibar.hpp"
//   Everywhere else:
//     #include "scibar.hpp"
//
// Dependencies (vendored in src/third_party/):
//   - canvas_ity.hpp  (ISC license) — 2D rasterization
//   - stb_truetype.h  (Public Domain) — font metrics & glyph rasterization

#ifndef SCIBAR_HPP
#define SCIBAR_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <array>
#include <optional>

namespace scibar {

// =========================================================================
// Enums
// =========================================================================

enum class ScaleType { Linear, Logarithmic, Categorical, Diverging };

enum class Orientation { Vertical, Horizontal };

// =========================================================================
// Data Structures
// =========================================================================

// Float RGBA used for interop with float-based color libraries (e.g., scimesh).
struct ColorF {
    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
};

// Explicit RGBA structure — eliminates endianness bugs across platforms.
// byte 0 (LSB) = R, byte 1 = G, byte 2 = B, byte 3 (MSB) = A.
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    static constexpr Color fromHex(uint32_t hex) {
        return { uint8_t(hex >> 24), uint8_t(hex >> 16),
                 uint8_t(hex >> 8),  uint8_t(hex & 0xFF) };
    }

    // Construct from float channels in [0, 1] range.
    static constexpr Color fromFloat(float r, float g, float b, float a = 1.0f) {
        return { uint8_t(r * 255.0f + 0.5f), uint8_t(g * 255.0f + 0.5f),
                 uint8_t(b * 255.0f + 0.5f), uint8_t(a * 255.0f + 0.5f) };
    }

    // Convert to float representation for interop.
    constexpr ColorF asFloat() const {
        return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
    }
};

struct Scale {
    ScaleType type = ScaleType::Linear;
    float min = 0.0f;
    float max = 1.0f;
    float midpoint = 0.0f; // For diverging scales or log shifts
    bool inverted = false;  // If true, max→min runs visually start→end of bar
};

struct Font {
    const void* handle = nullptr; // nullptr = use embedded Inter font (Tier 2, future)
    float size = 14.0f;
};

struct FontMetrics {
    float ascender   = 0.0f; // Distance from baseline to top of tallest glyph
    float descender  = 0.0f; // Distance from baseline to bottom (negative)
    float lineHeight = 0.0f; // Recommended line spacing
};

struct Tick {
    float value = 0.0f;
    std::string label;
};

/// Sub-tick (minor tick) — an unlabeled tick mark drawn between major ticks.
/// Typically shorter than major ticks and drawn without a text label.
struct SubTick {
    float value = 0.0f;
};

// Non-owning colormap view — avoids heap allocation per frame.
// Implicitly constructible from std::vector<Color> for ergonomic usage.
// Rvalue vector constructor deleted to prevent dangling from temporaries.
struct ColorMapView {
    const Color* data = nullptr;
    size_t size = 0;

    ColorMapView() = default;
    ColorMapView(const std::vector<Color>& v) : data(v.data()), size(v.size()) {}
    ColorMapView(std::vector<Color>&&) = delete;
    ColorMapView(const Color* d, size_t s) : data(d), size(s) {}
};

struct Spec {
    Scale scale;
    ColorMapView colormap;
    std::string title;
    std::vector<Tick>    ticks;    // Custom major ticks; auto-generated via generateTicks() if empty
    std::vector<SubTick> subTicks; // Custom sub-ticks; auto-generated via generateSubTicks() if empty
};

struct Style {
    bool  showFrame     = true;
    Color frameColor    = Color::fromHex(0x000000FF);
    Color tickColor     = Color::fromHex(0x000000FF);
    Color textColor     = Color::fromHex(0x000000FF);
    Font  font;

    float tickLength         = 5.0f;  // Outward tick mark length in pixels
    float subTickLength      = 3.0f;  // Sub-tick mark length in pixels (shorter than tickLength)
    int   tickPrecision      = 6;     // Significant digits for auto-generated tick labels (%.*g)
    int   subTicksPerInterval = 4;    // Number of sub-ticks between major ticks (linear/diverging only)
    bool  showSubTicks       = true;  // If false, sub-ticks are not drawn
    bool  reverseColors      = false; // If true, flip colormap direction (low→high becomes high→low)

    static Style defaultLight();
    static Style defaultDark();
};

struct Canvas {
    // pixels[] is packed uint32_t in RGBA byte order:
    // byte 0 (LSB) = R, byte 1 = G, byte 2 = B, byte 3 (MSB) = A.
    uint32_t* pixels = nullptr;
    int width  = 0;
    int height = 0;
};

struct Rect {
    int x = 0, y = 0, width = 0, height = 0;
};

inline Rect unionRect(const Rect& a, const Rect& b) {
    if (a.width <= 0 || a.height <= 0) return b;
    if (b.width <= 0 || b.height <= 0) return a;
    int xMin = a.x < b.x ? a.x : b.x;
    int yMin = a.y < b.y ? a.y : b.y;
    int xMax = (a.x + a.width)  > (b.x + b.width)  ? (a.x + a.width)  : (b.x + b.width);
    int yMax = (a.y + a.height) > (b.y + b.height) ? (a.y + a.height) : (b.y + b.height);
    return { xMin, yMin, xMax - xMin, yMax - yMin };
}

struct LayoutResult {
    Rect totalBoundingBox;
    Rect colorbarBoundingBox;
    int  generatedTickCount = 0;
};

struct SVGOptions {
    int totalWidth  = 800;
    int totalHeight = 600;

    std::string mainImageHref = ""; // Local path or "data:image/png;base64,..."
    Rect mainImageBounds = {20, 20, 550, 550};
    Rect colorbarBounds   = {600, 50, 150, 500};
};

// =========================================================================
// API Declarations
// =========================================================================

// --- Font loading ---
Font loadFont(const char* ttfFilePath, float size = 14.0f);

// --- Text measurement ---
std::array<float, 2> measureText(const std::string& text, const Font& font);
FontMetrics          fontMetrics(const Font& font);
float textAdvance(const Font& font, const std::string& text, int upToIndex);
float codepointAdvance(const Font& font, int leftCodepoint, int rightCodepoint);

// --- Tick generation ---
std::vector<Tick>    generateTicks(const Scale& scale, int targetCount = 5, int precision = 6);
std::vector<SubTick> generateSubTicks(const Scale& scale,
                                       const std::vector<Tick>& majorTicks,
                                       int subTicksPerInterval = 4);

// --- Low-level drawing (pixel backend) ---
void fillCanvas(Canvas& canvas, Color color);
Rect drawColorBar(Canvas& canvas, Rect bounds, const Spec& spec, const Style& style,
                  Orientation orientation = Orientation::Vertical);
Rect drawTicks(Canvas& canvas, Rect barBounds, const Spec& spec, const Style& style,
               Orientation orientation = Orientation::Vertical);
Rect drawSubTicks(Canvas& canvas, Rect barBounds, const Spec& spec, const Style& style,
                  Orientation orientation = Orientation::Vertical);
Rect drawTitle(Canvas& canvas, Rect bounds, const std::string& title, const Style& style);

// --- High-level convenience ---
LayoutResult drawLegend(Canvas& canvas, const Spec& spec,
                        const Style& style = Style::defaultLight());

// --- SVG export ---
std::string exportToSVG(const Spec& spec,
                        const Style& style = Style::defaultLight(),
                        const SVGOptions& options = {},
                        Orientation orientation = Orientation::Vertical);

} // namespace scibar

// =========================================================================
// Built-in colormaps (utility namespace)
// =========================================================================

namespace scibar::util {

/// @brief  The Viridis perceptually-uniform colormap (256 entries).
///
/// Returns a 256-entry RGBA lookup table suitable for Spec::colormap.
/// Data source: matplotlib viridis, identical to the official reference.
inline std::vector<scibar::Color> viridis() {
    static const float lut[] = {
        0.267004f, 0.004874f, 0.329415f, 0.268510f, 0.009605f, 0.335427f,
        0.269944f, 0.014625f, 0.341379f, 0.271305f, 0.019942f, 0.347269f,
        0.272594f, 0.025563f, 0.353093f, 0.273809f, 0.031497f, 0.358853f,
        0.274952f, 0.037752f, 0.364543f, 0.276022f, 0.044167f, 0.370164f,
        0.277018f, 0.050344f, 0.375715f, 0.277941f, 0.056324f, 0.381191f,
        0.278791f, 0.062145f, 0.386592f, 0.279566f, 0.067836f, 0.391917f,
        0.280267f, 0.073417f, 0.397163f, 0.280894f, 0.078907f, 0.402329f,
        0.281446f, 0.084320f, 0.407414f, 0.281924f, 0.089666f, 0.412415f,
        0.282327f, 0.094955f, 0.417331f, 0.282656f, 0.100196f, 0.422160f,
        0.282910f, 0.105393f, 0.426902f, 0.283091f, 0.110553f, 0.431554f,
        0.283197f, 0.115680f, 0.436115f, 0.283229f, 0.120777f, 0.440584f,
        0.283187f, 0.125848f, 0.444960f, 0.283072f, 0.130895f, 0.449241f,
        0.282884f, 0.135920f, 0.453427f, 0.282623f, 0.140926f, 0.457517f,
        0.282290f, 0.145912f, 0.461510f, 0.281887f, 0.150881f, 0.465405f,
        0.281412f, 0.155834f, 0.469201f, 0.280868f, 0.160771f, 0.472899f,
        0.280255f, 0.165693f, 0.476498f, 0.279574f, 0.170599f, 0.479997f,
        0.278826f, 0.175490f, 0.483397f, 0.278012f, 0.180367f, 0.486697f,
        0.277134f, 0.185228f, 0.489898f, 0.276194f, 0.190074f, 0.493001f,
        0.275191f, 0.194905f, 0.496005f, 0.274128f, 0.199721f, 0.498911f,
        0.273006f, 0.204520f, 0.501721f, 0.271828f, 0.209303f, 0.504434f,
        0.270595f, 0.214069f, 0.507052f, 0.269308f, 0.218818f, 0.509577f,
        0.267968f, 0.223549f, 0.512008f, 0.266580f, 0.228262f, 0.514349f,
        0.265145f, 0.232956f, 0.516599f, 0.263663f, 0.237631f, 0.518762f,
        0.262138f, 0.242286f, 0.520837f, 0.260571f, 0.246922f, 0.522828f,
        0.258965f, 0.251537f, 0.524736f, 0.257322f, 0.256130f, 0.526563f,
        0.255645f, 0.260703f, 0.528312f, 0.253935f, 0.265254f, 0.529983f,
        0.252194f, 0.269783f, 0.531579f, 0.250425f, 0.274290f, 0.533103f,
        0.248629f, 0.278775f, 0.534556f, 0.246811f, 0.283237f, 0.535941f,
        0.244972f, 0.287675f, 0.537260f, 0.243113f, 0.292092f, 0.538516f,
        0.241237f, 0.296485f, 0.539709f, 0.239346f, 0.300855f, 0.540844f,
        0.237441f, 0.305202f, 0.541921f, 0.235526f, 0.309527f, 0.542944f,
        0.233603f, 0.313828f, 0.543914f, 0.231674f, 0.318106f, 0.544834f,
        0.229739f, 0.322361f, 0.545706f, 0.227802f, 0.326594f, 0.546532f,
        0.225863f, 0.330805f, 0.547314f, 0.223925f, 0.334994f, 0.548053f,
        0.221989f, 0.339161f, 0.548752f, 0.220057f, 0.343307f, 0.549413f,
        0.218130f, 0.347432f, 0.550038f, 0.216210f, 0.351535f, 0.550627f,
        0.214298f, 0.355619f, 0.551184f, 0.212395f, 0.359683f, 0.551710f,
        0.210503f, 0.363727f, 0.552206f, 0.208623f, 0.367752f, 0.552675f,
        0.206756f, 0.371758f, 0.553117f, 0.204903f, 0.375746f, 0.553533f,
        0.203063f, 0.379716f, 0.553925f, 0.201239f, 0.383670f, 0.554294f,
        0.199430f, 0.387607f, 0.554642f, 0.197636f, 0.391528f, 0.554969f,
        0.195860f, 0.395433f, 0.555276f, 0.194100f, 0.399323f, 0.555565f,
        0.192357f, 0.403199f, 0.555836f, 0.190631f, 0.407061f, 0.556089f,
        0.188923f, 0.410910f, 0.556326f, 0.187231f, 0.414746f, 0.556547f,
        0.185556f, 0.418570f, 0.556753f, 0.183898f, 0.422383f, 0.556944f,
        0.182256f, 0.426184f, 0.557120f, 0.180629f, 0.429975f, 0.557282f,
        0.179019f, 0.433756f, 0.557430f, 0.177423f, 0.437527f, 0.557565f,
        0.175841f, 0.441290f, 0.557685f, 0.174274f, 0.445044f, 0.557792f,
        0.172719f, 0.448791f, 0.557885f, 0.171176f, 0.452530f, 0.557965f,
        0.169646f, 0.456262f, 0.558030f, 0.168126f, 0.459988f, 0.558082f,
        0.166617f, 0.463708f, 0.558119f, 0.165117f, 0.467423f, 0.558141f,
        0.163625f, 0.471133f, 0.558148f, 0.162142f, 0.474838f, 0.558140f,
        0.160665f, 0.478540f, 0.558115f, 0.159194f, 0.482237f, 0.558073f,
        0.157729f, 0.485932f, 0.558013f, 0.156270f, 0.489624f, 0.557936f,
        0.154815f, 0.493313f, 0.557840f, 0.153364f, 0.497000f, 0.557724f,
        0.151918f, 0.500685f, 0.557587f, 0.150476f, 0.504369f, 0.557430f,
        0.149039f, 0.508051f, 0.557250f, 0.147607f, 0.511733f, 0.557049f,
        0.146180f, 0.515413f, 0.556823f, 0.144759f, 0.519093f, 0.556572f,
        0.143343f, 0.522773f, 0.556295f, 0.141935f, 0.526453f, 0.555991f,
        0.140536f, 0.530132f, 0.555659f, 0.139147f, 0.533812f, 0.555298f,
        0.137770f, 0.537492f, 0.554906f, 0.136408f, 0.541173f, 0.554483f,
        0.135066f, 0.544853f, 0.554029f, 0.133743f, 0.548535f, 0.553541f,
        0.132444f, 0.552216f, 0.553018f, 0.131172f, 0.555899f, 0.552459f,
        0.129933f, 0.559582f, 0.551864f, 0.128729f, 0.563265f, 0.551229f,
        0.127568f, 0.566949f, 0.550556f, 0.126453f, 0.570633f, 0.549841f,
        0.125394f, 0.574318f, 0.549086f, 0.124395f, 0.578002f, 0.548287f,
        0.123463f, 0.581687f, 0.547445f, 0.122606f, 0.585371f, 0.546557f,
        0.121831f, 0.589055f, 0.545623f, 0.121148f, 0.592739f, 0.544641f,
        0.120565f, 0.596422f, 0.543611f, 0.120092f, 0.600104f, 0.542530f,
        0.119738f, 0.603785f, 0.541400f, 0.119512f, 0.607464f, 0.540218f,
        0.119423f, 0.611141f, 0.538982f, 0.119483f, 0.614817f, 0.537692f,
        0.119699f, 0.618490f, 0.536347f, 0.120081f, 0.622161f, 0.534946f,
        0.120638f, 0.625828f, 0.533488f, 0.121380f, 0.629492f, 0.531973f,
        0.122312f, 0.633153f, 0.530398f, 0.123444f, 0.636809f, 0.528763f,
        0.124780f, 0.640461f, 0.527068f, 0.126326f, 0.644107f, 0.525311f,
        0.128087f, 0.647749f, 0.523491f, 0.130067f, 0.651384f, 0.521608f,
        0.132268f, 0.655014f, 0.519661f, 0.134692f, 0.658636f, 0.517649f,
        0.137339f, 0.662252f, 0.515571f, 0.140210f, 0.665859f, 0.513427f,
        0.143303f, 0.669459f, 0.511215f, 0.146616f, 0.673050f, 0.508936f,
        0.150148f, 0.676631f, 0.506589f, 0.153894f, 0.680203f, 0.504172f,
        0.157851f, 0.683765f, 0.501686f, 0.162016f, 0.687316f, 0.499129f,
        0.166383f, 0.690856f, 0.496502f, 0.170948f, 0.694384f, 0.493803f,
        0.175707f, 0.697900f, 0.491033f, 0.180653f, 0.701402f, 0.488189f,
        0.185783f, 0.704891f, 0.485273f, 0.191090f, 0.708366f, 0.482284f,
        0.196571f, 0.711827f, 0.479221f, 0.202219f, 0.715272f, 0.476084f,
        0.208030f, 0.718701f, 0.472873f, 0.214000f, 0.722114f, 0.469588f,
        0.220124f, 0.725509f, 0.466226f, 0.226397f, 0.728888f, 0.462789f,
        0.232815f, 0.732247f, 0.459277f, 0.239374f, 0.735588f, 0.455688f,
        0.246070f, 0.738910f, 0.452024f, 0.252899f, 0.742211f, 0.448284f,
        0.259857f, 0.745492f, 0.444467f, 0.266941f, 0.748751f, 0.440573f,
        0.274149f, 0.751988f, 0.436601f, 0.281477f, 0.755203f, 0.432552f,
        0.288921f, 0.758394f, 0.428426f, 0.296479f, 0.761561f, 0.424223f,
        0.304148f, 0.764704f, 0.419943f, 0.311925f, 0.767822f, 0.415586f,
        0.319809f, 0.770914f, 0.411152f, 0.327796f, 0.773980f, 0.406640f,
        0.335885f, 0.777018f, 0.402049f, 0.344074f, 0.780029f, 0.397381f,
        0.352360f, 0.783011f, 0.392636f, 0.360741f, 0.785964f, 0.387814f,
        0.369214f, 0.788888f, 0.382914f, 0.377779f, 0.791781f, 0.377939f,
        0.386433f, 0.794644f, 0.372886f, 0.395174f, 0.797475f, 0.367757f,
        0.404001f, 0.800275f, 0.362552f, 0.412913f, 0.803041f, 0.357269f,
        0.421908f, 0.805774f, 0.351910f, 0.430983f, 0.808473f, 0.346476f,
        0.440137f, 0.811138f, 0.340967f, 0.449368f, 0.813768f, 0.335384f,
        0.458674f, 0.816363f, 0.329727f, 0.468053f, 0.818921f, 0.323998f,
        0.477504f, 0.821444f, 0.318195f, 0.487026f, 0.823929f, 0.312321f,
        0.496615f, 0.826376f, 0.306377f, 0.506271f, 0.828786f, 0.300362f,
        0.515992f, 0.831158f, 0.294279f, 0.525776f, 0.833491f, 0.288127f,
        0.535621f, 0.835785f, 0.281908f, 0.545524f, 0.838039f, 0.275626f,
        0.555484f, 0.840254f, 0.269281f, 0.565498f, 0.842430f, 0.262877f,
        0.575563f, 0.844566f, 0.256415f, 0.585678f, 0.846661f, 0.249897f,
        0.595839f, 0.848717f, 0.243329f, 0.606045f, 0.850733f, 0.236712f,
        0.616293f, 0.852709f, 0.230052f, 0.626579f, 0.854645f, 0.223353f,
        0.636902f, 0.856542f, 0.216620f, 0.647257f, 0.858400f, 0.209861f,
        0.657642f, 0.860219f, 0.203082f, 0.668054f, 0.861999f, 0.196293f,
        0.678489f, 0.863742f, 0.189503f, 0.688944f, 0.865448f, 0.182725f,
        0.699415f, 0.867117f, 0.175971f, 0.709898f, 0.868751f, 0.169257f,
        0.720391f, 0.870350f, 0.162603f, 0.730889f, 0.871916f, 0.156029f,
        0.741388f, 0.873449f, 0.149561f, 0.751884f, 0.874951f, 0.143228f,
        0.762373f, 0.876424f, 0.137064f, 0.772852f, 0.877868f, 0.131109f,
        0.783315f, 0.879285f, 0.125405f, 0.793760f, 0.880678f, 0.120005f,
        0.804182f, 0.882046f, 0.114965f, 0.814576f, 0.883393f, 0.110347f,
        0.824940f, 0.884720f, 0.106217f, 0.835270f, 0.886029f, 0.102646f,
        0.845561f, 0.887322f, 0.099702f, 0.855810f, 0.888601f, 0.097452f,
        0.866013f, 0.889868f, 0.095953f, 0.876168f, 0.891125f, 0.095250f,
        0.886271f, 0.892374f, 0.095374f, 0.896320f, 0.893616f, 0.096335f,
        0.906311f, 0.894855f, 0.098125f, 0.916242f, 0.896091f, 0.100717f,
        0.926106f, 0.897330f, 0.104071f, 0.935904f, 0.898570f, 0.108131f,
        0.945636f, 0.899815f, 0.112838f, 0.955300f, 0.901065f, 0.118128f,
        0.964894f, 0.902323f, 0.123941f, 0.974417f, 0.903590f, 0.130215f,
        0.983868f, 0.904867f, 0.136897f, 0.993248f, 0.906157f, 0.143936f
    };

    std::vector<scibar::Color> cmap(256);
    for (int i = 0; i < 256; ++i) {
        cmap[i] = scibar::Color{
            static_cast<uint8_t>(lut[i * 3 + 0] * 255.0f),
            static_cast<uint8_t>(lut[i * 3 + 1] * 255.0f),
            static_cast<uint8_t>(lut[i * 3 + 2] * 255.0f),
            255
        };
    }
    return cmap;
}

/// @brief  The Vik (Crameri) perceptually-uniform diverging colormap (256 entries).
///
/// Returns a 256-entry RGBA lookup table suitable for Spec::colormap
/// with ScaleType::Diverging. Vik is a blue→yellow→red diverging colormap
/// designed for scientific visualization. CVD-friendly, no false banding.
/// Data source: Fabio Crameri, ScientificColourMaps7 (CC0 license).
inline std::vector<scibar::Color> vik() {
    static const float lut[] = {
        0.001328f, 0.069836f, 0.379529f, 0.002366f, 0.076475f, 0.383518f,
        0.003304f, 0.083083f, 0.387487f, 0.004146f, 0.089590f, 0.391477f,
        0.004897f, 0.095948f, 0.395453f, 0.005563f, 0.102274f, 0.399409f,
        0.006151f, 0.108500f, 0.403388f, 0.006668f, 0.114686f, 0.407339f,
        0.007119f, 0.120845f, 0.411288f, 0.007512f, 0.126958f, 0.415230f,
        0.007850f, 0.133068f, 0.419166f, 0.008141f, 0.139092f, 0.423079f,
        0.008391f, 0.145171f, 0.427006f, 0.008606f, 0.151144f, 0.430910f,
        0.008790f, 0.157140f, 0.434809f, 0.008947f, 0.163152f, 0.438691f,
        0.009080f, 0.169142f, 0.442587f, 0.009193f, 0.175103f, 0.446459f,
        0.009290f, 0.181052f, 0.450337f, 0.009372f, 0.187051f, 0.454212f,
        0.009443f, 0.193028f, 0.458077f, 0.009506f, 0.198999f, 0.461951f,
        0.009564f, 0.205011f, 0.465816f, 0.009619f, 0.211021f, 0.469707f,
        0.009675f, 0.217047f, 0.473571f, 0.009735f, 0.223084f, 0.477461f,
        0.009802f, 0.229123f, 0.481352f, 0.009881f, 0.235206f, 0.485250f,
        0.009977f, 0.241277f, 0.489161f, 0.010098f, 0.247386f, 0.493080f,
        0.010254f, 0.253516f, 0.497020f, 0.010463f, 0.259675f, 0.500974f,
        0.010755f, 0.265853f, 0.504938f, 0.011176f, 0.272037f, 0.508925f,
        0.011716f, 0.278296f, 0.512923f, 0.012286f, 0.284554f, 0.516953f,
        0.012934f, 0.290865f, 0.520998f, 0.013790f, 0.297214f, 0.525074f,
        0.014838f, 0.303577f, 0.529184f, 0.016131f, 0.310015f, 0.533308f,
        0.017711f, 0.316474f, 0.537485f, 0.019630f, 0.322986f, 0.541677f,
        0.021948f, 0.329550f, 0.545931f, 0.024730f, 0.336144f, 0.550210f,
        0.028047f, 0.342826f, 0.554538f, 0.031980f, 0.349543f, 0.558906f,
        0.036812f, 0.356332f, 0.563341f, 0.042229f, 0.363171f, 0.567811f,
        0.048008f, 0.370086f, 0.572345f, 0.054292f, 0.377080f, 0.576933f,
        0.060963f, 0.384129f, 0.581571f, 0.068081f, 0.391265f, 0.586280f,
        0.075457f, 0.398460f, 0.591042f, 0.083246f, 0.405740f, 0.595868f,
        0.091425f, 0.413088f, 0.600754f, 0.099832f, 0.420499f, 0.605697f,
        0.108595f, 0.428000f, 0.610711f, 0.117694f, 0.435566f, 0.615770f,
        0.127042f, 0.443194f, 0.620895f, 0.136702f, 0.450888f, 0.626062f,
        0.146607f, 0.458643f, 0.631289f, 0.156787f, 0.466457f, 0.636560f,
        0.167187f, 0.474324f, 0.641866f, 0.177807f, 0.482238f, 0.647218f,
        0.188606f, 0.490191f, 0.652599f, 0.199580f, 0.498193f, 0.658021f,
        0.210783f, 0.506201f, 0.663465f, 0.222120f, 0.514263f, 0.668924f,
        0.233602f, 0.522322f, 0.674403f, 0.245231f, 0.530414f, 0.679894f,
        0.256999f, 0.538517f, 0.685405f, 0.268867f, 0.546617f, 0.690908f,
        0.280797f, 0.554717f, 0.696428f, 0.292852f, 0.562822f, 0.701935f,
        0.304985f, 0.570907f, 0.707448f, 0.317174f, 0.578997f, 0.712950f,
        0.329438f, 0.587064f, 0.718447f, 0.341729f, 0.595123f, 0.723934f,
        0.354067f, 0.603164f, 0.729412f, 0.366459f, 0.611186f, 0.734877f,
        0.378862f, 0.619189f, 0.740325f, 0.391305f, 0.627159f, 0.745757f,
        0.403760f, 0.635114f, 0.751183f, 0.416227f, 0.643046f, 0.756582f,
        0.428711f, 0.650956f, 0.761968f, 0.441199f, 0.658836f, 0.767341f,
        0.453697f, 0.666696f, 0.772699f, 0.466195f, 0.674537f, 0.778044f,
        0.478697f, 0.682349f, 0.783369f, 0.491208f, 0.690143f, 0.788682f,
        0.503691f, 0.697910f, 0.793980f, 0.516178f, 0.705661f, 0.799260f,
        0.528677f, 0.713387f, 0.804525f, 0.541149f, 0.721090f, 0.809775f,
        0.553624f, 0.728778f, 0.815010f, 0.566096f, 0.736441f, 0.820229f,
        0.578557f, 0.744089f, 0.825435f, 0.591014f, 0.751718f, 0.830626f,
        0.603468f, 0.759314f, 0.835793f, 0.615908f, 0.766896f, 0.840941f,
        0.628351f, 0.774452f, 0.846058f, 0.640779f, 0.781988f, 0.851147f,
        0.653203f, 0.789485f, 0.856206f, 0.665631f, 0.796945f, 0.861214f,
        0.678051f, 0.804371f, 0.866172f, 0.690457f, 0.811742f, 0.871059f,
        0.702868f, 0.819048f, 0.875866f, 0.715265f, 0.826290f, 0.880567f,
        0.727646f, 0.833439f, 0.885146f, 0.740019f, 0.840479f, 0.889570f,
        0.752354f, 0.847380f, 0.893807f, 0.764662f, 0.854125f, 0.897821f,
        0.776918f, 0.860678f, 0.901565f, 0.789096f, 0.866991f, 0.904992f,
        0.801170f, 0.873031f, 0.908043f, 0.813110f, 0.878738f, 0.910653f,
        0.824870f, 0.884062f, 0.912761f, 0.836396f, 0.888934f, 0.914302f,
        0.847617f, 0.893289f, 0.915195f, 0.858470f, 0.897074f, 0.915385f,
        0.868874f, 0.900206f, 0.914812f, 0.878729f, 0.902636f, 0.913418f,
        0.887965f, 0.904303f, 0.911164f, 0.896497f, 0.905178f, 0.908034f,
        0.904242f, 0.905221f, 0.904013f, 0.911151f, 0.904422f, 0.899132f,
        0.917175f, 0.902800f, 0.893409f, 0.922285f, 0.900367f, 0.886911f,
        0.926482f, 0.897173f, 0.879687f, 0.929789f, 0.893256f, 0.871826f,
        0.932236f, 0.888698f, 0.863396f, 0.933880f, 0.883552f, 0.854476f,
        0.934782f, 0.877893f, 0.845152f, 0.935013f, 0.871795f, 0.835493f,
        0.934644f, 0.865313f, 0.825561f, 0.933752f, 0.858522f, 0.815421f,
        0.932408f, 0.851469f, 0.805112f, 0.930682f, 0.844208f, 0.794685f,
        0.928622f, 0.836778f, 0.784169f, 0.926298f, 0.829215f, 0.773579f,
        0.923752f, 0.821545f, 0.762958f, 0.921017f, 0.813795f, 0.752313f,
        0.918147f, 0.805997f, 0.741659f, 0.915156f, 0.798157f, 0.731008f,
        0.912080f, 0.790294f, 0.720370f, 0.908933f, 0.782421f, 0.709752f,
        0.905741f, 0.774540f, 0.699150f, 0.902506f, 0.766670f, 0.688588f,
        0.899249f, 0.758812f, 0.678051f, 0.895973f, 0.750973f, 0.667550f,
        0.892690f, 0.743148f, 0.657086f, 0.889402f, 0.735345f, 0.646657f,
        0.886118f, 0.727569f, 0.636274f, 0.882831f, 0.719826f, 0.625923f,
        0.879556f, 0.712106f, 0.615618f, 0.876289f, 0.704419f, 0.605357f,
        0.873033f, 0.696764f, 0.595141f, 0.869784f, 0.689144f, 0.584972f,
        0.866551f, 0.681541f, 0.574832f, 0.863333f, 0.673985f, 0.564746f,
        0.860121f, 0.666453f, 0.554708f, 0.856920f, 0.658957f, 0.544709f,
        0.853732f, 0.651500f, 0.534753f, 0.850562f, 0.644061f, 0.524842f,
        0.847402f, 0.636670f, 0.514974f, 0.844258f, 0.629296f, 0.505146f,
        0.841125f, 0.621957f, 0.495369f, 0.838005f, 0.614653f, 0.485627f,
        0.834895f, 0.607392f, 0.475941f, 0.831802f, 0.600144f, 0.466284f,
        0.828715f, 0.592938f, 0.456675f, 0.825639f, 0.585758f, 0.447109f,
        0.822582f, 0.578600f, 0.437595f, 0.819528f, 0.571478f, 0.428106f,
        0.816496f, 0.564388f, 0.418657f, 0.813463f, 0.557328f, 0.409260f,
        0.810446f, 0.550285f, 0.399892f, 0.807443f, 0.543274f, 0.390575f,
        0.804446f, 0.536288f, 0.381299f, 0.801454f, 0.529329f, 0.372040f,
        0.798475f, 0.522380f, 0.362835f, 0.795500f, 0.515460f, 0.353660f,
        0.792535f, 0.508575f, 0.344523f, 0.789573f, 0.501692f, 0.335435f,
        0.786617f, 0.494827f, 0.326343f, 0.783657f, 0.487977f, 0.317312f,
        0.780695f, 0.481123f, 0.308300f, 0.777737f, 0.474295f, 0.299327f,
        0.774763f, 0.467464f, 0.290352f, 0.771788f, 0.460620f, 0.281424f,
        0.768787f, 0.453783f, 0.272508f, 0.765776f, 0.446929f, 0.263640f,
        0.762724f, 0.440055f, 0.254764f, 0.759638f, 0.433147f, 0.245872f,
        0.756510f, 0.426200f, 0.237047f, 0.753316f, 0.419216f, 0.228190f,
        0.750051f, 0.412163f, 0.219330f, 0.746698f, 0.405028f, 0.210470f,
        0.743239f, 0.397819f, 0.201593f, 0.739651f, 0.390493f, 0.192739f,
        0.735899f, 0.383060f, 0.183852f, 0.731988f, 0.375473f, 0.174977f,
        0.727865f, 0.367743f, 0.166045f, 0.723516f, 0.359852f, 0.157131f,
        0.718915f, 0.351766f, 0.148211f, 0.714028f, 0.343503f, 0.139282f,
        0.708841f, 0.335048f, 0.130458f, 0.703318f, 0.326354f, 0.121545f,
        0.697448f, 0.317502f, 0.112841f, 0.691227f, 0.308462f, 0.104132f,
        0.684653f, 0.299264f, 0.095633f, 0.677734f, 0.289916f, 0.087350f,
        0.670476f, 0.280477f, 0.079197f, 0.662904f, 0.271015f, 0.071510f,
        0.655048f, 0.261520f, 0.064079f, 0.646969f, 0.252081f, 0.057104f,
        0.638686f, 0.242711f, 0.050618f, 0.630261f, 0.233488f, 0.044750f,
        0.621722f, 0.224449f, 0.039414f, 0.613135f, 0.215657f, 0.034829f,
        0.604539f, 0.207086f, 0.031072f, 0.595947f, 0.198741f, 0.028212f,
        0.587403f, 0.190700f, 0.026019f, 0.578937f, 0.182918f, 0.024396f,
        0.570545f, 0.175423f, 0.023257f, 0.562268f, 0.168171f, 0.022523f,
        0.554076f, 0.161202f, 0.022110f, 0.546007f, 0.154400f, 0.021861f,
        0.538043f, 0.147854f, 0.021737f, 0.530182f, 0.141491f, 0.021722f,
        0.522424f, 0.135276f, 0.021800f, 0.514776f, 0.129209f, 0.021957f,
        0.507213f, 0.123272f, 0.022179f, 0.499733f, 0.117487f, 0.022455f,
        0.492348f, 0.111818f, 0.022775f, 0.485034f, 0.106209f, 0.023130f,
        0.477801f, 0.100607f, 0.023513f, 0.470639f, 0.095156f, 0.023916f,
        0.463530f, 0.089668f, 0.024336f, 0.456494f, 0.084258f, 0.024766f,
        0.449521f, 0.078741f, 0.025203f, 0.442603f, 0.073404f, 0.025644f,
        0.435737f, 0.067904f, 0.026084f, 0.428918f, 0.062415f, 0.026522f,
        0.422146f, 0.056832f, 0.026954f, 0.415437f, 0.051116f, 0.027378f,
        0.408768f, 0.045352f, 0.027790f, 0.402132f, 0.039448f, 0.028189f,
        0.395562f, 0.033385f, 0.028570f, 0.389015f, 0.027844f, 0.028932f,
        0.382496f, 0.022586f, 0.029271f, 0.376028f, 0.017608f, 0.029583f,
        0.369578f, 0.012890f, 0.029866f, 0.363161f, 0.008243f, 0.030115f,
        0.356785f, 0.004035f, 0.030327f, 0.350423f, 0.000061f, 0.030499f
    };

    std::vector<scibar::Color> cmap(256);
    for (int i = 0; i < 256; ++i) {
        cmap[i] = scibar::Color{
            static_cast<uint8_t>(lut[i * 3 + 0] * 255.0f),
            static_cast<uint8_t>(lut[i * 3 + 1] * 255.0f),
            static_cast<uint8_t>(lut[i * 3 + 2] * 255.0f),
            255
        };
    }
    return cmap;
}

} // namespace scibar::util

#endif // SCIBAR_HPP
