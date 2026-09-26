#include "ui/relay_queue_panel.h"

#include "app/format.h"
#include "ui/rows.h"
#include "ui/style.h"

#include <imgui.h>
#include <string>

namespace ui {

namespace {

// How many held messages to list after the armed one, so the queue fits the short bottom panel.
constexpr std::size_t kHoldRows = 3;

// One row: the item's label on the left (marked with ">" when it is the one armed to go next),
// its state on the right. Armed stands out in amber; sent is dimmed, being done with; hold and
// the rest are the panel's normal green.
void ItemRow(const app::RelayItem& item) {
    ImGui::TableNextRow();
    const bool armed = item.state == app::RelayState::Armed;
    const bool sent = item.state == app::RelayState::Sent;
    const ImVec4 color = armed  ? WarningColor()
                         : sent ? ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)
                                : ImGui::GetStyleColorVec4(ImGuiCol_Text);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(armed ? ">" : " ");
    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(item.label.c_str());
    ImGui::TableSetColumnIndex(2);
    ImGui::TextUnformatted(app::ToString(item.state));
    ImGui::PopStyleColor();
}

} // namespace

void DrawRelayQueuePanel(const app::RelayItem& carrierPing, const app::RelayQueue& queue,
                         net::Clock::time_point now) {
    // Docked into the dashboard's dockspace by default (see ui::DrawDockSpace); this position is
    // only used the first time the window ever appears, before it has a dock to fall into.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 20.0F, viewport->Pos.y + 20.0F),
                            ImGuiCond_FirstUseEver);

    if (ImGui::Begin("RELAY QUEUE")) {
        // The schedule first, where the panel's short default height never hides it.
        if (queue.Empty()) {
            PlaceholderText("NO MESSAGES QUEUED");
        } else {
            const std::string next = app::FormatCountdown(queue.NextAt() - now);
            const std::string last =
                queue.LastSentAt() ? app::FormatUtcClock(*queue.LastSentAt(), now) : "NONE YET";
            ImGui::TextDisabled("NEXT SEND IN %s   LAST SENT %s", next.c_str(), last.c_str());
        }

        if (ImGui::BeginTable("##relay", 3, ImGuiTableFlags_SizingFixedFit)) {
            ImGui::TableSetupColumn("marker", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("state", ImGuiTableColumnFlags_WidthFixed);

            ItemRow(carrierPing);
            for (const app::RelayItem& item : queue.Items(kHoldRows)) {
                ItemRow(item);
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();
}

} // namespace ui
