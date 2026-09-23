#include "ui/log_panel.h"

#include "app/format.h"
#include "app/typing_effect.h"
#include "ui/rows.h"

#include <imgui.h>

namespace ui {

void DrawLogPanel(const char* title, const app::EventLog& log, const char* placeholder,
                  std::optional<net::Clock::time_point> typingEffectNow) {
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

        // One line types in at a time (see app::TypingStartTimes): entries whose turn has not
        // come yet are skipped entirely this frame, rather than shown blank or typed in parallel
        // with the one still going.
        const std::vector<net::Clock::time_point> typingStarts =
            typingEffectNow ? app::TypingStartTimes(log.Entries(), app::kTypingCharsPerSecond)
                            : std::vector<net::Clock::time_point>{};

        for (std::size_t i = 0; i < log.Entries().size(); ++i) {
            const app::LogEntry& entry = log.Entries()[i];
            if (typingEffectNow && *typingEffectNow < typingStarts[i]) {
                break; // this entry, and every later one, has not started yet
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", app::FormatUtcTime(entry.time).c_str());
            ImGui::TableNextColumn();
            if (typingEffectNow) {
                const std::string visible = app::TypingEffect(
                    entry.message, *typingEffectNow - typingStarts[i], app::kTypingCharsPerSecond);
                ImGui::TextUnformatted(visible.c_str());
            } else {
                ImGui::TextUnformatted(entry.message.c_str());
            }
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
