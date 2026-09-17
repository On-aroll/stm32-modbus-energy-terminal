#include "meter_protocol.h"

#include "modbus_rtu.h"

#define METER_REGISTER_COUNT 10U

static uint16_t read_u16_be(const uint8_t *data) {
    return ((uint16_t)data[0] << 8U) | data[1];
}

bool meter_parse_input_registers(const uint8_t *frame, size_t length, MeterMeasurements *measurements) {
    if (measurements == NULL || !modbus_validate_read_response(frame, length, frame != NULL ? frame[0] : 0U, METER_REGISTER_COUNT)) {
        return false;
    }

    uint16_t registers[METER_REGISTER_COUNT];
    for (size_t index = 0; index < METER_REGISTER_COUNT; ++index) {
        registers[index] = read_u16_be(&frame[3U + index * 2U]);
    }

    for (size_t phase = 0; phase < 3U; ++phase) {
        measurements->phase_voltage_v[phase] = registers[phase] / 10.0F;
        measurements->phase_current_a[phase] = registers[phase + 3U] / 10.0F;
    }
    measurements->active_power_kw = registers[6] / 10.0F;
    measurements->power_factor = registers[7] / 1000.0F;
    measurements->frequency_hz = registers[8] / 100.0F;
    measurements->load_rate_pct = registers[9] / 10.0F;
    return true;
}

