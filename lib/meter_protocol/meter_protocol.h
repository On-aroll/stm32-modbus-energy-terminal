#ifndef METER_PROTOCOL_H
#define METER_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    float phase_voltage_v[3];
    float phase_current_a[3];
    float active_power_kw;
    float power_factor;
    float frequency_hz;
    float load_rate_pct;
} MeterMeasurements;

bool meter_parse_input_registers(const uint8_t *frame, size_t length, MeterMeasurements *measurements);

#endif

