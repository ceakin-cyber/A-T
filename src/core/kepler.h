#pragma once

namespace core {

// Solves the classical Kepler equation E - e*sin(E) = M for the eccentric anomaly E, in radians.
// meanAnomaly is in radians and should be in [0, 2*pi); the result is on the same branch, so a
// mean anomaly in that range gives an eccentric anomaly in [0, 2*pi). Meant for eccentricities
// below about 0.9; it does not converge reliably for orbits close to parabolic.
double SolveKepler(double meanAnomaly, double eccentricity);

// The form of Kepler's equation used inside SGP4, written with the non-singular terms
// axnl = e*cos(w) and aynl = e*sin(w) (w is the argument of perigee):
//     E' - axnl*sin(E') + aynl*cos(E') = u
// where u is the mean longitude measured from the node and E' = E + w is the eccentric
// longitude. Solved by Newton's method exactly as the reference implementation does: at most 10
// iterations, stopping once a step is below 1e-12, with each step limited to +-0.95 rad.
double SolveKeplerSgp4(double u, double axnl, double aynl);

} // namespace core
