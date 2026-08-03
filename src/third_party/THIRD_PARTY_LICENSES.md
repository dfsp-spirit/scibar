# Third-Party Licenses

This file lists all third-party code and assets included in scibar.
The scibar library itself is licensed under the [MIT License](../../LICENSE).

---

## Vendored Source Files

These files live in `src/third_party/` and are compiled or included
as part of the library.

### canvas_ity.hpp

- **Project:** [canvas_ity](https://github.com/a-e-k/canvas_ity)
- **Author:** Andrew Kensler
- **License:** ISC
- **Usage:** 2D rasterization backend

### stb_image.h

- **Project:** [nothings/stb](https://github.com/nothings/stb)
- **Author:** Sean Barrett
- **License:** Public Domain / MIT
- **Usage:** Image loading (PNG, JPEG, etc.)

### stb_image_write.h

- **Project:** [nothings/stb](https://github.com/nothings/stb)
- **Author:** Sean Barrett
- **License:** Public Domain / MIT
- **Usage:** Image writing (PNG output)

### stb_truetype.h

- **Project:** [nothings/stb](https://github.com/nothings/stb)
- **Author:** Sean Barrett
- **License:** Public Domain / MIT
- **Usage:** TrueType font metrics and glyph rasterization

---

## Embedded Assets

These assets are embedded as data within `src/core/scibar/scibar.hpp`
and are not separate files.

### Inter Font

- **Project:** [Inter](https://rsms.me/inter/)
- **Author:** Rasmus Andersson
- **License:** SIL Open Font License 1.1
- **Usage:** Default typeface for colorbar labels and titles

### viridis Colormap

- **Author:** Stéfan van der Walt and Nathaniel Smith
- **Reference:** [smith2015viridis.bib](../../web/references/smith2015viridis.bib)
- **License:** CC0 (public domain dedication)
- **Usage:** Default sequential colormap (256 entries)

### vik Colormap

- **Project:** [Scientific colour maps](https://www.fabiocrameri.ch/colourmaps/)
- **Author:** Fabio Crameri
- **License:** MIT (see also https://zenodo.org/records/8409685)
- **Usage:** Default diverging colormap (256 entries)
