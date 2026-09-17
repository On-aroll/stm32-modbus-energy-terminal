"""Generate a reproducible request/response transcript without hardware."""

import json
from pathlib import Path

from protocol import build_demo_response, build_read_input_request, parse_response

DEMO_REGISTERS = [2301, 2297, 2304, 321, 308, 315, 207, 952, 5001, 721]


def main() -> None:
    request = build_read_input_request()
    response = build_demo_response(DEMO_REGISTERS)
    measurements = parse_response(response)
    report = {
        "mode": "host protocol simulation",
        "request_hex": request.hex(" ").upper(),
        "response_hex": response.hex(" ").upper(),
        "decoded": measurements.__dict__,
    }
    output = Path(__file__).resolve().parents[1] / "results"
    output.mkdir(exist_ok=True)
    (output / "protocol-report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
