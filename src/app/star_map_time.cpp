#include "app/star_map_time.h"

namespace app {

namespace {

// Detaches from the real clock, seeding `simulated` from `now` -- but only the first time; an
// already-detached clock keeps whatever `simulated` it has.
void DetachIfNeeded(StarMapTime& time, std::chrono::system_clock::time_point now) {
    if (time.following) {
        time.following = false;
        time.simulated = now;
    }
}

} // namespace

std::chrono::system_clock::time_point Effective(const StarMapTime& time,
                                                 std::chrono::system_clock::time_point now) {
    return time.following ? now : time.simulated;
}

void Jump(StarMapTime& time, std::chrono::system_clock::time_point now,
         std::chrono::duration<double> delta) {
    DetachIfNeeded(time, now);
    time.simulated += std::chrono::duration_cast<std::chrono::system_clock::duration>(delta);
}

void SetPlaying(StarMapTime& time, std::chrono::system_clock::time_point now, bool playing) {
    if (playing) {
        DetachIfNeeded(time, now);
    }
    time.playing = playing;
}

void Resume(StarMapTime& time) {
    time.following = true;
    time.playing = false;
}

void Advance(StarMapTime& time, std::chrono::system_clock::time_point now,
            double realDeltaSeconds) {
    if (time.following || !time.playing) {
        return;
    }
    time.simulated += std::chrono::duration_cast<std::chrono::system_clock::duration>(
        std::chrono::duration<double>(realDeltaSeconds * time.speed));
}

} // namespace app
