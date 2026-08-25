**English** | [简体中文](README_zh.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)

# STM32F411 FreeRTOS Queue-by-Value vs. Queue-by-Pointer ISR Timing Experiment

## Project Overview

##### Introduction

This project uses an STM32F411CEU6 and FreeRTOS to compare the time required by `xQueueSendFromISR()` to transfer a structure by value or by pointer. A logic analyzer captures the key input and GPIO trace signal to show the difference between the two methods.

##### Main Features

- Select queue-by-value or queue-by-pointer with a compile-time switch.
- Change the event-structure size through `test_payload`.
- Trigger the key interrupt on PA0 and output a measurement marker on PB12.

##### Repository Structure

```text
.
├── BSP/key/             # Queue-by-value and queue-by-pointer key implementations
├── BSP/led/             # LED driver and task
├── Core/                # Main program, GPIO, and FreeRTOS configuration
├── Drivers/             # STM32 HAL and CMSIS
├── Middlewares/         # FreeRTOS source
├── MDK-ARM/             # Keil project files
└── STM32F411CEU6.ioc    # STM32CubeMX configuration
```

## Project Usage Guide

##### Environment and Dependencies

| Type | Configuration |
|---|---|
| MCU | STM32F411CEU6, 100 MHz |
| RTOS / firmware package | FreeRTOS V10.3.1, STM32Cube FW_F4 V1.28.3 |
| Toolchain | Keil MDK-ARM 5, ARM Compiler 5.06 update 7 |
| Hardware tools | SWD programmer/debugger and logic analyzer |

The HAL, CMSIS, and FreeRTOS source files are included in the project.

##### Hardware Connections and Configuration

| Signal | Pin | Purpose |
|---|---|---|
| KEY1 | PA0 | Pull-up input; falling edge triggers EXTI0 |
| INT_TRACE | PB12 | Push-pull output providing the logic-analyzer marker |
| GND | GND | Common ground between the board and logic analyzer |

Connect at least PA0, PB12, and GND to the logic analyzer.

##### Build, Run, and Observe

1. Open `MDK-ARM/STM32F411CEU6.uvprojx` in Keil.
2. Select the transfer method in `BSP/key/inc/bsp_key_config.h`:

   ```c
   #define BSP_KEY_QUEUE_USE_POINTER    (0U)
   ```

   - `0U`: use `bsp_key_queue_value.c/.h` and transfer the complete structure.
   - `1U`: use `bsp_key_queue_pointer.c/.h` and transfer a structure pointer.

3. Set `test_payload` to the same length in both headers. Comment out the array in both headers for the 8 Byte baseline.
4. Rebuild the project, flash the firmware, and press KEY1.
5. Measure the interval from the PA0 falling edge to the PB12 toggle edge, then record the result for each mode.

> The current pointer implementation reuses one static event object and is intended only for this controlled experiment. Use an object pool or another buffer scheme with explicit ownership in high-concurrency applications.

##### Porting Notes

When porting to another MCU or board, reconfigure the system clock, KEY1, and INT_TRACE pins, and verify that the EXTI interrupt priority meets the FreeRTOS requirements. Compiler enum sizes and memory-alignment rules may change the structure size, so repeat the measurements after porting.

## Experiment Report

##### 1. Background and Principle

1. Interrupt service routines (ISRs) are generally expected to enter and exit quickly and are non-blocking. Therefore, we typically perform only lightweight operations in an ISR, such as changing an event flag.
2. In many cases, we need to optimize the code in an interrupt service routine and shorten its execution time to improve overall system performance.
3. Before a section of code begins, we set its trace GPIO pin high, and set it low after the code finishes. A logic analyzer measures the pulse width, which serves as an approximation of the code's execution time.
4. **FreeRTOS queues are implemented using value copying**. When transferring a large amount of data, transferring a pointer provides better performance than transferring the original data directly.

##### 2. Experiment Hypothesis

Assume that, in a FreeRTOS interrupt service routine, the time consumed by using xQueueSendFromISR to transfer a pointer to the original data

is less than

the time consumed when transferring the value of the original data.

##### 3. Experiment Platform and Configuration

- STM32F4211CEU6
- Keil MDK

##### 4. Variables and Control Conditions

- Independent variable 1: whether a FreeRTOS queue transfers the value of the original data or a pointer to the original data
- Independent variable 2: the size of the data object to be transferred
- Dependent variable: the execution time of the interrupt service routine IRQ
- Control variables: all other variables remain the same

##### 5. Experiment Procedure

For both queue-by-value and queue-by-pointer, measure the execution time of the interrupt service routine IRQ while increasing the transferred data-object size from 8 Byte to 136 Byte, 264 Byte, and 408 Byte.

##### 6. Experiment Data, Analysis, and Conclusion

| Test case (total structure size)          | By value | By pointer | Reduction | Reduction rate | Average reduction per Byte |
| ----------------------------------------- | -------: | ---------: | --------: | -------------: | -------------------------: |
| Baseline: array commented out (8 Byte)    | 10.85 µs |   10.85 µs |      0 µs |             0% |                          0 |
| `uint32_t test_payload[32]` (136 Byte)    | 20.40 µs |   10.85 µs |   9.55 µs |         46.81% |             0.0746 µs/Byte |
| `uint32_t test_payload[64]` (264 Byte)    | 29.30 µs |   10.85 µs |  18.45 µs |         62.97% |             0.0695 µs/Byte |
| `uint32_t test_payload[100]` (408 Byte)   | 39.40 µs |   10.85 µs |  28.55 µs |         72.46% |             0.0701 µs/Byte |

The larger the structure, the greater the benefit of pointer transfer. At 408 Byte, the latency is reduced by 28.55 µs, or 72.46%. Based on the PC, logic analyzer, MCU, and software configuration used in this experiment, pointer transfer reduces the time by approximately **0.071 µs** on average for each additional Byte in the structure. This value describes only the trend in these measurements and is not a fixed cross-platform metric.

##### 7. Errors and Limitations

The magnitude and accuracy of the collected data are affected by the performance of the MCU board under test and the sample rate of the logic analyzer. The data may contain some error, but the hypothesis was verified.

##### 8. Author's Reflections

In real-world development, when a large amount of data must be transferred—especially with FreeRTOS queue-send APIs, and even more so when sending from an interrupt—using pointer transfer is recommended as an optimization.

This approximately “zero-copy” pointer-based optimization can reduce program execution time and effectively improve program performance.

## License

This project is released under the **MIT License**.

Copyright (c) 2026 JesMicro. See the [LICENSE](../LICENSE) file in the repository root for the complete license terms.
