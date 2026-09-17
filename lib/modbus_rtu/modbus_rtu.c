#include "modbus_rtu.h"

uint16_t modbus_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFFU;
    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8U; ++bit) {
            if ((crc & 0x0001U) != 0U) {
                crc = (uint16_t)((crc >> 1U) ^ 0xA001U);
            } else {
                crc >>= 1U;
            }
        }
    }
    return crc;
}

size_t modbus_build_read_input_request(
    uint8_t slave_id,
    uint16_t start_address,
    uint16_t quantity,
    uint8_t *output,
    size_t output_capacity) {
    if (output == NULL || output_capacity < 8U || quantity == 0U || quantity > 125U) {
        return 0U;
    }
    output[0] = slave_id;
    output[1] = 0x04U;
    output[2] = (uint8_t)(start_address >> 8U);
    output[3] = (uint8_t)(start_address & 0xFFU);
    output[4] = (uint8_t)(quantity >> 8U);
    output[5] = (uint8_t)(quantity & 0xFFU);
    const uint16_t crc = modbus_crc16(output, 6U);
    output[6] = (uint8_t)(crc & 0xFFU);
    output[7] = (uint8_t)(crc >> 8U);
    return 8U;
}

bool modbus_validate_read_response(
    const uint8_t *frame,
    size_t length,
    uint8_t expected_slave,
    uint16_t expected_registers) {
    const size_t expected_length = 5U + (size_t)expected_registers * 2U;
    if (frame == NULL || length != expected_length || expected_registers > 125U) {
        return false;
    }
    if (frame[0] != expected_slave || frame[1] != 0x04U || frame[2] != expected_registers * 2U) {
        return false;
    }
    const uint16_t received_crc = (uint16_t)frame[length - 2U] | ((uint16_t)frame[length - 1U] << 8U);
    return modbus_crc16(frame, length - 2U) == received_crc;
}

