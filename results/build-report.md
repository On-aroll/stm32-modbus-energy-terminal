# Firmware build report

Build date: 2026-09-17  
Target: `bluepill_f103c8` / STM32F103C8T6  
Framework: STM32CubeF1 1.8.7  
Toolchain: GNU Arm Embedded 12.3.1 (`toolchain-gccarmnoneeabi` 1.120301.0)  
Command: `pio run`

## Result

```text
RAM:   [=         ]   5.1% (used 1036 bytes from 20480 bytes)
Flash: [===       ]  32.7% (used 21460 bytes from 65536 bytes)
========================= [SUCCESS] Took 5.24 seconds =========================
```

The generated binary is `.pio/build/bluepill_f103c8/firmware.bin` and is intentionally excluded from version control because it is reproducible from source.

## Linked timebase check

The application provides `SysTick_Handler` and calls `HAL_IncTick` to maintain the millisecond timebase used by delays, UART timeouts, and periodic polling. `python tools/check_firmware_symbols.py` checks the linked ELF, rather than only checking source text. The same check runs in CI after compilation.

```text
08000fb4 W HAL_IncTick
08001a68 T SysTick_Handler
08004e5c T Default_Handler
```

`SysTick_Handler` is a strong text symbol at a different address from `Default_Handler`; `HAL_IncTick` is linked into the image. The regression check rejected the preceding ELF, where `SysTick_Handler` was a weak alias of `Default_Handler`, and passed after this fix.

## Host protocol checks

`python -m unittest discover -v`: 6 tests passed. The example power is 20.7 kW, consistent (within its 0.1 kW register resolution) with the provided phase voltages, currents, and common power factor: approximately 20.676 kW. `python tools/frame_lab.py` regenerated the matching response frame and CRC.

## Notes

- The build completed with no project-source errors.
- This incremental build emitted Newlib-Nano unimplemented POSIX syscall warnings and an RWX load-segment linker warning; they did not prevent creation of the firmware image. No POSIX file or console APIs are used for board communication; diagnostic output goes through the HAL UART API.
- This report confirms compilation and linking for the selected target. It does not claim successful flashing or physical meter communication; those steps remain in `hardware/bringup-checklist.md`.
