# Land outline data

`land_110m.txt` holds the outlines of the world's land masses, drawn as the background of the
ground track map: one closed ring per line, as space-separated `longitude,latitude` pairs in
degrees (positive east and north). A line starting `hole` is a hole in the land around it rather
than land itself; there is one, the Caspian Sea.

- **Source:** Natural Earth's 1:110m physical land layer (`ne_110m_land`), from the
  [natural-earth-vector](https://github.com/nvkelso/natural-earth-vector) repository
  (`geojson/ne_110m_land.geojson`), fetched 2026-09-25. The 1:110m scale is Natural Earth's
  coarsest: plenty for a map a few hundred pixels wide, and small enough to load instantly.
- **Extracted:** every ring of every polygon, in file order (a polygon's first ring is its
  outline, and any after it are holes, marked `hole`), with GeoJSON's repeated closing
  point dropped and coordinates rounded to 0.01 degrees (about a kilometer). 128 rings, 5,015
  points.
- **License:** public domain ([Natural Earth terms of use](https://www.naturalearthdata.com/about/terms-of-use/)).
  No attribution is required, but it is given here anyway.
