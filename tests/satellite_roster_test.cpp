#include "app/observer.h"
#include "app/satellite_roster.h"
#include "app/tracked_satellite.h"

#include <chrono>
#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

// ISS elements from Celestrak, 2026-09-20 (epoch JD 2461304.02959654).
const char* const kIss = "ISS (ZARYA)\n"
                         "1 25544U 98067A   26263.52959654  .00008422  00000+0  15975-3 0  9999\n"
                         "2 25544  51.6308 188.2246 0004825 162.2847 197.8311 15.49196792586535\n";

net::Clock::time_point FromJulianDate(double julianDate) {
    const std::chrono::duration<double> sinceEpoch((julianDate - 2440587.5) * 86400.0);
    return net::Clock::time_point(std::chrono::duration_cast<net::Clock::duration>(sinceEpoch));
}

constexpr double kEpochJd = 2461304.52959654;

// A loader that returns a real satellite for 25544 and nothing for anything else, without
// touching the network.
app::SatelliteLoader FakeLoader() {
    return [](int noradId) -> std::optional<app::TrackedSatellite> {
        if (noradId != 25544) {
            return std::nullopt;
        }
        const auto loaded = net::LoadedTle{kIss, net::Clock::now(), net::TleSource::Network};
        return app::MakeSatellite(loaded);
    };
}

std::vector<app::WatchEntry> Watchlist() {
    return {{25544, "ISS (ZARYA)"}, {99999, "GHOST SATELLITE"}};
}

TEST(SatelliteRoster, LoadsEveryWatchlistEntry) {
    app::SatelliteRoster roster(Watchlist(), app::kObserver, FakeLoader());
    ASSERT_EQ(roster.Size(), 2U);
    EXPECT_TRUE(roster.At(0).satellite.has_value());
    EXPECT_TRUE(roster.At(0).planner.has_value());
    EXPECT_EQ(roster.At(0).entry.noradId, 25544);
}

TEST(SatelliteRoster, AnEntryThatFailsToLoadHasNoSatelliteOrPlanner) {
    app::SatelliteRoster roster(Watchlist(), app::kObserver, FakeLoader());
    EXPECT_FALSE(roster.At(1).satellite.has_value());
    EXPECT_FALSE(roster.At(1).planner.has_value());
    EXPECT_EQ(roster.At(1).entry.noradId, 99999);
    EXPECT_EQ(roster.At(1).entry.name, "GHOST SATELLITE");
}

TEST(SatelliteRoster, SelectsTheFirstEntryByDefault) {
    app::SatelliteRoster roster(Watchlist(), app::kObserver, FakeLoader());
    EXPECT_EQ(roster.SelectedIndex(), 0);
    EXPECT_EQ(&roster.Selected(), &roster.At(0));
}

TEST(SatelliteRoster, SetSelectedIndexChangesTheSelection) {
    app::SatelliteRoster roster(Watchlist(), app::kObserver, FakeLoader());
    roster.SetSelectedIndex(1);
    EXPECT_EQ(roster.SelectedIndex(), 1);
    EXPECT_EQ(&roster.Selected(), &roster.At(1));
}

TEST(SatelliteRoster, SetSelectedIndexClampsToTheValidRange) {
    app::SatelliteRoster roster(Watchlist(), app::kObserver, FakeLoader());
    roster.SetSelectedIndex(50);
    EXPECT_EQ(roster.SelectedIndex(), 1);
    roster.SetSelectedIndex(-5);
    EXPECT_EQ(roster.SelectedIndex(), 0);
}

TEST(SatelliteRoster, SetSelectedIndexOnAnEmptyRosterDoesNothing) {
    app::SatelliteRoster roster({}, app::kObserver, FakeLoader());
    EXPECT_EQ(roster.Size(), 0U);
    roster.SetSelectedIndex(3);
    EXPECT_EQ(roster.SelectedIndex(), 0);
}

TEST(SatelliteRoster, UpdateAdvancesEveryLoadedSatelliteNotOnlyTheSelectedOne) {
    std::vector<app::WatchEntry> watchlist = {{25544, "ISS (ZARYA)"}, {25544, "ISS (ZARYA) 2"}};
    app::SatelliteRoster roster(watchlist, app::kObserver, FakeLoader());
    roster.SetSelectedIndex(0);

    roster.Update(FromJulianDate(kEpochJd));

    EXPECT_TRUE(roster.At(0).position.has_value());
    EXPECT_TRUE(roster.At(1).position.has_value())
        << "the unselected entry should still be updated";
}

TEST(SatelliteRoster, UpdateDoesNotCrashOnAnEntryThatFailedToLoad) {
    app::SatelliteRoster roster(Watchlist(), app::kObserver, FakeLoader());
    roster.Update(FromJulianDate(kEpochJd));
    EXPECT_FALSE(roster.At(1).position.has_value());
    EXPECT_TRUE(roster.At(1).eventLog.Entries().empty());
}

TEST(UpdateWatchedSatellite, PopulatesPositionAndNextPass) {
    app::WatchedSatellite watched;
    watched.entry = {25544, "ISS (ZARYA)"};
    watched.satellite = FakeLoader()(25544);
    ASSERT_TRUE(watched.satellite.has_value());
    watched.planner.emplace(watched.satellite->model, app::kObserver);

    app::UpdateWatchedSatellite(watched, FromJulianDate(kEpochJd));

    EXPECT_TRUE(watched.position.has_value());
    EXPECT_GT(watched.position->geodetic.altitudeKm, 380.0);
    EXPECT_LT(watched.position->geodetic.altitudeKm, 450.0);
}

TEST(UpdateWatchedSatellite, LogsARiseWhenTheSatelliteComesAboveTheHorizon) {
    app::WatchedSatellite watched;
    watched.entry = {25544, "ISS (ZARYA)"};
    watched.satellite = FakeLoader()(25544);
    ASSERT_TRUE(watched.satellite.has_value());
    watched.planner.emplace(watched.satellite->model, app::kObserver);

    // Step forward until the first visibility transition is logged, well within a day.
    bool logged = false;
    for (double minute = 0; minute < 1440 && !logged; ++minute) {
        app::UpdateWatchedSatellite(watched, FromJulianDate(kEpochJd + minute / 1440.0));
        logged = !watched.eventLog.Entries().empty();
    }
    ASSERT_TRUE(logged);
    EXPECT_EQ(watched.eventLog.Entries()[0].message, "SIGNAL ACQUIRED");
}

TEST(UpdateWatchedSatellite, DoesNothingIfTheSatelliteNeverLoaded) {
    app::WatchedSatellite watched;
    watched.entry = {99999, "GHOST SATELLITE"};
    app::UpdateWatchedSatellite(watched, FromJulianDate(kEpochJd));
    EXPECT_FALSE(watched.position.has_value());
    EXPECT_FALSE(watched.nextPass.has_value());
    EXPECT_TRUE(watched.eventLog.Entries().empty());
}

} // namespace
