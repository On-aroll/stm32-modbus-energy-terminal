#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint16_t modbus_crc16(const uint8_t *data, size_t length);
size_t modbus_build_read_input_request(
    uint8_t slave_id,
    uint16_t start_address,
    uint16_t quantity,
    uint8_t *output,
    size_t output_capacity);
bool modbus_validate_read_response(
    const uint8_t *frame,
    size_t length,
    uint8_t expected_slave,
    uint16_t expected_registers);

#endif

