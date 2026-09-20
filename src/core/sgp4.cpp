#include "core/sgp4.h"

#include "core/time.h"

#include <cmath>
#include <numbers>

namespace core {

namespace {

constexpr double kTwoPi = 2.0 * std::numbers::pi;
constexpr double kDegToRad = std::numbers::pi / 180.0;
constexpr double kMinutesPerDay = 1440.0;
constexpr double kDeepSpacePeriodMinutes = 225.0;

} // namespace

std::optional<Sgp4Model> InitSgp4(const Tle& tle) {
    if (tle.eccentricity < 0.0 || tle.eccentricity >= 1.0 || tle.meanMotion <= 0.0) {
        return std::nullopt;
    }

    using namespace wgs72;

    Sgp4Model m;
    m.epochJd = JulianDateFromEpoch(tle.epochYear, tle.epochDay);
    m.bstar = tle.bstar;
    m.eccentricity = tle.eccentricity;
    m.inclination = tle.inclination * kDegToRad;
    m.raan = tle.raan * kDegToRad;
    m.argPerigee = tle.argPerigee * kDegToRad;
    m.meanAnomaly = tle.meanAnomaly * kDegToRad;
    m.meanMotionKozai = tle.meanMotion * kTwoPi / kMinutesPerDay;

    // Quantities that depend only on the epoch elements.
    const double eccsq = m.eccentricity * m.eccentricity;
    m.omeosq = 1.0 - eccsq;
    const double rteosq = std::sqrt(m.omeosq);
    m.cosInclination = std::cos(m.inclination);
    m.sinInclination = std::sin(m.inclination);
    const double cosio2 = m.cosInclination * m.cosInclination;

    // The TLE's mean motion is a "Kozai" mean motion. SGP4 works with its own (Brouwer) mean
    // motion, recovered by removing the J2 correction that Kozai's theory adds.
    const double x2o3 = 2.0 / 3.0;
    const double ak = std::pow(kXke / m.meanMotionKozai, x2o3);
    const double d1 = 0.75 * kJ2 * (3.0 * cosio2 - 1.0) / (rteosq * m.omeosq);
    double del = d1 / (ak * ak);
    const double adel = ak * (1.0 - del * del - del * (1.0 / 3.0 + 134.0 * del * del / 81.0));
    del = d1 / (adel * adel);
    m.meanMotion = m.meanMotionKozai / (1.0 + del);

    m.semiMajorAxis = std::pow(kXke / m.meanMotion, x2o3);
    const double po = m.semiMajorAxis * m.omeosq;
    m.posq = po * po;
    m.con42 = 1.0 - 5.0 * cosio2;
    m.con41 = -m.con42 - cosio2 - cosio2;
    const double rp = m.semiMajorAxis * (1.0 - m.eccentricity);
    m.perigeeKm = (rp - 1.0) * kRadiusEarthKm;
    m.gsto = GreenwichMeanSiderealTime(m.epochJd);

    if (kTwoPi / m.meanMotion >= kDeepSpacePeriodMinutes) {
        return std::nullopt;
    }

    // Drag model. Below 220 km perigee the higher-order drag terms are omitted, and below 156 km
    // the atmospheric density parameters s and qoms24 are altered.
    m.isSimple = rp < 220.0 / kRadiusEarthKm + 1.0;

    m.s = 78.0 / kRadiusEarthKm + 1.0;
    const double qzms2ttemp = (120.0 - 78.0) / kRadiusEarthKm;
    m.qoms24 = qzms2ttemp * qzms2ttemp * qzms2ttemp * qzms2ttemp;
    if (m.perigeeKm < 156.0) {
        double sfour = m.perigeeKm - 78.0;
        if (m.perigeeKm < 98.0) {
            sfour = 20.0;
        }
        const double qzms24temp = (120.0 - sfour) / kRadiusEarthKm;
        m.qoms24 = qzms24temp * qzms24temp * qzms24temp * qzms24temp;
        m.s = sfour / kRadiusEarthKm + 1.0;
    }

    m.tsi = 1.0 / (m.semiMajorAxis - m.s);
    m.eta = m.semiMajorAxis * m.eccentricity * m.tsi;
    return m;
}

} // namespace core
