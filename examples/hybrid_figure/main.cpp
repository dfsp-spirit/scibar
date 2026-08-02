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
/// Output: hybrid_brain.svg (standalone SVG, view in any browser)

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
    spec.title    = "Sulcal Depth (mm)";
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

    // --- SVG options ---
    SVGOptions opts;
    opts.totalWidth   = totalW;
    opts.totalHeight  = totalH;
    opts.mainImageHref   = "whole_brain_sulc.png";
    opts.mainImageBounds = {imgX, imgY, imgW, imgH};
    opts.colorbarBounds  = {barX, barY, barW, barH};

    // --- Render ---
    Style style = Style::defaultLight();
    std::string svg = exportToSVG(spec, style, opts, Orientation::Vertical);

    // --- Write ---
    std::ofstream out("hybrid_brain.svg");
    if (!out) {
        std::cerr << "ERROR: Could not write hybrid_brain.svg\n";
        return 1;
    }
    out << svg;
    out.close();

    std::cout << "Wrote hybrid_brain.svg (" << totalW << "×" << totalH
              << " px, " << svg.size() << " bytes)\n";
    std::cout << "Open with: firefox hybrid_brain.svg\n";
    std::cout << "Or convert to PDF: rsvg-convert -f pdf -o hybrid_brain.pdf hybrid_brain.svg\n";

    return 0;
}
