**English** | [简体中文](README_zh.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)

# STM32F411 Bootloader Jump Experiment

## Project Description

##### Introduction

This subproject is the Bootloader for the STM32F411CEU6. It checks the APP at a fixed Flash address and transfers execution to it when the image is valid.

##### Main Features

- The Bootloader occupies `0x08000000–0x08018FFF`.
- It validates the APP using the initial MSP stored in the APP vector table.
- It releases USART1, loads the APP MSP, and jumps to the APP reset handler.
- It reports its status through USART1 and remains in the Bootloader if the APP is invalid.

##### Repository Structure

```text
01_Bootloader/
├── Core/       # User code, startup flow, and peripheral initialization
├── Drivers/    # CMSIS and STM32F4 HAL drivers
├── MDK-ARM/    # Keil project and startup file
└── STM32F411CEU6.ioc
```

## Usage Guide

##### Environment and Dependencies

- STM32F411CEU6 development board with a 25 MHz HSE
- Keil MDK-ARM V5.32 and ARM Compiler 5.06
- STM32CubeMX 6.16.1 and STM32F4xx DFP 2.17.1
- A debug probe; a serial terminal and USB-to-UART adapter for observing output

##### Hardware Connection and Configuration

| Signal | STM32 Pin | Connection |
| --- | --- | --- |
| USART1_TX | PA9 | USB-to-UART RX |
| USART1_RX | PA10 | USB-to-UART TX |
| GND | GND | Common ground with the adapter |

The serial format is 115200 baud, 8 data bits, no parity, and 1 stop bit. The system clock is 100 MHz.

##### Build, Run, and Observe

1. Open `MDK-ARM/STM32F411CEU6.uvprojx` in Keil, then build and flash it.
2. Make sure the paired APP is programmed at `0x08019000`, then reset the board.
3. The serial terminal first shows `bootloader running...`. If the APP is valid, execution then moves to the APP. If it is invalid, the Bootloader periodically prints `TEST 1142!`.

##### Porting Notes

When porting, update `APP_FLASH_ADDR` in `Core/Src/main.c` and the IROM layout in the Keil project together. Adapt the clock and USART1 pin configuration for the target MCU as well.

## Author's Notes

##### Project-Related Reflections

None.

##### Other Reflections

None.

## License

This project is licensed under the **MIT License**.

Copyright (c) 2026 JesMicro. See the repository root [LICENSE](../LICENSE) file for the full license text.
