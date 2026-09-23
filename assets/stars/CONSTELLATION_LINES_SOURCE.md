# Constellation line data

`constellation_lines.csv` lists the star-pair connections that draw each constellation's stick
figure: one row per line segment, giving the IAU three-letter constellation abbreviation and the
Hipparcos catalog (`hip`) numbers of the two stars the segment joins. `hygdata_mag6.csv` (see
`SOURCE.md`) carries its own `hip` column, so a segment's endpoints are found there by matching
on that column, not on this project's own `id`.

- **Source:** the `modern` sky culture's `index.json` from the
  [Stellarium](https://github.com/Stellarium/stellarium) repository
  (`skycultures/modern/index.json`), fetched 2026-09-22. Stellarium's IAU-based "modern"
  constellations were used, rather than one of its illustrative/historical sky cultures, to match
  the plain stick-figure look this project wants.
- **Extracted:** each constellation's `lines` array is a list of polylines, each a sequence of
  Hipparcos numbers; this file flattens every consecutive pair in every polyline into its own
  `con,hip1,hip2` row. All 88 IAU constellations are present, in 695 segments total. Every
  endpoint in the modern sky culture's lines is a plain Hipparcos number (no Gaia IDs or deep-sky
  object references, which some of Stellarium's other sky cultures use for fainter stars), so no
  segments needed to be dropped. Of the 710 distinct `hip` endpoints this file references, 707
  are present in `hygdata_mag6.csv`; the 3 missing (HIP 10826, 30665, 33165) fall just outside its
  mag <= 6.0 cutoff.
- **License:** [GPL v2 or later](https://github.com/Stellarium/stellarium/blob/master/COPYING),
  the license Stellarium is published under; it has no separate license for its sky culture data
  files. This file is a derived extract of that data and is redistributed under the same terms.
  This applies to this data file only, not to the project's own source code, which remains MIT
  licensed (see `/LICENSE`).
