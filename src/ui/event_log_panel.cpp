#include "ui/event_log_panel.h"

#include "app/format.h"
#include "ui/rows.h"

#include <imgui.h>

namespace ui {

void DrawEventLogPanel(const app::EventLog& log) {
    if (!ImGui::Begin("EVENT LOG")) {
        ImGui::End();
        return;
    }

    if (log.Entries().empty()) {
        PlaceholderText("AWAITING EVENTS...");
    } else {
        for (const app::LogEntry& entry : log.Entries()) {
            ImGui::TextDisabled("%s", app::FormatUtcTime(entry.time).c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(entry.message.c_str());
        }
        // Auto-scroll to the newest entry, but only while already at the bottom, so scrolling up
        // to read older entries is not fought.
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0F) {
            ImGui::SetScrollHereY(1.0F);
        }
    }
    ImGui::End();
}

} // namespace ui
