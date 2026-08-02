# API Overview {#api_overview}

scibar provides two API levels — **high-level** (single call, auto-layout) and
**low-level** (manual positioning, full control) — across two output backends
(raster pixels and vector SVG).

> **Related:** [Getting Started](@ref getting_started) ·
> [FAQ](@ref faq) ·
> [Full API Reference](namespacescibar.html)

---

## Raster API — draws into a pixel `Canvas`

Use these when you need a packed RGBA pixel buffer (for PNG, PPM, or
real-time overlays/HUDs).

### High-level

| Function | Description |
|---|---|
| `drawLegend(canvas, spec, style)` | Auto-layout: renders title, colorbar, major ticks, and sub-ticks in a single call. The vertical layout is computed automatically; for horizontal bars, pass `Orientation::Horizontal`. |

### Low-level

| Function | Description |
|---|---|
| `drawColorBar(canvas, bounds, spec, style, orientation)` | The gradient bar only. Returns the actual bounding box (including frame). |
| `drawTicks(canvas, barBounds, spec, style, orientation)` | Major tick marks and labels. Auto-generates ticks if `spec.ticks` is empty. |
| `drawSubTicks(canvas, barBounds, spec, style, orientation)` | Short, unlabeled sub-ticks between major ticks. |
| `drawTitle(canvas, bounds, title, style)` | Title text, centered horizontally in the given bounds. |

### Utilities

| Function | Description |
|---|---|
| `fillCanvas(canvas, color)` | Fill the entire canvas with a solid background color (e.g., white). |
| `writePPM(canvas, path)` | Save the canvas as a PPM file (zero dependencies). |
| `unionRect(a, b)` | Compute the bounding box enclosing two rectangles. |

---

## Vector API — returns an SVG string

Use these for publication-quality vector graphics and hybrid figures
(raster image + vector colorbar in one SVG).

### High-level

| Function | Description |
|---|---|
| `exportLegendToSVG(spec, style, width, height, orientation)` | Auto-layout: returns a complete SVG document with title, colorbar, ticks, and sub-ticks. Mirrors `drawLegend()` for vector output. |

### Low-level

| Function | Description |
|---|---|
| `exportToSVG(spec, style, options, orientation)` | Full-control SVG export. Use `SVGOptions` to set exact bounds, and optionally embed a raster image (`mainImageHref`) alongside the vector colorbar for hybrid figures. |

---

## Shared / Mid-level

| Function | Description |
|---|---|
| `computeLegendLayout(width, height, spec, style, orientation)` | Pre-compute the layout that both `drawLegend()` and `exportLegendToSVG()` use internally. Call this once, then pass the resulting `LegendLayout` bounds to the low-level raster and vector functions for **identical placement** across PNG and SVG output. |

---

## Tick Generation

| Function | Description |
|---|---|
| `generateTicks(scale, targetCount, precision)` | Auto-generate "nice" major tick values and labels from a data scale. |
| `generateSubTicks(scale, majorTicks, subTicksPerInterval)` | Generate evenly-spaced sub-ticks between the given major ticks. |

---

## Font & Text

| Function | Description |
|---|---|
| `loadFont(path, size)` | Load a custom `.ttf` font. If not called, the embedded Inter font is used. |
| `measureText(text, font)` | Measure the pixel dimensions of a text string. |
| `fontMetrics(font)` | Get ascender, descender, and line height for precise vertical alignment. |
| `textAdvance(font, text, upToIndex)` | Measure horizontal advance up to a character index (for cursor/selection placement). |
| `codepointAdvance(font, left, right)` | Kerning advance between two Unicode codepoints. |

---

## Choosing the Right API

| You want… | Use… |
|---|---|
| Quick raster colorbar, default layout | `drawLegend(canvas, spec, style)` |
| Quick SVG colorbar, default layout | `exportLegendToSVG(spec, style, w, h)` |
| Raster with pixel-level control | `drawColorBar()` + `drawTicks()` + `drawSubTicks()` + `drawTitle()` |
| SVG with exact bounds or hybrid image | `exportToSVG(spec, style, options)` |
| Identical layout in both PNG and SVG | `computeLegendLayout()` → pass bounds to both backends |

**Rule of thumb:** Start with the high-level function. If you need to tweak
positions, switch to the low-level equivalents. If you're producing both
PNG and SVG, call `computeLegendLayout()` once so both outputs match.

---

## Minimal Examples

**High-level raster (one call):**

```cpp
drawLegend(canvas, spec, Style::defaultLight());
```

**Low-level raster (manual placement):**

```cpp
Rect barRect{50, 45, 500, 30};
Rect titleRect{barRect.x, barRect.y - 35, barRect.width, 30};

drawTitle(canvas, titleRect, spec.title, style);
drawColorBar(canvas, barRect, spec, style, Orientation::Horizontal);
drawTicks(canvas, barRect, spec, style, Orientation::Horizontal);
drawSubTicks(canvas, barRect, spec, style, Orientation::Horizontal);
```

**High-level vector (one call):**

```cpp
std::string svg = exportLegendToSVG(spec, style, 700, 250,
                                     Orientation::Horizontal);
```

**Low-level vector (hybrid figure):**

```cpp
SVGOptions opts;
opts.totalWidth  = 800;
opts.totalHeight = 600;
opts.mainImageHref   = "rendered_mesh.png";
opts.mainImageBounds = {20, 20, 550, 550};
opts.colorbarBounds  = {600, 50, 120, 500};

std::string svg = exportToSVG(spec, style, opts);
```

See the [examples directory](https://github.com/dfsp-spirit/scibar/tree/main/examples)
for complete, runnable programs demonstrating each pattern.
