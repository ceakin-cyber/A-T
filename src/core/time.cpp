#include "core/time.h"

#include <cmath>
#include <numbers>

namespace core {

namespace {

constexpr double kJulianDateOfUnixEpoch = 2440587.5;
constexpr double kJulianDateOfJ2000 = 2451545.0;
constexpr double kDaysPerCentury = 36525.0;
constexpr double kSecondsPerDay = 86400.0;

} // namespace

double JulianDate(const CalendarTime& time) {
    // Meeus, "Astronomical Algorithms", chapter 7 (Gregorian calendar).
    int year = time.year;
    int month = time.month;
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    const int century = year / 100;
    const int gregorianCorrection = 2 - century + century / 4;

    const double dayFraction = (time.hour + (time.minute + time.second / 60.0) / 60.0) / 24.0;
    return std::floor(365.25 * (year + 4716)) + std::floor(30.6001 * (month + 1)) + time.day +
           gregorianCorrection - 1524.5 + dayFraction;
}

double JulianDateFromEpoch(int year, double dayOfYear) {
    return JulianDate({year, 1, 1}) + (dayOfYear - 1.0);
}

double JulianDateFromTimePoint(std::chrono::system_clock::time_point time) {
    const std::chrono::duration<double> sinceEpoch = time.time_since_epoch();
    return kJulianDateOfUnixEpoch + sinceEpoch.count() / kSecondsPerDay;
}

double GreenwichMeanSiderealTime(double julianDate) {
    // Vallado, "Fundamentals of Astrodynamics and Applications", algorithm 15. The polynomial
    // gives sidereal time in seconds; 240 seconds of time correspond to one degree.
    const double t = (julianDate - kJulianDateOfJ2000) / kDaysPerCentury;
    const double seconds = -6.2e-6 * t * t * t + 0.093104 * t * t +
                           (876600.0 * 3600.0 + 8640184.812866) * t + 67310.54841;

    const double degrees = std::fmod(seconds / 240.0, 360.0);
    const double radians = degrees * std::numbers::pi / 180.0;
    return radians < 0.0 ? radians + 2.0 * std::numbers::pi : radians;
}

} // namespace core
