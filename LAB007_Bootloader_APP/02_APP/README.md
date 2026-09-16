**English** | [简体中文](README_zh.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)

# STM32F411 Bootloader APP Experiment

## Project Description

##### Introduction

This subproject is the STM32F411CEU6 APP paired with the Bootloader. It is linked at `0x08019000` and prints a serial message after the Bootloader transfers execution to it.

##### Main Features

- The APP occupies `0x08019000–0x0807FFFF`.
- It relocates the interrupt vector table to `0x08019000` and re-enables global interrupts.
- It initializes the system clock and USART1.
- It prints `TEST 1143!` through USART1 every 2 seconds.

##### Repository Structure

```text
02_APP/
├── Core/       # APP main program and peripheral initialization
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

1. Open `MDK-ARM/STM32F411CEU6.uvprojx` in Keil and confirm that IROM starts at `0x08019000` with a size of `0x67000`.
2. Build and flash the APP, then program the paired Bootloader.
3. Reset the board and watch the serial terminal. After a successful Bootloader jump, `TEST 1143!` is printed every 2 seconds.

##### Porting Notes

Keep the APP IROM start address consistent with the `SCB->VTOR` offset in `Core/Src/main.c` and with the APP start address used by the Bootloader. Also adapt the clock, UART, and available Flash size for the target MCU.

## Author's Notes

##### Project-Related Reflections

None.

##### Other Reflections

None.

## License

This project is licensed under the **MIT License**.

Copyright (c) 2026 JesMicro. See the repository root [LICENSE](../LICENSE) file for the full license text.
