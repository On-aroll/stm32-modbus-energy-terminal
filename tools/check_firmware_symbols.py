"""Check the linked ELF uses a real SysTick handler and retains the HAL tick."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess


def locate_nm() -> str:
    executable = shutil.which("arm-none-eabi-nm")
    if executable:
        return executable
    core_dir = Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio"))
    try:
        from platformio.project.config import ProjectConfig
    except ImportError:
        pass
    else:
        core_dir = Path(ProjectConfig.get_instance().get("platformio", "core_dir"))
    suffix = ".exe" if os.name == "nt" else ""
    bundled = core_dir / "packages" / "toolchain-gccarmnoneeabi" / "bin" / f"arm-none-eabi-nm{suffix}"
    if bundled.is_file():
        return str(bundled)
    raise SystemExit("arm-none-eabi-nm not found; pass --nm with its installed toolchain path")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, default=Path(".pio/build/bluepill_f103c8/firmware.elf"))
    parser.add_argument("--nm", help="Path to arm-none-eabi-nm (auto-detected if omitted)")
    args = parser.parse_args()
    if not args.elf.is_file():
        raise SystemExit(f"Firmware ELF not found: {args.elf}; run pio run first")
    result = subprocess.run(
        [args.nm or locate_nm(), "-n", str(args.elf)], check=True, capture_output=True, text=True
    )
    symbols = {}
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3:
            address, kind, name = parts
            symbols[name] = (address, kind)
    systick = symbols.get("SysTick_Handler")
    if systick is None or systick[1] != "T":
        raise SystemExit(f"SysTick_Handler must be a strong global text symbol, got {systick}")
    default = symbols.get("Default_Handler")
    if default is not None and systick[0] == default[0]:
        raise SystemExit("SysTick_Handler resolves to Default_Handler")
    tick = symbols.get("HAL_IncTick")
    if tick is None or tick[1] not in {"T", "t", "W"}:
        raise SystemExit(f"HAL_IncTick must be linked into the firmware, got {tick}")
    print(f"PASS SysTick_Handler: 0x{systick[0]} ({systick[1]}); HAL_IncTick: 0x{tick[0]} ({tick[1]})")
    print("This checks linked timebase symbols; it does not replace a hardware runtime test.")


if __name__ == "__main__":
    main()
