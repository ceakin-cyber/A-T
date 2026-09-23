#pragma once

#include <chrono>

namespace app {

// The time source for the star map panel: normally follows the real system clock like every
// other panel, but can be detached to jump to a different moment or play the sky forward (or
// backward) at a chosen speed, to see the sky at another time. This never affects anything else
// in the app -- the satellite tracker, pass predictor and event log always use the real system
// clock, passed to them separately.
struct StarMapTime {
    bool following = true; // true: Effective() always returns `now`
    std::chrono::system_clock::time_point simulated{}; // used only while !following; meaningless
                                                        // (and left at its default) while
                                                        // following
    bool playing = false; // whether `simulated` advances on its own each frame; meaningful only
                          // while !following
    double speed = 3600.0; // how fast `simulated` advances while playing, as a multiple of real
                           // elapsed time; negative runs the sky backwards, 0 would freeze it
                           // (prefer `playing = false` for that instead)
};

// The moment the star map should compute the sky for this frame: `now` while following, else
// the detached `simulated` time.
std::chrono::system_clock::time_point Effective(const StarMapTime& time,
                                                 std::chrono::system_clock::time_point now);

// Detaches from the real clock (if not already: does nothing to an already-detached `simulated`)
// and offsets its current effective time by `delta`, positive or negative. `playing` is left
// unchanged.
void Jump(StarMapTime& time, std::chrono::system_clock::time_point now,
         std::chrono::duration<double> delta);

// Sets whether the simulated time advances on its own. Turning it on while still following the
// real clock detaches first, seeding `simulated` from `now`, so playback starts from the moment
// it was turned on rather than jumping to whatever `simulated` last held.
void SetPlaying(StarMapTime& time, std::chrono::system_clock::time_point now, bool playing);

// Re-attaches to the real system clock and stops playback. `speed` is left as it was, so a later
// detach resumes at the same speed.
void Resume(StarMapTime& time);

// Advances `simulated` by realDeltaSeconds * speed. A no-op while following (Effective() already
// tracks `now` in that case) or while not playing. Safe, and intended, to call every frame
// regardless of state.
void Advance(StarMapTime& time, std::chrono::system_clock::time_point now,
            double realDeltaSeconds);

} // namespace app
