"""Host-side Modbus frame helper used for protocol verification."""

from dataclasses import dataclass


def crc16_modbus(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc & 0xFFFF


def append_crc(data: bytes) -> bytes:
    crc = crc16_modbus(data)
    return data + bytes((crc & 0xFF, crc >> 8))


def build_read_input_request(slave: int = 1, start: int = 0, quantity: int = 10) -> bytes:
    return append_crc(bytes((slave, 0x04, start >> 8, start & 0xFF, quantity >> 8, quantity & 0xFF)))


def build_demo_response(registers: list[int], slave: int = 1) -> bytes:
    payload = bytearray((slave, 0x04, len(registers) * 2))
    for value in registers:
        payload.extend(((value >> 8) & 0xFF, value & 0xFF))
    return append_crc(bytes(payload))


@dataclass(frozen=True)
class Measurements:
    voltage_v: tuple[float, float, float]
    current_a: tuple[float, float, float]
    active_power_kw: float
    power_factor: float
    frequency_hz: float
    load_rate_pct: float


def parse_response(frame: bytes, expected_slave: int = 1) -> Measurements:
    if len(frame) != 25 or frame[0] != expected_slave or frame[1] != 0x04 or frame[2] != 20:
        raise ValueError("Unexpected response layout")
    if crc16_modbus(frame[:-2]) != int.from_bytes(frame[-2:], "little"):
        raise ValueError("CRC mismatch")
    registers = [int.from_bytes(frame[3 + i * 2:5 + i * 2], "big") for i in range(10)]
    return Measurements(
        voltage_v=tuple(value / 10 for value in registers[0:3]),
        current_a=tuple(value / 10 for value in registers[3:6]),
        active_power_kw=registers[6] / 10,
        power_factor=registers[7] / 1000,
        frequency_hz=registers[8] / 100,
        load_rate_pct=registers[9] / 10,
    )

