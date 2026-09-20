#pragma once

#include <chrono>

namespace core {

// A UTC calendar date and time in the Gregorian calendar.
struct CalendarTime {
    int year = 2000;
    int month = 1; // 1-12
    int day = 1;   // 1-31
    int hour = 0;
    int minute = 0;
    double second = 0.0;
};

// Julian date (days since noon UTC on 4713 BC Jan 1) of a calendar time.
double JulianDate(const CalendarTime& time);

// Julian date of a TLE epoch: a four-digit year plus day of year, where 1.0 is Jan 1 00:00 UTC.
double JulianDateFromEpoch(int year, double dayOfYear);

// Julian date of a point on the system clock, which counts from the Unix epoch in UTC.
double JulianDateFromTimePoint(std::chrono::system_clock::time_point time);

// Greenwich mean sidereal time in radians, in [0, 2*pi), using the IAU-1982 model that SGP4 uses.
// UT1 is taken to be UTC; the two differ by less than 0.9 s.
double GreenwichMeanSiderealTime(double julianDate);

} // namespace core
