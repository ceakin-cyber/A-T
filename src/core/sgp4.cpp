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

    // Drag coefficients.
    const double etasq = m.eta * m.eta;
    const double eeta = m.eccentricity * m.eta;
    const double psisq = std::fabs(1.0 - etasq);
    const double coef = m.qoms24 * std::pow(m.tsi, 4.0);
    const double coef1 = coef / std::pow(psisq, 3.5);
    const double cc2 =
        coef1 * m.meanMotion *
        (m.semiMajorAxis * (1.0 + 1.5 * etasq + eeta * (4.0 + etasq)) +
         0.375 * kJ2 * m.tsi / psisq * m.con41 * (8.0 + 3.0 * etasq * (8.0 + etasq)));
    m.cc1 = m.bstar * cc2;

    double cc3 = 0.0;
    if (m.eccentricity > 1.0e-4) {
        cc3 = -2.0 * coef * m.tsi * kJ3OverJ2 * m.meanMotion * m.sinInclination / m.eccentricity;
    }
    const double x1mth2 = 1.0 - cosio2;
    m.cc4 =
        2.0 * m.meanMotion * coef1 * m.semiMajorAxis * m.omeosq *
        (m.eta * (2.0 + 0.5 * etasq) + m.eccentricity * (0.5 + 2.0 * etasq) -
         kJ2 * m.tsi / (m.semiMajorAxis * psisq) *
             (-3.0 * m.con41 * (1.0 - 2.0 * eeta + etasq * (1.5 - 0.5 * eeta)) +
              0.75 * x1mth2 * (2.0 * etasq - eeta * (1.0 + etasq)) * std::cos(2.0 * m.argPerigee)));
    m.cc5 = 2.0 * coef1 * m.semiMajorAxis * m.omeosq * (1.0 + 2.75 * (etasq + eeta) + eeta * etasq);

    // Secular rates of the mean anomaly, argument of perigee and node (J2, J2^2 and J4 terms).
    const double cosio4 = cosio2 * cosio2;
    const double pinvsq = 1.0 / m.posq;
    const double temp1 = 1.5 * kJ2 * pinvsq * m.meanMotion;
    const double temp2 = 0.5 * temp1 * kJ2 * pinvsq;
    const double temp3 = -0.46875 * kJ4 * pinvsq * pinvsq * m.meanMotion;
    m.mdot = m.meanMotion + 0.5 * temp1 * rteosq * m.con41 +
             0.0625 * temp2 * rteosq * (13.0 - 78.0 * cosio2 + 137.0 * cosio4);
    m.argpdot = -0.5 * temp1 * m.con42 + 0.0625 * temp2 * (7.0 - 114.0 * cosio2 + 395.0 * cosio4) +
                temp3 * (3.0 - 36.0 * cosio2 + 49.0 * cosio4);
    const double xhdot1 = -temp1 * m.cosInclination;
    m.nodedot =
        xhdot1 + (0.5 * temp2 * (4.0 - 19.0 * cosio2) + 2.0 * temp3 * (3.0 - 7.0 * cosio2)) *
                     m.cosInclination;

    m.omgcof = m.bstar * cc3 * std::cos(m.argPerigee);
    if (m.eccentricity > 1.0e-4) {
        m.xmcof = -x2o3 * coef * m.bstar / eeta;
    }
    m.nodecf = 3.5 * m.omeosq * xhdot1 * m.cc1;
    m.t2cof = 1.5 * m.cc1;
    const double delmotemp = 1.0 + m.eta * std::cos(m.meanAnomaly);
    m.delmo = delmotemp * delmotemp * delmotemp;
    m.sinmao = std::sin(m.meanAnomaly);

    if (!m.isSimple) {
        const double cc1sq = m.cc1 * m.cc1;
        m.d2 = 4.0 * m.semiMajorAxis * m.tsi * cc1sq;
        const double temp = m.d2 * m.tsi * m.cc1 / 3.0;
        m.d3 = (17.0 * m.semiMajorAxis + m.s) * temp;
        m.d4 =
            0.5 * temp * m.semiMajorAxis * m.tsi * (221.0 * m.semiMajorAxis + 31.0 * m.s) * m.cc1;
        m.t3cof = m.d2 + 2.0 * cc1sq;
        m.t4cof = 0.25 * (3.0 * m.d3 + m.cc1 * (12.0 * m.d2 + 10.0 * cc1sq));
        m.t5cof = 0.2 * (3.0 * m.d4 + 12.0 * m.cc1 * m.d3 + 6.0 * m.d2 * m.d2 +
                         15.0 * cc1sq * (2.0 * m.d2 + cc1sq));
    }
    return m;
}

namespace {

// Wraps an angle into [0, 2*pi).
double WrapAngle(double angle) {
    const double wrapped = std::fmod(angle, kTwoPi);
    return wrapped < 0.0 ? wrapped + kTwoPi : wrapped;
}

} // namespace

std::optional<MeanElements> PropagateSecular(const Sgp4Model& m, double t) {
    using namespace wgs72;
    const double x2o3 = 2.0 / 3.0;

    // Secular gravity: the angles drift linearly.
    const double xmdf = m.meanAnomaly + m.mdot * t;
    const double argpdf = m.argPerigee + m.argpdot * t;
    const double nodedf = m.raan + m.nodedot * t;
    double argpm = argpdf;
    double mm = xmdf;
    const double t2 = t * t;
    double nodem = nodedf + m.nodecf * t2;
    double tempa = 1.0 - m.cc1 * t;
    double tempe = m.bstar * m.cc4 * t;
    double templ = m.t2cof * t2;

    // Atmospheric drag, including the higher-order terms unless the orbit is simple.
    if (!m.isSimple) {
        const double delomg = m.omgcof * t;
        const double delmtemp = 1.0 + m.eta * std::cos(xmdf);
        const double delm = m.xmcof * (delmtemp * delmtemp * delmtemp - m.delmo);
        const double temp = delomg + delm;
        mm = xmdf + temp;
        argpm = argpdf - temp;
        const double t3 = t2 * t;
        const double t4 = t3 * t;
        tempa = tempa - m.d2 * t2 - m.d3 * t3 - m.d4 * t4;
        tempe = tempe + m.bstar * m.cc5 * (std::sin(mm) - m.sinmao);
        templ = templ + m.t3cof * t3 + t4 * (m.t4cof + t * m.t5cof);
    }

    if (m.meanMotion <= 0.0) {
        return std::nullopt;
    }
    MeanElements e;
    e.semiMajorAxis = std::pow(kXke / m.meanMotion, x2o3) * tempa * tempa;
    e.meanMotion = kXke / std::pow(e.semiMajorAxis, 1.5);
    e.eccentricity = m.eccentricity - tempe;
    if (e.eccentricity >= 1.0 || e.eccentricity < -0.001) {
        return std::nullopt;
    }
    if (e.eccentricity < 1.0e-6) {
        e.eccentricity = 1.0e-6;
    }

    mm += m.meanMotion * templ;
    const double xlm = mm + argpm + nodem;
    e.inclination = m.inclination;
    e.raan = WrapAngle(nodem);
    e.argPerigee = WrapAngle(argpm);
    e.meanAnomaly = WrapAngle(WrapAngle(xlm) - e.argPerigee - e.raan);
    return e;
}

} // namespace core
