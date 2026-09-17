import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))

from fault_lab import run_fault_sequence
from protocol import build_demo_response, build_read_input_request, crc16_modbus, parse_response


class ProtocolTests(unittest.TestCase):
    def test_standard_crc_vector(self):
        self.assertEqual(crc16_modbus(bytes.fromhex("01 03 00 00 00 0A")), 0xCDC5)

    def test_read_input_request(self):
        self.assertEqual(build_read_input_request().hex(" ").upper(), "01 04 00 00 00 0A 70 0D")

    def test_response_decode(self):
        frame = build_demo_response([2301, 2297, 2304, 321, 308, 315, 2046, 952, 5001, 721])
        values = parse_response(frame)
        self.assertEqual(values.voltage_v, (230.1, 229.7, 230.4))
        self.assertEqual(values.current_a, (32.1, 30.8, 31.5))
        self.assertAlmostEqual(values.power_factor, 0.952)
        self.assertAlmostEqual(values.frequency_hz, 50.01)

    def test_bad_crc_is_rejected(self):
        frame = bytearray(build_demo_response([2300] * 10))
        frame[-1] ^= 0xFF
        with self.assertRaisesRegex(ValueError, "CRC"):
            parse_response(bytes(frame))

    def test_communication_loss_and_recovery(self):
        timeline = run_fault_sequence([True, False, False, False, True])
        self.assertEqual(timeline[0]["status"], "ONLINE")
        self.assertEqual(timeline[2]["status"], "DEGRADED")
        self.assertEqual(timeline[3]["status"], "COMMUNICATION_LOST")
        self.assertEqual(timeline[4]["status"], "ONLINE")
        self.assertEqual(timeline[4]["consecutive_failures"], 0)


if __name__ == "__main__":
    unittest.main()
