#include "app/pass_planner.h"

#include "core/time.h"

namespace app {

namespace {

// The scan starts this far before `now`, so a pass that is already under way is found with its
// real rise time. The longest ISS pass is about 13 minutes; this is 72.
constexpr double kLookBackDays = 0.05;
constexpr double kHorizonDays = 2.0;
// Scan again once `now` is this close to the end of the window.
constexpr double kRefreshMarginDays = 0.25;

} // namespace

PassPlanner::PassPlanner(const core::Sgp4Model& model, const core::Geodetic& observer)
    : model_(model), observer_(observer) {}

void PassPlanner::Scan(double fromJd) {
    passes_ = core::FindPasses(model_, observer_, fromJd - kLookBackDays, fromJd + kHorizonDays);
    validFromJd_ = fromJd;
    windowEndJd_ = fromJd + kHorizonDays;
    scanned_ = true;
}

std::optional<core::Pass> PassPlanner::Next(net::Clock::time_point now) {
    const double jd = core::JulianDateFromTimePoint(now);

    // The first pass that has not yet set, if its set time is real. A pass that runs past the end
    // of the window has a made-up set time, so it does not count.
    const auto find = [&]() -> const core::Pass* {
        for (const core::Pass& pass : passes_) {
            if (pass.setJd > jd) {
                return pass.setsAfterWindow ? nullptr : &pass;
            }
        }
        return nullptr;
    };

    if (!scanned_ || jd < validFromJd_ || jd > windowEndJd_ - kRefreshMarginDays) {
        Scan(jd);
    }
    const core::Pass* pass = find();

    // Nothing usable ahead in the window. Passes come in clusters with gaps of half a day or more,
    // so the next one may lie just beyond the end of an old window: scan again from here. If the
    // scan is recent, it has already looked, and asking again every frame would be wasteful.
    if (pass == nullptr && jd - validFromJd_ >= kRefreshMarginDays) {
        Scan(jd);
        pass = find();
    }

    if (pass == nullptr) {
        return std::nullopt;
    }
    return *pass;
}

} // namespace app
