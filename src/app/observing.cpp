#include "app/observing.h"

#include "core/frames.h"
#include "core/sun.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace app {

namespace {

constexpr double kMinutesPerDay = 1440.0;
constexpr double kSecondsPerDay = 86400.0;

// The satellite's Earth-fixed position at `julianDate`, or nullopt if the propagator fails.
std::optional<core::Vec3> SatelliteEcef(const core::Sgp4Model& model, double julianDate) {
    const auto state = core::Propagate(model, (julianDate - model.epochJd) * kMinutesPerDay);
    if (!state) {
        return std::nullopt;
    }
    return core::TemeToEcef(state->position, julianDate);
}

core::Vec3 Scaled(const core::Vec3& v, double factor) {
    return {v.x * factor, v.y * factor, v.z * factor};
}

} // namespace

std::optional<core::LookAngles> LookAnglesAt(const core::Sgp4Model& model,
                                             const core::Geodetic& observer, double julianDate) {
    const std::optional<core::Vec3> ecef = SatelliteEcef(model, julianDate);
    if (!ecef) {
        return std::nullopt;
    }
    return core::EcefToLookAngles(observer, *ecef);
}

double SunElevationAt(const core::Geodetic& observer, double julianDate) {
    const core::Vec3 sun = Scaled(core::SunDirectionEcef(julianDate), core::kAstronomicalUnitKm);
    return core::EcefToLookAngles(observer, sun).elevation;
}

Visibility ClassifyVisibility(double satelliteElevationRad, double sunElevationRad, bool sunlit) {
    if (satelliteElevationRad <= 0.0) {
        return Visibility::BelowHorizon;
    }
    if (sunElevationRad > kTwilightSunElevationRad) {
        return Visibility::Daylight;
    }
    if (!sunlit) {
        return Visibility::InShadow;
    }
    return Visibility::Visible;
}

std::optional<Visibility> VisibilityAt(const core::Sgp4Model& model, const core::Geodetic& observer,
                                       double julianDate) {
    const std::optional<core::Vec3> ecef = SatelliteEcef(model, julianDate);
    if (!ecef) {
        return std::nullopt;
    }
    return ClassifyVisibility(core::EcefToLookAngles(observer, *ecef).elevation,
                              SunElevationAt(observer, julianDate),
                              core::IsSunlit(*ecef, core::SunDirectionEcef(julianDate)));
}

const char* ToString(Visibility visibility) {
    switch (visibility) {
    case Visibility::Visible:
        return "VISIBLE";
    case Visibility::BelowHorizon:
        return "BELOW HORIZON";
    case Visibility::Daylight:
        return "DAYLIGHT";
    case Visibility::InShadow:
        return "IN SHADOW";
    }
    return "UNKNOWN";
}

std::optional<VisibleWindow> FindVisibleWindow(const core::Sgp4Model& model,
                                               const core::Geodetic& observer,
                                               const core::Pass& pass, double stepSeconds) {
    if (stepSeconds <= 0.0 || pass.setJd <= pass.riseJd) {
        return std::nullopt;
    }
    const double step = stepSeconds / kSecondsPerDay;
    std::optional<VisibleWindow> window;
    // Stop just short of the set itself, where the elevation is exactly zero (below horizon).
    for (double jd = pass.riseJd + step / 2.0; jd < pass.setJd; jd += step) {
        const std::optional<core::Vec3> ecef = SatelliteEcef(model, jd);
        if (!ecef) {
            return std::nullopt;
        }
        const double elevation = core::EcefToLookAngles(observer, *ecef).elevation;
        const Visibility visibility =
            ClassifyVisibility(elevation, SunElevationAt(observer, jd),
                               core::IsSunlit(*ecef, core::SunDirectionEcef(jd)));
        if (visibility != Visibility::Visible) {
            continue;
        }
        if (!window) {
            window = VisibleWindow{jd, jd, elevation};
        }
        window->endJd = jd;
        window->maxElevationRad = std::max(window->maxElevationRad, elevation);
    }
    return window;
}

std::optional<VisiblePass> FindNextVisiblePass(const core::Sgp4Model& model,
                                               const core::Geodetic& observer, double fromJd,
                                               double days) {
    for (const core::Pass& pass : core::FindPasses(model, observer, fromJd, fromJd + days)) {
        const std::optional<VisibleWindow> window = FindVisibleWindow(model, observer, pass);
        if (window && window->endJd >= fromJd) {
            return VisiblePass{pass, *window};
        }
    }
    return std::nullopt;
}

const char* CompassPoint(double azimuthRad) {
    static constexpr const char* kPoints[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    const double eighths = azimuthRad / (2.0 * std::numbers::pi) * 8.0;
    const int index = static_cast<int>(std::lround(eighths)) % 8;
    return kPoints[index < 0 ? index + 8 : index];
}

} // namespace app
