#include "core/kepler.h"

#include <cmath>

namespace core {

namespace {

constexpr double kTolerance = 1e-12;
constexpr int kMaxIterations = 10;
constexpr double kMaxStep = 0.95;

} // namespace

double SolveKepler(double meanAnomaly, double eccentricity) {
    // With w = 0 the general form reduces to the classical equation.
    return SolveKeplerSgp4(meanAnomaly, eccentricity, 0.0);
}

double SolveKeplerSgp4(double u, double axnl, double aynl) {
    double eo1 = u;
    double step = 1.0;
    for (int iteration = 0; iteration < kMaxIterations && std::fabs(step) >= kTolerance;
         ++iteration) {
        const double sinE = std::sin(eo1);
        const double cosE = std::cos(eo1);
        const double derivative = 1.0 - cosE * axnl - sinE * aynl;
        step = (u - aynl * cosE + axnl * sinE - eo1) / derivative;
        if (std::fabs(step) >= kMaxStep) {
            step = step > 0.0 ? kMaxStep : -kMaxStep;
        }
        eo1 += step;
    }
    return eo1;
}

} // namespace core
