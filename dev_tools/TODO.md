# scibar TODO

---

## Documentation

### `reverseColors` and `inverted` behaviour must be clearly documented

Two independent flags control colormap direction:

| Flag | Struct | What it flips |
|---|---|---|
| `reverseColors` | `Style` | Colormap lookup order (visual only) |
| `inverted` | `Scale` | Data direction — where min/max land spatially |

The following table should appear in the documentation:

| What you want | How to get it |
|---|---|
| Forward bar, min→max top-to-bottom (or left-to-right) | default |
| Colors flipped, data range unchanged | `style.reverseColors = true` |
| Axis inverted (data range flipped), same colors | `scale.inverted = true` |
| Colors flipped + data range inverted | `scale.inverted = true` + `style.reverseColors = true` |

**FAQ entry** (draft):

> **Q: I set `reverseColors = true` but my tick labels are the same. Shouldn't they flip too?**
> No. `reverseColors` only flips the colormap lookup — it's equivalent to using `viridis_r` instead of `viridis` in matplotlib. The data domain stays exactly as you set it. To flip the data direction (and tick labels), set `scale.inverted = true`. You can combine both for every possible orientation.

> **Q: On continuous scales, `inverted` + `reverseColors` produce the same gradient pixels. Is this a bug?**
> No — this is by design. The two flags cancel on continuous scales because they flip two independent things (data→position and data→color) that compose back to the original mapping. The tick labels still differ: `inverted` flips their positions, `reverseColors` does not.

> **Q: How is this different from matplotlib's `colorbar.ax.invert_yaxis()`?**
> matplotlib's `invert_yaxis()` flips both data and visual together. scibar keeps data (`Spec`) and visuals (`Style`) separate: `scale.inverted` handles data direction, `style.reverseColors` handles colormap direction. They compose independently.

**TODO:** Example `diverging_horizontal_vik_axisinversed` demonstrates row 4.
