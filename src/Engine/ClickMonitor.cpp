#include "ClickMonitor.h"

// macOS has its own implementation in ClickMonitorMac.mm. Everywhere else there
// is no monitor yet: raylib's button state covers normal clicks there.
#ifndef __APPLE__

struct ClickMonitor::State {
};

ClickMonitor::ClickMonitor() = default;

ClickMonitor::~ClickMonitor() = default;

ClickMonitor::Clicks ClickMonitor::Take() {
    return Clicks{};
}

bool ClickMonitor::IsLeftHeldAsRight() const {
    return false;
}

#endif
