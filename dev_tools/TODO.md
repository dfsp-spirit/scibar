# scibar TODO

---

## Documentation

### `reverseColors` behaviour must be clearly documented

`Style::reverseColors` flips the **colormap direction only** — tick labels and data domain are unchanged. This is separate from inverting the axis (swapping `min`/`max`).

The following table should appear in the documentation (e.g., in a "Colormap Direction" section or FAQ):

| What you want | How to get it |
|---|---|
| Forward bar, min→max top-to-bottom (or left-to-right) | default (`reverseColors = false`) |
| Colors flipped, data range unchanged | `style.reverseColors = true` |
| Entire bar inverted (data range flipped) | swap `Scale::min` ↔ `Scale::max` |
| Colors flipped + data range inverted | swap `min`↔`max` + `reverseColors = true` |

**FAQ entry** (draft):

> **Q: I set `reverseColors = true` but my tick labels are the same. Shouldn't they flip too?**  
> No. `reverseColors` only flips the colormap lookup — it's equivalent to using `viridis_r` instead of `viridis` in matplotlib. The data domain (`Scale::min` / `Scale::max`) stays exactly as you set it. If you want a fully inverted colorbar (labels reversed too), swap `Scale::min` and `Scale::max` — the colors will naturally follow because the gradient is data-driven. You can also combine both for every possible orientation.

> **Q: How is this different from matplotlib's `colorbar.ax.invert_yaxis()`?**  
> matplotlib's `invert_yaxis()` operates on the axis object which owns both data and visual properties, so it flips everything. scibar keeps data (`Spec`) and visuals (`Style`) separate — `reverseColors` is visual-only. To invert the axis, swap `min`↔`max` on your `Scale`.

**TODO:** Add an example (one is enough, e.g., `axis_inversion`) that demonstrates swapping `min`/`max` to achieve a fully inverted colorbar, contrasting it with `reverseColors`-only reversal.
