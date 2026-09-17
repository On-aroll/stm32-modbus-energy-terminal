#include "fault_manager.h"

void fault_manager_init(FaultManager *manager) {
    if (manager == 0) {
        return;
    }
    manager->consecutive_failures = 0U;
    manager->successful_polls = 0U;
    manager->failed_polls = 0U;
    manager->status = TERMINAL_DEGRADED;
}

void fault_manager_record_success(FaultManager *manager) {
    if (manager == 0) {
        return;
    }
    manager->consecutive_failures = 0U;
    manager->successful_polls += 1U;
    manager->status = TERMINAL_ONLINE;
}

void fault_manager_record_failure(FaultManager *manager) {
    if (manager == 0) {
        return;
    }
    if (manager->consecutive_failures < UINT8_MAX) {
        manager->consecutive_failures += 1U;
    }
    manager->failed_polls += 1U;
    manager->status = manager->consecutive_failures >= 3U ? TERMINAL_COMMUNICATION_LOST : TERMINAL_DEGRADED;
}

