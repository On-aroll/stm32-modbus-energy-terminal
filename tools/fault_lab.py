"""Exercise the communication-state recovery policy without target hardware."""

import json
from dataclasses import dataclass
from pathlib import Path


@dataclass
class CommunicationMonitor:
    consecutive_failures: int = 0
    successful_polls: int = 0
    failed_polls: int = 0
    status: str = "DEGRADED"

    def record(self, success: bool) -> None:
        if success:
            self.consecutive_failures = 0
            self.successful_polls += 1
            self.status = "ONLINE"
            return
        self.consecutive_failures = min(255, self.consecutive_failures + 1)
        self.failed_polls += 1
        self.status = "COMMUNICATION_LOST" if self.consecutive_failures >= 3 else "DEGRADED"


def run_fault_sequence(sequence: list[bool]) -> list[dict[str, int | str | bool]]:
    monitor = CommunicationMonitor()
    timeline: list[dict[str, int | str | bool]] = []
    for poll, success in enumerate(sequence, start=1):
        monitor.record(success)
        timeline.append(
            {
                "poll": poll,
                "response_valid": success,
                "consecutive_failures": monitor.consecutive_failures,
                "successful_polls": monitor.successful_polls,
                "failed_polls": monitor.failed_polls,
                "status": monitor.status,
            }
        )
    return timeline


def main() -> None:
    sequence = [True, False, False, False, False, True, True]
    report = {
        "mode": "host fault-injection simulation",
        "scenario": "valid -> four timeouts -> recovery",
        "timeline": run_fault_sequence(sequence),
    }
    output = Path(__file__).resolve().parents[1] / "results"
    output.mkdir(exist_ok=True)
    (output / "fault-injection-report.json").write_text(
        json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
