**English** | [简体中文](README_zh.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)

# STM32F411 UART Interrupt vs DMA Receive-Time Comparison Lab

## Project Overview

##### Introduction

This project compares the CPU overhead of USART interrupt-driven (IT) and DMA reception on an STM32F411, as well as their effects on the execution time of foreground work in the main loop.

##### Main Features

- Switch between IT and DMA reception with one macro;
- Simulate a continuous workload by copying a two-dimensional array;
- Measure the main loop, IRQ handler, and callback with three GPIO trace signals.

##### Repository Structure

```text
Core/       Application code and interrupt handlers
Drivers/    STM32 HAL and CMSIS
MDK-ARM/    Keil project
*.ioc       STM32CubeMX configuration
README*     Chinese and English documentation
```

## Usage Guide

##### Environment and Dependencies

STM32F411CEU6, STM32Cube FW_F4 V1.28.3, Keil µVision 5.38, a serial terminal, and a logic analyzer.

##### Hardware Connections and Configuration

USART1 uses PA9/PA10. Connect the logic analyzer to PB4 (main loop), PB5 (callback), PB6 (IRQ), and GND. Select the receive mode in `Core/Inc/usart.h`:

```c
/* 1: IT reception; 0: DMA reception */
#define USART1_RX_IT_DMA_MODE  1U
```

##### Build, Run, and Observe

Open `MDK-ARM/STM32F411CEU6.uvprojx` in Keil, perform a full build, and flash the target. Configure the serial terminal for 115200 baud, 8N1, and ASCII mode. Send `ABCDEFGHIJKLMNOPQRSTUVWXYZ123456` every 1 ms without appending a newline, then measure the high-pulse widths of the three GPIO signals.

##### Porting Notes

When porting to another STM32, reconfigure the UART pins, DMA stream/channel, and IRQ. Reduce the test-array size on devices with less SRAM.

## Lab Report

##### 1. Background and Principles

- One complete interrupt-driven UART reception:
  1. Before the UART peripheral receives any data, the CPU executes the foreground work in the main loop.
  2. When the UART receives the first byte, the hardware samples the start bit, data bits, and stop bit, places the byte in the `DR` register, and sets the `RXNE` hardware flag.
  3. The UART sends an interrupt request to the NVIC. The CPU saves the current execution context and enters `USARTx_IRQHandler()`, reads the corresponding UART `DR` value, and stores it in an SRAM buffer. Reading the register clears `RXNE`, after which the CPU returns to the interrupted foreground work. Reception of one byte is now complete.
  4. The second and third steps repeat for every received byte. After the configured number of bytes has arrived, the `HAL_UART_RxCpltCallback()` interrupt callback is invoked.
     - In this callback, an application normally sets a new-data flag, notifies a related thread, and restarts reception. `HAL_UART_Receive_IT()` represents a one-shot, fixed-length receive operation.
- One complete DMA-driven UART reception:
  1. Before the UART peripheral receives any data, the CPU executes the foreground work in the main loop.
  2. After the UART receives one byte, the hardware samples the start bit, data bits, and stop bit, writes the data to the USART1 `DR` register, and generates a DMA request.
  3. DMA automatically reads the `DR` register and moves the data into the SRAM receive buffer. The CPU does not need to enter `USARTx_IRQHandler()` for each byte and can continue executing the main-loop workload.
  4. When half of the buffer has been received, DMA generates a half-transfer interrupt and enters `DMAx_Streamx_IRQHandler()`. This interrupt is enabled by default but can be disabled.
  5. After the configured number of bytes has been transferred, `DMAx_Streamx_IRQHandler()` runs again and ultimately calls `HAL_UART_RxCpltCallback()`.
     - In `HAL_UART_RxCpltCallback()`, an application normally sets a new-data flag or notifies a related thread, then starts the next receive operation with `HAL_UART_Receive_DMA()`.
- **For the same receive length, interrupt-driven UART reception interrupts the CPU many times without making use of its ALU or FPU computing capability. DMA interrupts the CPU only twice—or at least once if the half-transfer interrupt is disabled—greatly improving CPU utilization.**
- To measure a function's execution time, its trace GPIO is driven high immediately before the function and low immediately afterward. A logic analyzer measures the resulting pulse width.

##### 2. Hypothesis

With all other conditions unchanged—including hardware, platform, program, and compiler settings—the total overhead of receiving the same amount of data with UART interrupts will be much greater than with UART DMA.

##### 3. Test Platform and Configuration

- STM32F411CEU6
- Logic analyzer

##### 4. Variables and Controlled Conditions

- Independent variable: IT or DMA reception mode.
- Dependent variables: execution times of the main loop, IRQ handler, and callback.
- Controlled variables: data length, transmission interval, core clock, array size, compiler settings, and all other factors.

##### 5. Procedure

Measure and record the main-loop workload, IRQ handler, and callback execution times under the following conditions:

1. No data reception
2. Interrupt-driven UART reception
3. DMA reception

##### 6. Data, Analysis, and Conclusion

| Trace item | No reception | IT reception | DMA reception |
|---|---:|---:|---:|
| Main-loop execution time | 710.4 µs | 721.85 µs | 710.25 µs |
| One receive IRQ | 0 µs | 1.15 µs (first 31) + 2.4 µs (32nd) | Complete 4.65 µs; half transfer 1.5 µs |
| One completion callback | 0 µs | 0.75 µs | 2.45 µs |

**IT increases the main-loop time by 11.45 µs (1.61%), while DMA differs from the baseline by only 0.15 µs, which can be treated as measurement variation. For each 32-byte block, the total IT IRQ overhead is approximately `31 × 1.15 + 2.4 = 38.05 µs`. The DMA half-transfer and completion IRQs total approximately `1.5 + 4.65 = 6.15 µs`, a reduction of about *83.8%*. Although the DMA callback is longer, it runs only once per block, so its overall CPU overhead remains significantly lower than IT. The 32nd IT IRQ and the DMA completion IRQ already include their respective callbacks and must not be counted twice.**

Note: For both IT and DMA reception, the recorded main-loop time comes from a complete main-loop iteration executed entirely while reception was active.

##### 7. Errors and Limitations

The magnitude and precision of the measurements depend on the performance of the target MCU board and the logic analyzer's sample rate. Some measurement error may be present, but the underlying principle is demonstrated.

##### 8. Author’s Reflections

## License

This project is released under the **MIT License**.

Copyright (c) 2026 JesMicro. See the [LICENSE](../LICENSE) file in the repository root for the complete license terms.
