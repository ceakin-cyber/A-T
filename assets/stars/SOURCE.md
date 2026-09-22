# Star catalog

`hygdata_mag6.csv` is a filtered copy of the [HYG Database](https://github.com/astronexus/HYG-Database),
which merges the Hipparcos, Yale Bright Star, and Gliese Nearby Star catalogs into one CSV with
right ascension, declination, magnitude, distance, and spectral type for each star.

- **Source:** `hyg/CURRENT/hygdata_v41.csv` from the HYG-Database repository, fetched 2026-09-21.
- **Filter applied:** only stars with `mag <= 6.0` (naked-eye visible) are kept, cutting the
  original 119,626 stars down to 5,071. This keeps the file small and matches what the star map
  needs; nothing else about the data is changed.
- **License:** [CC BY-SA 4.0](http://creativecommons.org/licenses/by-sa/4.0/), the license the
  HYG Database is published under. This file is a filtered subset (an adaptation) of that data
  and is redistributed under the same license, as CC BY-SA requires. This applies to this data
  file only, not to the project's own source code, which remains MIT licensed (see `/LICENSE`).
