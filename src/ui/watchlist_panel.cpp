#include "ui/watchlist_panel.h"

#include "ui/rows.h"
#include "ui/style.h"

#include <cstdio>
#include <imgui.h>

namespace ui {

void DrawWatchlistPanel(app::SatelliteRoster& roster) {
    if (!ImGui::Begin("SATELLITES")) {
        ImGui::End();
        return;
    }

    if (roster.Size() == 0) {
        PlaceholderText("NO DATA");
        ImGui::End();
        return;
    }

    for (std::size_t i = 0; i < roster.Size(); ++i) {
        const app::WatchedSatellite& watched = roster.At(i);
        const bool loaded = watched.satellite.has_value();
        const ImVec4 color = loaded ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : CriticalColor();

        char label[112];
        const char* name = watched.entry.name.empty() ? "UNNAMED" : watched.entry.name.c_str();
        if (loaded) {
            std::snprintf(label, sizeof label, "%5d  %s##sat%zu", watched.entry.noradId, name, i);
        } else {
            std::snprintf(label, sizeof label, "%5d  %s  (OFFLINE)##sat%zu", watched.entry.noradId,
                          name, i);
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        const bool selected = static_cast<int>(i) == roster.SelectedIndex();
        if (ImGui::Selectable(label, selected)) {
            roster.SetSelectedIndex(static_cast<int>(i));
        }
        ImGui::PopStyleColor();
    }
    ImGui::End();
}

} // namespace ui
