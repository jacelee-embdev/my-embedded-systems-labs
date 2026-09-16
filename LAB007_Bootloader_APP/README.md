**English** | [简体中文](README_zh.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# STM32F411 Bootloader and APP Jump Experiment

## Project Description

##### Introduction

This repository contains paired Bootloader and APP projects for the STM32F411CEU6. It demonstrates Flash partitioning, APP validation, and the basic process of jumping from the Bootloader to the APP.

##### Main Features

- The Bootloader checks the APP located at `0x08019000` after reset.
- When the APP is valid, the Bootloader sets the main stack pointer and jumps to it.
- Both projects output status messages through USART1 for easy observation.

##### Repository Structure

```text
task026_Bootloader_APP/
├── 01_Bootloader/    # Bootloader project in the first Flash region
└── 02_APP/           # APP project starting at 0x08019000
```

## Usage Guide

##### Environment and Dependencies

- STM32F411CEU6 development board with a 25 MHz HSE
- Keil MDK-ARM V5.32 and ARM Compiler 5.06
- STM32CubeMX 6.16.1 and STM32F4xx DFP 2.17.1
- A debug probe; a serial terminal and USB-to-UART adapter for observing output

##### Hardware Connection and Configuration

Both projects use USART1 with PA9 as TX and PA10 as RX. The serial format is 115200 baud, 8 data bits, no parity, and 1 stop bit. When using a USB-to-UART adapter, cross-connect TX/RX and connect the grounds.

The Flash layout is:

| Region | Start Address | Size |
| --- | --- | --- |
| Bootloader | `0x08000000` | `0x19000` |
| APP | `0x08019000` | `0x67000` |

##### Build, Run, and Observe

1. Open and build `MDK-ARM/STM32F411CEU6.uvprojx` in each subproject.
2. Flash both the APP and Bootloader while keeping the configured Flash addresses unchanged.
3. Reset the board and open a serial terminal. Bootloader messages appear first, followed by repeated `TEST 1143!` messages from the APP when the jump succeeds.

##### Porting Notes

For another target, update the APP start address in the Bootloader, the APP IROM start and size, and the APP vector-table offset together. Also reconfigure the clock, UART, and Flash capacity for the target MCU.

## Author's Notes

##### Project-Related Reflections

None.

##### Other Reflections

None.

## License

This project is licensed under the **MIT License**.

Copyright (c) 2026 JesMicro. See the [LICENSE](LICENSE) file in the repository root for the full license text.
