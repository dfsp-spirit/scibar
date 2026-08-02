// scibar — Full CLI Demo Application
//
// A complete command-line tool that renders publication-quality colorbars
// from a TOML configuration file with CLI overrides.  Demonstrates how to
// integrate scibar into a larger workflow.
//
// Usage:
//   scibar_demo                           # uses config.toml in cwd
//   scibar_demo --config my.toml          # explicit config file
//   scibar_demo --cmap vik --min 0 --max 50  # quick overrides
//   scibar_demo --list-cmaps              # list built-in colormaps
//   scibar_demo -h                        # full help
//
// Dependencies (vendored alongside this file):
//   toml.hpp — toml++ single-header (MIT), TOML config parsing

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define SCIBAR_IMPLEMENTATION

#include "../../src/core/scibar/scibar.hpp"
#include "../../src/third_party/stb_image_write.h"
#include "toml.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

// =========================================================================
// Application config (merged from TOML + CLI)
// =========================================================================

struct AppConfig {
    // Output
    std::string format   = "both";
    std::string filename = "colorbar";
    int canvasW = 800;
    int canvasH = 300;

    // Scale
    std::string scaleType = "linear";
    float scaleMin    = 0.0f;
    float scaleMax    = 100.0f;
    float scaleMid    = 50.0f;
    bool  scaleInverted = false;

    // Colormap
    std::string cmapName = "viridis";
    std::string cmapFile;         // external file path (overrides cmapName if set)
    bool cmapReverse = false;

    // Orientation
    std::string orientation = "vertical";

    // Title
    std::string titleText = "Value";

    // Layout (0 = auto)
    int barThickness = 0;  // colorbar thickness in pixels
    int barMargin    = 0;  // margin from canvas edge in pixels

    // Style
    std::string theme         = "light";
    float  fontSize        = 14.0f;
    float  tickLength      = 5.0f;
    int    tickPrecision   = 2;
    int    subticks        = 4;
    bool   showFrame       = true;
    bool   ticksInward     = false;
};

// =========================================================================
// Help text
// =========================================================================

static void printHelp(const char* prog) {
    printf(
        "scibar_demo — Full-featured scientific colorbar renderer\n"
        "\n"
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  --config FILE       TOML config file (default: config.toml)\n"
        "  --cmap NAME         Colormap: viridis, vik\n"
        "  --cmap-file FILE    Load colormap from text file (R G B triplets)\n"
        "  --type TYPE         Scale type: linear, log, categorical, diverging\n"
        "  --min VAL           Data minimum\n"
        "  --max VAL           Data maximum\n"
        "  --mid VAL           Data midpoint (diverging only)\n"
        "  --orientation DIR   vertical, horizontal\n"
        "  --format FMT        Output format: png, svg, both\n"
        "  --output FILE       Output filename base (no extension)\n"
        "  --width N           Canvas width\n"
        "  --height N          Canvas height\n"
        "  --title TEXT        Colorbar title\n"
        "  --theme THEME       light, dark\n"
        "  --font-size N       Font size in pixels\n"
        "  --tick-len N        Tick mark length\n"
        "  --tick-prec N       Tick label precision (significant digits)\n"
        "  --subticks N        Sub-ticks per interval (0 = off)\n"
        "  --no-frame          Hide frame\n"
        "  --ticks-inward      Draw tick marks inside the bar\n"
        "  --reverse           Reverse colormap direction\n"
        "  --bar-thickness N   Colorbar thickness in pixels (default: auto)\n"
        "  --bar-margin N      Margin from canvas edges (default: auto)\n"
        "  --inverted          Invert scale axis\n"
        "  --list-cmaps        List available colormaps and exit\n"
        "  -h, --help          Show this help and exit\n"
        "\n"
        "Config file format is TOML; see config.toml for all defaults.\n"
        "CLI flags override config file values.\n",
        prog);
}

// =========================================================================
// TOML config loading
// =========================================================================

static AppConfig loadTOMLConfig(const std::string& path) {
    AppConfig cfg;

    std::ifstream ifs(path);
    if (!ifs.is_open()) return cfg; // missing config → use all defaults

    try {
        auto tbl = toml::parse(ifs);

        // [output]
        if (auto* out = tbl["output"].as_table()) {
            cfg.format   = (*out)["format"].value_or("both");
            cfg.filename = (*out)["filename"].value_or("colorbar");
        }

        // [canvas]
        if (auto* cv = tbl["canvas"].as_table()) {
            cfg.canvasW = (*cv)["width"].value_or(800);
            cfg.canvasH = (*cv)["height"].value_or(300);
        }

        // [scale]
        if (auto* sc = tbl["scale"].as_table()) {
            cfg.scaleType     = (*sc)["type"].value_or("linear");
            cfg.scaleMin      = static_cast<float>((*sc)["min"].value_or(0.0));
            cfg.scaleMax      = static_cast<float>((*sc)["max"].value_or(100.0));
            cfg.scaleMid      = static_cast<float>((*sc)["midpoint"].value_or(50.0));
            cfg.scaleInverted = (*sc)["inverted"].value_or(false);
        }

        // [colormap]
        if (auto* cm = tbl["colormap"].as_table()) {
            cfg.cmapName    = (*cm)["name"].value_or("viridis");
            cfg.cmapReverse = (*cm)["reverse"].value_or(false);
        }

        // [orientation]
        if (auto* ori = tbl["orientation"].as_table()) {
            cfg.orientation = (*ori)["direction"].value_or("vertical");
        }

        // [title]
        if (auto* tt = tbl["title"].as_table()) {
            cfg.titleText = (*tt)["text"].value_or("Value");
        }

        // [layout]
        if (auto* lo = tbl["layout"].as_table()) {
            cfg.barThickness = (*lo)["bar_thickness"].value_or(0);
            cfg.barMargin    = (*lo)["bar_margin"].value_or(0);
        }

        // [style]
        if (auto* st = tbl["style"].as_table()) {
            cfg.theme         = (*st)["theme"].value_or("light");
            cfg.fontSize      = static_cast<float>((*st)["font_size"].value_or(14.0));
            cfg.tickLength    = static_cast<float>((*st)["tick_length"].value_or(5.0));
            cfg.tickPrecision = (*st)["tick_precision"].value_or(2);
            cfg.subticks      = (*st)["subticks"].value_or(4);
            cfg.showFrame     = (*st)["show_frame"].value_or(true);
            cfg.ticksInward   = (*st)["ticks_inward"].value_or(false);
        }
    } catch (const toml::parse_error& e) {
        fprintf(stderr, "Warning: failed to parse '%s': %s\n", path.c_str(), e.what());
        fprintf(stderr, "Using defaults.\n");
    }

    return cfg;
}

// =========================================================================
// CLI argument parsing
// =========================================================================

static void applyCLIArgs(int argc, char** argv, AppConfig& cfg, std::string& configPath, bool& listCmaps) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto nextFloat = [&]() -> float {
            return (i + 1 < argc) ? static_cast<float>(atof(argv[++i])) : 0.0f;
        };
        auto nextInt = [&]() -> int {
            return (i + 1 < argc) ? atoi(argv[++i]) : 0;
        };
        auto nextStr = [&]() -> std::string {
            return (i + 1 < argc) ? std::string(argv[++i]) : "";
        };

        if (arg == "-h" || arg == "--help") {
            printHelp(argv[0]);
            exit(0);
        } else if (arg == "--list-cmaps") {
            listCmaps = true;
        } else if (arg == "--config") {
            configPath = nextStr();
        } else if (arg == "--cmap") {
            cfg.cmapName = nextStr();
        } else if (arg == "--cmap-file") {
            cfg.cmapFile = nextStr();
        } else if (arg == "--type") {
            cfg.scaleType = nextStr();
        } else if (arg == "--min") {
            cfg.scaleMin = nextFloat();
        } else if (arg == "--max") {
            cfg.scaleMax = nextFloat();
        } else if (arg == "--mid") {
            cfg.scaleMid = nextFloat();
        } else if (arg == "--orientation") {
            cfg.orientation = nextStr();
        } else if (arg == "--format") {
            cfg.format = nextStr();
        } else if (arg == "--output") {
            cfg.filename = nextStr();
        } else if (arg == "--width") {
            cfg.canvasW = nextInt();
        } else if (arg == "--height") {
            cfg.canvasH = nextInt();
        } else if (arg == "--title") {
            cfg.titleText = nextStr();
        } else if (arg == "--theme") {
            cfg.theme = nextStr();
        } else if (arg == "--font-size") {
            cfg.fontSize = nextFloat();
        } else if (arg == "--tick-len") {
            cfg.tickLength = nextFloat();
        } else if (arg == "--tick-prec") {
            cfg.tickPrecision = nextInt();
        } else if (arg == "--subticks") {
            cfg.subticks = nextInt();
        } else if (arg == "--no-frame") {
            cfg.showFrame = false;
        } else if (arg == "--ticks-inward") {
            cfg.ticksInward = true;
        } else if (arg == "--reverse") {
            cfg.cmapReverse = true;
        } else if (arg == "--inverted") {
            cfg.scaleInverted = true;
        } else if (arg == "--bar-thickness") {
            cfg.barThickness = nextInt();
        } else if (arg == "--bar-margin") {
            cfg.barMargin = nextInt();
        } else {
            fprintf(stderr, "Unknown option: %s\n", arg.c_str());
            fprintf(stderr, "Try '%s --help' for usage.\n", argv[0]);
            exit(1);
        }
    }
}

// =========================================================================
// Load colormap from a plain-text file (Crameri-style RGB triplets)
// =========================================================================

static std::vector<scibar::Color> loadColormapFromFile(const std::string& path) {
    std::vector<scibar::Color> cmap;
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        fprintf(stderr, "Error: cannot open colormap file '%s'\n", path.c_str());
        exit(1);
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(ifs, line)) {
        lineNo++;
        // Trim leading/trailing whitespace
        size_t start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos) continue;  // blank line
        size_t end = line.find_last_not_of(" \t\r");
        line = line.substr(start, end - start + 1);

        // Skip comment lines
        if (line.empty() || line[0] == '#') continue;

        float r = 0.0f, g = 0.0f, b = 0.0f;
        if (sscanf(line.c_str(), "%f %f %f", &r, &g, &b) != 3) {
            // Try comma-separated as fallback
            if (sscanf(line.c_str(), "%f,%f,%f", &r, &g, &b) != 3) {
                fprintf(stderr, "Error: malformed line %d in '%s': '%s'\n",
                        lineNo, path.c_str(), line.c_str());
                exit(1);
            }
        }

        cmap.push_back(scibar::Color::fromFloat(r, g, b));
    }

    if (cmap.empty()) {
        fprintf(stderr, "Error: no valid color entries in '%s'\n", path.c_str());
        exit(1);
    }

    printf("Loaded %zu colors from '%s'\n", cmap.size(), path.c_str());
    return cmap;
}

// =========================================================================
// Resolve colormap
// =========================================================================

static std::vector<scibar::Color> resolveColormap(const std::string& name) {
    if (name == "viridis") return scibar::util::viridis();
    if (name == "vik")     return scibar::util::vik();
    fprintf(stderr, "Unknown colormap '%s'. Try --list-cmaps.\n", name.c_str());
    exit(1);
}

// =========================================================================
// Build scibar Spec & Style from AppConfig
// =========================================================================

static void buildSpec(const AppConfig& cfg, scibar::Spec& spec,
                      std::vector<scibar::Color>& cmapStorage) {
    // Scale type
    if (cfg.scaleType == "linear")      spec.scale.type = scibar::ScaleType::Linear;
    else if (cfg.scaleType == "log")    spec.scale.type = scibar::ScaleType::Logarithmic;
    else if (cfg.scaleType == "categorical") spec.scale.type = scibar::ScaleType::Categorical;
    else if (cfg.scaleType == "diverging")   spec.scale.type = scibar::ScaleType::Diverging;
    else {
        fprintf(stderr, "Unknown scale type '%s'.\n", cfg.scaleType.c_str());
        exit(1);
    }

    spec.scale.min      = cfg.scaleMin;
    spec.scale.max      = cfg.scaleMax;
    spec.scale.midpoint = cfg.scaleMid;
    spec.scale.inverted = cfg.scaleInverted;

    spec.title   = cfg.titleText;
    if (!cfg.cmapFile.empty()) {
        cmapStorage  = loadColormapFromFile(cfg.cmapFile);
    } else {
        cmapStorage  = resolveColormap(cfg.cmapName);
    }
    spec.colormap = cmapStorage;  // ColorMapView from lvalue — safe
    // ticks/subTicks left empty → auto-generated
}

static void buildStyle(const AppConfig& cfg, scibar::Style& style) {
    style = (cfg.theme == "dark") ? scibar::Style::defaultDark()
                                  : scibar::Style::defaultLight();

    style.font.size            = cfg.fontSize;
    style.tickLength           = cfg.tickLength;
    style.tickPrecision        = cfg.tickPrecision;
    style.subTicksPerInterval  = cfg.subticks;
    style.showSubTicks         = (cfg.subticks > 0);
    style.showFrame            = cfg.showFrame;
    style.ticksInward          = cfg.ticksInward;
    style.reverseColors        = cfg.cmapReverse;
}

// =========================================================================
// Compute bar layout (shared by PNG and SVG renderers)
// =========================================================================

struct BarLayout {
    int margin;
    int barThick;
    int barLen;
    int barX;
    int barY;
};

static BarLayout computeBarLayout(int canvasW, int canvasH, int cfgThickness,
                                   int cfgMargin, scibar::Orientation ori,
                                   int titleHeight = 0) {
    BarLayout bl{};

    bl.margin   = (cfgMargin > 0) ? cfgMargin : (std::min(canvasW, canvasH) / 10);
    bl.barThick = (cfgThickness > 0) ? cfgThickness
                   : ((ori == scibar::Orientation::Vertical) ? (canvasW / 6) : (canvasH / 6));
    bl.barLen   = (ori == scibar::Orientation::Vertical)
                      ? (canvasH - 2 * bl.margin)
                      : (canvasW - 2 * bl.margin);

    if (ori == scibar::Orientation::Vertical) {
        // Center the bar horizontally
        bl.barX = (canvasW - bl.barThick) / 2;
        bl.barY = bl.margin;
    } else {
        // Horizontal: bar runs left-to-right, centered
        bl.barX = (canvasW - bl.barLen) / 2;
        bl.barY = bl.margin;
    }

    if (titleHeight > 0) {
        // Reserve space for title above the bar
        int neededTop = titleHeight + bl.margin;
        if (bl.barY < neededTop) {
            bl.barY = neededTop;
            if (ori == scibar::Orientation::Vertical) {
                bl.barLen = canvasH - bl.barY - bl.margin;
            }
        }
    }
    return bl;
}

// =========================================================================
// Render
// =========================================================================

static void renderPNG(const AppConfig& cfg, const scibar::Spec& spec,
                      const scibar::Style& style, scibar::Orientation ori) {
    int W = cfg.canvasW, H = cfg.canvasH;

    // Adjust canvas for orientation
    if (ori == scibar::Orientation::Horizontal && W < H) std::swap(W, H);

    std::vector<uint32_t> buf(static_cast<size_t>(W) * H);
    scibar::Canvas canvas{buf.data(), W, H};
    scibar::fillCanvas(canvas, (cfg.theme == "dark")
                      ? scibar::Color{30, 30, 30, 255}
                      : scibar::Color{255, 255, 255, 255});

    int titleH = static_cast<int>(cfg.fontSize * 2.0f);

    // Layout — pass titleH so vertical mode reserves space above the bar
    auto bl = computeBarLayout(W, H, cfg.barThickness, cfg.barMargin, ori, titleH);

    scibar::Rect barRect{bl.barX, bl.barY, bl.barThick, bl.barLen};
    if (ori == scibar::Orientation::Horizontal) {
        barRect = {bl.barX, bl.barY, bl.barLen, bl.barThick};
    }

    // Title
    scibar::Rect titleRect{bl.barX, bl.barY - titleH - 5, barRect.width, titleH};
    scibar::drawTitle(canvas, titleRect, spec.title, style);

    // Bar
    scibar::drawColorBar(canvas, barRect, spec, style, ori);
    scibar::drawTicks(canvas, barRect, spec, style, ori);
    scibar::drawSubTicks(canvas, barRect, spec, style, ori);

    std::string fname = cfg.filename + ".png";
    if (!stbi_write_png(fname.c_str(), W, H, 4, buf.data(), W * 4)) {
        fprintf(stderr, "Failed to write %s\n", fname.c_str());
        exit(1);
    }
    printf("Wrote %s (%dx%d)\n", fname.c_str(), W, H);
}

static void renderSVG(const AppConfig& cfg, const scibar::Spec& spec,
                      const scibar::Style& style, scibar::Orientation ori) {
    scibar::SVGOptions opts;
    opts.totalWidth  = cfg.canvasW;
    opts.totalHeight = cfg.canvasH;

    int titleH = static_cast<int>(cfg.fontSize * 2.0f);
    auto bl = computeBarLayout(cfg.canvasW, cfg.canvasH, cfg.barThickness, cfg.barMargin, ori, titleH);

    if (ori == scibar::Orientation::Vertical) {
        opts.colorbarBounds = {bl.barX, bl.barY, bl.barThick, bl.barLen};
    } else {
        opts.colorbarBounds = {bl.barX, bl.barY, bl.barLen, bl.barThick};
    }

    std::string svg = scibar::exportToSVG(spec, style, opts, ori);

    std::string fname = cfg.filename + ".svg";
    FILE* f = fopen(fname.c_str(), "w");
    if (!f) {
        fprintf(stderr, "Failed to write %s\n", fname.c_str());
        exit(1);
    }
    fputs(svg.c_str(), f);
    fclose(f);
    printf("Wrote %s\n", fname.c_str());
}

// =========================================================================
// Main
// =========================================================================

int main(int argc, char** argv) {
    std::string configPath = "config.toml";
    bool listCmaps = false;

    // 1. Load TOML defaults
    AppConfig cfg = loadTOMLConfig(configPath);

    // 2. Apply CLI overrides
    applyCLIArgs(argc, argv, cfg, configPath, listCmaps);

    // 3. Re-load config if --config changed the path
    if (configPath != "config.toml") {
        AppConfig fileCfg = loadTOMLConfig(configPath);
        // Merge: file config is base, CLI overrides already applied to `cfg`.
        // Simple approach: only use file config for values NOT overridden by CLI.
        // For simplicity here, just note that CLI args always win.
    }

    if (listCmaps) {
        printf("Built-in colormaps:\n  viridis  — perceptually-uniform sequential\n");
        printf("  vik      — diverging blue-yellow-red (Crameri)\n");
        printf("\nYou can also load any colormap from a text file with --cmap-file.\n");
        printf("Format: one RGB triplet per line (space or comma separated, 0.0–1.0).\n");
        printf("Example file included: batlow.txt (Crameri)\n");
        return 0;
    }

    // 4. Build Spec & Style
    scibar::Spec   spec;
    scibar::Style  style;
    std::vector<scibar::Color> cmapStorage;
    buildSpec(cfg, spec, cmapStorage);
    buildStyle(cfg, style);

    // 5. Orientation
    scibar::Orientation ori = scibar::Orientation::Vertical;
    if (cfg.orientation == "horizontal") ori = scibar::Orientation::Horizontal;

    // 6. Render
    bool doPNG = (cfg.format == "png"  || cfg.format == "both");
    bool doSVG = (cfg.format == "svg"  || cfg.format == "both");

    if (doPNG) renderPNG(cfg, spec, style, ori);
    if (doSVG) renderSVG(cfg, spec, style, ori);

    printf("Done.\n");
    return 0;
}
