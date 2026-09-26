#pragma once

namespace app {

// The geometry of the Moon's lit part as seen from Earth, for drawing its phase: which side is
// lit, and how much of each horizontal slice across the disk.

// Whether the lit side is on the observer's right. From the northern hemisphere a waxing Moon
// (the first half of the cycle, before full) is lit on the right and a waning one on the left;
// from the southern hemisphere, where the Moon is seen the other way up, it is the reverse.
bool MoonLitOnRight(double ageDays, double observerLatitudeRad);

// The lit stretch of one horizontal slice across a disk of radius 1, as x from `left` to
// `right` (both in [-1, 1]); `left == right` when none of it is lit.
struct LitSpan {
    double left = 0.0;
    double right = 0.0;
};

// The lit stretch of the slice at height `y` (in [-1, 1]; 0 is through the middle), for a Moon
// with `illuminatedFraction` of its disk lit (0 new, 1 full), lit on the right or left. The lit
// part is bounded on one side by the disk's own edge and on the other by the terminator, the
// line between day and night on the Moon, which from Earth looks like half an ellipse: bulging
// toward the lit side at a crescent, straight down the middle at a half moon, and bulging away
// from it at a gibbous moon.
LitSpan MoonLitSpan(double y, double illuminatedFraction, bool litOnRight);

} // namespace app
