#include "ui/log_panel.h"

#include "app/format.h"
#include "ui/rows.h"

#include <imgui.h>

namespace ui {

void DrawLogPanel(const char* title, const app::EventLog& log, const char* placeholder) {
    if (!ImGui::Begin(title)) {
        ImGui::End();
        return;
    }

    if (log.Entries().empty()) {
        PlaceholderText(placeholder);
    } else if (ImGui::BeginTable("##log", 2, ImGuiTableFlags_SizingFixedFit)) {
        // The timestamp sits in the same fixed-width column every other panel uses for its
        // label, so every message starts at the same x position as the tracker and pass values.
        ImGui::TableSetupColumn("time", ImGuiTableColumnFlags_WidthFixed, kLabelColumnWidth);
        ImGui::TableSetupColumn("message", ImGuiTableColumnFlags_WidthStretch);
        for (const app::LogEntry& entry : log.Entries()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", app::FormatUtcTime(entry.time).c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(entry.message.c_str());
        }
        ImGui::EndTable();
        // Auto-scroll to the newest entry, but only while already at the bottom, so scrolling up
        // to read older entries is not fought.
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0F) {
            ImGui::SetScrollHereY(1.0F);
        }
    }
    ImGui::End();
}

} // namespace ui
