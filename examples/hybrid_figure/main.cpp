/// scibar — Hybrid Figure Demo (scimesh integration prototype)
///
/// Demonstrates a publication-ready hybrid SVG: a pre-rendered raster
/// mesh image (from scimesh) embedded alongside a vector colorbar with
/// proper linear gradient and crisp typography — all in a single
/// self-contained SVG file.
///
/// The raster image is a FreeSurfer brain surface reconstruction
/// (whole_brain_sulc.png, 938×416, 300k triangles) rendered with scimesh.
/// Sulcal depth is mapped to vertex colors using the viridis colormap.
///
/// Output: hybrid_brain_referenced.svg (external image ref)
///         hybrid_brain_embedded.svg   (self-contained, base64 inlined)

#define SCIBAR_IMPLEMENTATION
#include "../../src/core/scibar/scibar.hpp"

#include <fstream>
#include <iostream>

int main() {
    using namespace scibar;

    // --- Data spec (must match the pre-rendered scimesh image) ---
    // From scimesh output: "Pooled data range: -9.37742 to 11.8088"
    // Colormap: viridis (256-entry perceptually-uniform sequential)
    auto cmap = util::viridis();   // store locally — ColorMapView is non-owning
    Spec spec;
    spec.scale    = Scale{ScaleType::Linear, -9.37742f, 11.8088f};
    spec.label    = "Sulcal Depth (mm)";
    spec.colormap = cmap;

    // --- Layout ---
    // Image: 938×416, placed at (30, 30) with padding around it.
    // Colorbar: same height as image (416), placed to the right.
    const int imgW = 938, imgH = 416;
    const int margin   = 30;    // padding on all sides
    const int imgX     = margin;
    const int imgY     = margin;
    const int barW     = 90;
    const int barH     = imgH;
    const int barX     = imgX + imgW + 40;  // gap between image and colorbar
    const int barY     = imgY;
    const int totalW   = barX + barW + margin;
    const int totalH   = imgY + imgH + margin + 40; // extra room for title below

    // --- SVG options (shared layout, different image href) ---
    SVGOptions opts;
    opts.totalWidth    = totalW;
    opts.totalHeight   = totalH;
    opts.mainImageBounds = {imgX, imgY, imgW, imgH};
    opts.colorbarBounds  = {barX, barY, barW, barH};

    Style style = Style::defaultLight();

    // --- 1. Referenced variant (external .png file) ---
    opts.mainImageHref = "whole_brain_sulc.png";
    std::string svgRef = exportToSVG(spec, style, opts, Orientation::Vertical);

    std::ofstream outRef("hybrid_brain_referenced.svg");
    if (!outRef) {
        std::cerr << "ERROR: Could not write hybrid_brain_referenced.svg\n";
        return 1;
    }
    outRef << svgRef;
    outRef.close();
    std::cout << "Wrote hybrid_brain_referenced.svg (" << totalW << "×" << totalH
              << " px, " << svgRef.size() << " bytes)\n";

    // --- 2. Embedded variant (base64 data URI, fully self-contained) ---
    opts.mainImageHref = util::imageToDataURI("whole_brain_sulc.png");
    if (opts.mainImageHref.empty()) {
        std::cerr << "ERROR: Could not read whole_brain_sulc.png for embedding\n";
        return 1;
    }
    std::string svgEmb = exportToSVG(spec, style, opts, Orientation::Vertical);

    std::ofstream outEmb("hybrid_brain_embedded.svg");
    if (!outEmb) {
        std::cerr << "ERROR: Could not write hybrid_brain_embedded.svg\n";
        return 1;
    }
    outEmb << svgEmb;
    outEmb.close();
    std::cout << "Wrote hybrid_brain_embedded.svg (" << totalW << "×" << totalH
              << " px, " << svgEmb.size() << " bytes)\n";

    return 0;
}
