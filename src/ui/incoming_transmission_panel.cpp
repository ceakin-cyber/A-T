#include "ui/incoming_transmission_panel.h"

#include "ui/log_panel.h"

namespace ui {

void DrawIncomingTransmissionPanel(const app::EventLog& log, net::Clock::time_point now) {
    DrawLogPanel("INCOMING TRANSMISSION", log, "AWAITING TRANSMISSION...", now);
}

} // namespace ui
