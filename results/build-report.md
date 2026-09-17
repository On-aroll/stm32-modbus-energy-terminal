# Firmware build report

Build date: 2026-09-17  
Target: `bluepill_f103c8` / STM32F103C8T6  
Framework: STM32CubeF1 1.8.7  
Toolchain: GNU Arm Embedded 12.3.1 (`toolchain-gccarmnoneeabi` 1.120301.0)  
Command: `pio run`

## Result

```text
RAM:   [=         ]   5.1% (used 1036 bytes from 20480 bytes)
Flash: [===       ]  32.7% (used 21436 bytes from 65536 bytes)
======================== [SUCCESS] Took 159.79 seconds ========================
```

The generated binary is `.pio/build/bluepill_f103c8/firmware.bin` and is intentionally excluded from version control because it is reproducible from source.

## Notes

- The build completed with no project-source errors.
- Warnings emitted during this build came from the vendor STM32CubeF1 HAL/LL sources and Newlib-Nano's unimplemented POSIX syscall stubs; they do not prevent creation of the firmware image.
- This report confirms compilation and linking for the selected target. It does not claim successful flashing or physical meter communication; those steps remain in `hardware/bringup-checklist.md`.
