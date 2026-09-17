#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include <stdint.h>

typedef enum {
    TERMINAL_ONLINE = 0,
    TERMINAL_DEGRADED,
    TERMINAL_COMMUNICATION_LOST
} TerminalStatus;

typedef struct {
    uint8_t consecutive_failures;
    uint32_t successful_polls;
    uint32_t failed_polls;
    TerminalStatus status;
} FaultManager;

void fault_manager_init(FaultManager *manager);
void fault_manager_record_success(FaultManager *manager);
void fault_manager_record_failure(FaultManager *manager);

#endif

