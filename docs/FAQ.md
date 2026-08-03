# Frequently Asked Questions {#faq}

## Colormap Direction

### Q: I set `reverseColors = true` but my tick labels are the same. Shouldn't they flip too?

No. `reverseColors` only flips the colormap lookup — it's equivalent to
using `viridis_r` instead of `viridis` in matplotlib.  The data domain
stays exactly as you set it.

To flip the data direction (and tick labels), set `scale.inverted = true`.
You can combine both for every possible orientation.

### Q: On continuous scales, `inverted` + `reverseColors` produce the same gradient pixels. Is this a bug?

No — this is by design.  The two flags cancel on continuous scales
because they flip two independent things (data→position and
data→color) that compose back to the original mapping.  The tick labels
still differ: `inverted` flips their positions, `reverseColors` does not.

### Q: How is this different from matplotlib's `colorbar.ax.invert_yaxis()`?

matplotlib's `invert_yaxis()` flips both data and visual together.
scibar keeps data (`Scale`) and visuals (`Style`) separate:

- `scale.inverted` handles data direction
- `style.reverseColors` handles colormap direction

They compose independently.

### Q: What's the full matrix of direction options?

| What you want | How to get it |
|---|---|
| Forward bar, min→max top-to-bottom (or left-to-right) | default |
| Colors flipped, data range unchanged | `style.reverseColors = true` |
| Axis inverted (data range flipped), same colors | `scale.inverted = true` |
| Colors flipped + data range inverted | `scale.inverted = true` + `style.reverseColors = true` |

## Tick Formatting

### Q: How do I control tick label precision?

Set `Style::tickPrecision` to the number of significant digits you want.
The default is 6.  Labels are formatted with `printf("%.*g", precision, value)`.

```cpp
Style style = Style::defaultLight();
style.tickPrecision = 2;  // e.g., "3.1" instead of "3.14159"
```

### Q: Can I provide my own tick values and labels?

Yes.  Populate `Spec::ticks` with your own `Tick` values before calling
any draw function.  If `spec.ticks` is non-empty, `generateTicks()` is
skipped and your custom ticks are used.

```cpp
Spec spec;
spec.ticks = {
    {0.0f,  "zero"},
    {50.0f, "fifty"},
    {100.0f,"hundred"}
};
```

## Layout

### Q: Why don't my elements fit? The ticks go off-screen!

scibar is **not a layout engine**. We provide sane defaults for the high-level API,
but if they do not work for you, e.g., because you need a very long label text, you may have
to use the low level API instead, where you can customize positions to your needs.

With the low-level API, each draw function renders exactly
at the coordinates you pass in `Rect bounds`, with ticks and labels
protruding outward.  You are responsible for leaving enough space.

The returned `Rect` from each function reports the actual bounding box
including protrusions.  Use `unionRect()` to compute the total extent:

```cpp
Rect barBox   = drawColorBar(canvas, {20, 50, 150, 400}, spec, style);
Rect tickBox  = drawTicks(canvas, barBox, spec, style);
Rect totalBox = unionRect(barBox, tickBox);
// totalBox now includes all tick/label protrusions
```

### Q: How do I create a horizontal colorbar?

Pass `Orientation::Horizontal` to the draw functions:

```cpp
drawColorBar(canvas, {50, 20, 400, 30}, spec, style, Orientation::Horizontal);
drawTicks(canvas, barBox, spec, style, Orientation::Horizontal);
```

See the `examples/categorical_horizontal/` and related examples for
complete horizontal colorbar programs.

### Q: What does `drawLegend()` do differently from the low-level API?

`drawLegend()` applies hardcoded defaults for a vertical colorbar
filling the entire canvas, with ticks on the right and the label
centered above.  It performs **no** constraint solving, collision
detection, or multi-pass refinement.

Use `drawLegend()` for rapid prototyping and debug overlays.
Use the low-level API (`drawColorBar`, `drawTicks`, `drawLabel`) for
anything that will appear in a publication.

### Q: My SVG and raster outputs look slightly different — labels are shifted or text sizes don't match. Why?

This is expected. The two backends render text very differently:

- **Raster** uses stb_truetype to rasterize glyphs into the pixel buffer.
  Every pixel is exact — what you see is what's in the file.

- **SVG** is interpreted by a viewer (browser, image viewer, Inkscape).
  The SVG file contains coordinates and font instructions, but the actual
  rendering depends on the viewer engine.

Common causes of differences:

| Cause | Effect | Affected viewers |
|---|---|---|
| `dominant-baseline` not supported | Labels shift vertically, may overlap the bar | librsvg (Ubuntu Image Viewer, GNOME) |
| Font metrics differ from stb_truetype | Slightly different text heights or positions | All viewers (each font engine is unique) |
| SVG `<text>` anti-aliasing vs raster aa | Text may appear slightly bolder or thinner | Varies by viewer |
| Sub-pixel rendering | SVG text edges look different from raster | All viewers |

**Recommendation:** View SVG output in Chrome, Firefox, or Inkscape for
the most accurate rendering. The Ubuntu Image Viewer (Eye of GNOME) uses
librsvg, which has known issues with `dominant-baseline`.

If you need pixel-identical output across viewers, stick with raster (PPM/PNG).

**Converting to PDF?** The same caveats apply — the converter is just another
SVG renderer. Inkscape CLI (`inkscape --export-filename=out.pdf in.svg`) and
Chrome headless (`chromium --headless --print-to-pdf=out.pdf in.svg`) produce
the most faithful results. Avoid librsvg-based converters (`rsvg-convert`)
for scibar SVGs. See the PDF conversion FAQ above for details.

## Colormaps

### Q: How do I use my own colormap?

Pass any `std::vector<Color>` to `Spec::colormap`.  The `ColorMapView`
is non-owning — make sure the vector outlives any draw calls.

```cpp
std::vector<scibar::Color> my_colormap = {
    {0, 0, 255, 255},     // blue
    {0, 255, 255, 255},   // cyan
    {0, 255, 0, 255},     // green
    {255, 255, 0, 255},   // yellow
    {255, 0, 0, 255}      // red
};

Spec spec;
spec.colormap = my_colormap;  // ColorMapView keeps a non-owning pointer
```

### Q: What colormaps are built in?

Two 256-entry colormaps are included:

- `scibar::util::viridis()` — perceptually-uniform sequential (blue→green→yellow)
- `scibar::util::vik()` — diverging (blue→yellow→red), CVD-friendly, by Fabio Crameri

Both are backed by static data — calling them repeatedly costs nothing.

## Fonts

### Q: How do I use a custom font?

```cpp
scibar::Font font = scibar::loadFont("path/to/MyFont.ttf", 16.0f);
Style style = Style::defaultLight();
style.font = font;
```

The font is loaded once and kept in memory for the lifetime of the
process.  The embedded Inter font (used when `Font::handle == nullptr`)
requires zero configuration.

### Q: Can I measure text without rendering it?

Yes.  Use `measureText()` for pixel dimensions and `fontMetrics()` for
vertical metrics:

```cpp
auto [w, h] = scibar::measureText("My Label", font);
FontMetrics fm = scibar::fontMetrics(font);
// fm.ascender, fm.descender, fm.lineHeight are all in pixels
```

## SVG Export

### Q: My SVG shows faint lines between color segments. What's wrong?

This is a known anti-aliasing artifact in SVG renderers.  For continuous
scales, scibar uses `<linearGradient>` with `<stop>` elements, which
should be seam-free in all conformant renderers.

For categorical scales, scibar renders individual `<rect>` elements with
`shape-rendering="crispEdges"`.  If you still see seams, try using a
continuous scale or overlapping rects by 0.5px (not built in — you'd
need to post-process the SVG).

### Q: How do I embed a rendered mesh image in the SVG?

Set `SVGOptions::mainImageHref` to the path of your rendered image:

```cpp
SVGOptions opts;
opts.mainImageHref = "rendered_mesh.png";
opts.mainImageBounds = {20, 20, 550, 550};
opts.colorbarBounds  = {600, 50, 120, 500};
```

The image is embedded with `image-rendering="crisp-edges"` to prevent
bilinear blurring of scientific data.

## PNG Export / stb_image_write

### Q: Does `exportColorbar()` support PNG output? How?

Yes — out of the box. scibar vendors `stb_image_write.h` (public domain) and
compiles it with `STB_IMAGE_WRITE_STATIC` (internal linkage) so it
never conflicts with your own use of stb libraries.  PNG, PPM, and SVG all
work with zero configuration:

```cpp
exportColorbar(opts, "colorbar.png");  // PNG  (stb_image_write)
exportColorbar(opts, "colorbar.ppm");  // PPM  (zero-dependency plaintext)
exportColorbar(opts, "colorbar.svg");  // SVG  (zero-dependency vector)
```

### Q: How do I disable scibar's built-in PNG support?

Define `SCIBAR_NO_PNG` **before** including scibar.hpp:

```cpp
#define SCIBAR_NO_PNG
#define SCIBAR_IMPLEMENTATION
#include "scibar/scibar.hpp"
```

**Consequences:**

- `exportColorbar(opts, "file.png")` returns `false` — PNG output is not available.
- PPM and SVG output continue to work normally.
- scibar does **not** include `stb_image_write.h` at all — no stb code in your
  translation units, and no include-path dependency on scibar's vendored stb header.

When to use this:

| Scenario | Action |
|---|---|
| You want smaller compilation / less code in each TU | `#define SCIBAR_NO_PNG` |
| Your project already uses stb_image_write.h at a different path | `#define SCIBAR_NO_PNG` |
| You want to call `stbi_write_png` yourself on a scibar `Canvas` | No action needed — scibar's stb is static, so it never conflicts with yours |

See `examples/no_png/` for a complete example.

## Error Handling

### Q: What happens if I pass invalid data?

scibar uses **assertions** for invalid input (log scale with `min <= 0`,
null canvas pixels, zero-dimension `Rect`).  In debug builds, the program
halts immediately with a clear message.  In release builds (`-DNDEBUG`),
behavior is undefined — the caller is responsible for providing valid data.

### Q: Does scibar throw exceptions?

No. scibar does not throw exceptions and does not use error codes.
Invalid input triggers assertions in debug builds.  This is a deliberate
design choice: scibar is a low-level building block, and the caller is
expected to validate inputs.

### The vector output format of scibar is SVG, but I need PDF!

The conversion between these formats is easy and there are many established tools for this. E.g., under Linux, try one of these options:

```shell
# Option 1: Inkscape CLI (most reliable — no X11/GUI required)
inkscape --export-filename=figure.pdf figure.svg

# Option 2: Chrome/Chromium headless (excellent SVG support, same engine as viewing)
chromium --headless --print-to-pdf=figure.pdf figure.svg

# Option 3: librsvg (lightweight, fast — but has known issues with text baseline)
rsvg-convert -f pdf -o figure.pdf figure.svg

# Option 4: CairoSVG (Python)
cairosvg figure.svg -o figure.pdf
```

**We recommend Inkscape CLI** — it doesn't require X11/GUI (`--export-filename` works headlessly), handles `dominant-baseline` correctly, embeds glyph outlines faithfully, and is packaged for every distro.

**⚠️ Avoid librsvg for scibar SVGs** — librsvg (used by `rsvg-convert` and the Ubuntu Image Viewer) has known issues with `dominant-baseline`, causing tick labels to shift and overlap the bar. See the "SVG vs. raster" FAQ below for details.

**For CI pipelines / HPC / headless environments**, both Inkscape CLI and Chrome headless work without a display server. Install with:

```shell
# Inkscape (headless-safe)
sudo apt install inkscape

# Chrome headless
sudo apt install chromium-browser
```

If you cannot install either (e.g., minimal Docker image), `rsvg-convert` is an acceptable fallback — just be aware that text positioning may differ from the raster output.

Please note that we will not re-implement the general-purpose functionality of SVG to PDF conversion in scibar, as that is clearly out of scope.
