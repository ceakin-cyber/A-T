#include "ui/event_log_panel.h"

#include "ui/log_panel.h"

namespace ui {

void DrawEventLogPanel(const app::EventLog& log) {
    DrawLogPanel("EVENT LOG", log, "AWAITING EVENTS...");
}

} // namespace ui
