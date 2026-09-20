#include "app/tracked_satellite.h"

#include <iostream>

namespace app {

std::optional<TrackedSatellite> MakeSatellite(const net::LoadedTle& loaded) {
    const std::optional<core::Tle> tle = core::ParseTle(loaded.text);
    if (!tle) {
        std::cerr << "TLE is malformed (bad format or checksum)\n";
        return std::nullopt;
    }

    const std::optional<core::Sgp4Model> model = core::InitSgp4(*tle);
    if (!model) {
        std::cerr << "Satellite " << tle->catalogNumber
                  << " is not supported: its orbit is deep-space (period of 225 minutes or "
                     "more) or its elements are invalid\n";
        return std::nullopt;
    }

    return TrackedSatellite{*tle, *model, loaded.fetchedAt, loaded.source};
}

std::optional<TrackedSatellite> LoadSatellite(int noradId) {
    const std::optional<net::LoadedTle> loaded = net::LoadTle(noradId);
    if (!loaded) {
        return std::nullopt;
    }
    return MakeSatellite(*loaded);
}

} // namespace app
