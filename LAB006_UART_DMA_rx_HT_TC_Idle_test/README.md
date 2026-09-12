**English** | [简体中文](README_zh.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# STM32F411CEU6 UART Circular DMA HT/TC/IDLE Event Experiment

## Project Description

### Overview

This project verifies the HT, TC, and IDLE event behavior and callback
parameters of STM32F411CEU6 UART Receive-to-Idle with circular DMA reception.

### Features

- Starts USART1 circular DMA reception with
  `HAL_UARTEx_ReceiveToIdle_DMA()`.
- Tests HT, TC, and IDLE events with a 10-byte buffer.
- Logs event types and `Size` through EasyLogger and SEGGER RTT.
- Starts reception from a FreeRTOS test task.

### Project Layout

```text
Test/                         UART DMA experiment code
Core/                         STM32CubeMX initialization and interrupt code
Docs/Images/                  Experiment screenshots
Middlewares_User/EasyLogger/  EasyLogger component
Middlewares_User/SEGGER_RTT/  SEGGER RTT output
MDK-ARM/                      Keil project files
```

## Usage Guide

### Environment and Dependencies

| Item | Configuration |
| --- | --- |
| MCU / clock | STM32F411CEU6 / 100 MHz |
| IDE / compiler | Keil MDK 5 / ARM Compiler 5 |
| RTOS | FreeRTOS (CMSIS-RTOS V2) |
| Logging | EasyLogger 2.2.99 / SEGGER RTT 8.12g |
| Debug tools | J-Link, RTT Viewer, serial terminal |

### Hardware and Configuration

| Item | Configuration |
| --- | --- |
| USART1 TX / RX | PA9 / PA10 |
| UART settings | 115200, 8N1, no flow control |
| USART1 RX DMA | DMA2 Stream2, Channel 4, Circular |
| DMA buffer | 10 bytes |
| Wiring | USB-to-UART TX to PA10, with a shared ground |

### Build, Run, and Observe

1. Open `MDK-ARM/STM32F411CEU6.uvprojx` in Keil, build, and flash it.
2. Connect J-Link RTT Viewer to the board and open Terminal 0.
3. Select HEX transmission in the serial tool and disable appended CR/LF.
4. Reset the board before each test, send the selected data, and record logs.

### Porting Notes

Update the UART handle, GPIO, DMA Stream/Channel, and interrupt configuration
when porting. Keep DMA in Circular mode. If the buffer length changes, update
the interpretation of HT, TC, and `Size` positions accordingly.

## Experiment Report

### 1. Background and Principle

With UART Receive-to-Idle and DMA enabled, data of different lengths is sent
in a single transfer. The resulting DMA half-transfer, DMA transfer-complete,
and UART idle events provide a basis for calculating circular-buffer head and
tail positions.

### 2. Hypothesis

None.

### 3. Platform and Configuration

DMA and UART reception are enabled, and Receive-to-Idle reception is started
with `HAL_UARTEx_ReceiveToIdle_DMA()`.

### 4. Variables and Controlled Conditions

- Independent variable: length of one continuous transmission.
- Dependent variables: HT, TC, and IDLE counts and their `Size` values.
- Controlled variables: all other test conditions.

### 5. Procedure

Keep all other conditions unchanged, send different lengths of data, and
record the HT, TC, and IDLE counts. Reset the system before each new test. The
transmission lengths cover:

1. Less than half of the buffer.
2. Exactly half of the buffer.
3. More than half but less than the full buffer.
4. Exactly one full buffer.
5. More than one full buffer.

### 6. Data, Analysis, and Conclusion

The buffer size is 10 bytes. Each test uses one continuous transmission, and
the system is reset first so DMA starts from the beginning of the buffer.

| Test | Length category (selected value) | HT | TC | IDLE | Callback sequence |
| :---: | :---: | :---: | :---: | :---: | :--- |
| 1 | 1–4 (3 bytes) | 0 | 0 | 1 | IDLE(Size=3) |
| 2 | 5 bytes | 1 | 0 | 1 | HT(Size=5) → IDLE(Size=5) |
| 3 | 6–9 (8 bytes) | 1 | 0 | 1 | HT(Size=5) → IDLE(Size=8) |
| 4 | 10 bytes | 1 | 1 | 1 | HT(Size=5) → TC(Size=10) → IDLE(Size=10) |
| 5 | 11–14 (12 bytes) | 1 | 1 | 1 | HT(Size=5) → TC(Size=10) → IDLE(Size=2) |
| 6 | 15 bytes | 2 | 1 | 1 | HT(Size=5) → TC(Size=10) → HT(Size=5) → IDLE(Size=5) |
| 7 | 16–19 (18 bytes) | 2 | 1 | 1 | HT(Size=5) → TC(Size=10) → HT(Size=5) → IDLE(Size=8) |
| 8 | 20 bytes | 2 | 2 | 1 | HT(Size=5) → TC(Size=10) → HT(Size=5) → TC(Size=10) → IDLE(Size=10) |
| 9 | 21–24 (22 bytes) | 2 | 2 | 1 | HT(Size=5) → TC(Size=10) → HT(Size=5) → TC(Size=10) → IDLE(Size=2) |

Experiment screenshots:

1. Test 1: Send 3 bytes (1–4 byte category)

   ![Test 1](Docs/Images/1.png)

2. Test 2: Send 5 bytes

   ![Test 2](Docs/Images/2.png)

3. Test 3: Send 8 bytes (6–9 byte category)

   ![Test 3](Docs/Images/3.png)

4. Test 4: Send 10 bytes

   ![Test 4](Docs/Images/4.png)

5. Test 5: Send 12 bytes (11–14 byte category)

   ![Test 5](Docs/Images/5.png)

6. Test 6: Send 15 bytes

   ![Test 6](Docs/Images/6.png)

7. Test 7: Send 18 bytes (16–19 byte category)

   ![Test 7](Docs/Images/7.png)

8. Test 8: Send 20 bytes

   ![Test 8](Docs/Images/8.png)

9. Test 9: Send 22 bytes (21–24 byte category)

   ![Test 9](Docs/Images/9.png)

**Conclusion:**

In Circular DMA mode, callback `Size` is the write-end position in the current
buffer cycle, not the amount of data added since the previous callback. Let the
buffer length be `UART_RX_BUFFER_SIZE`:

- HT occurs halfway through the buffer, with
  `Size = UART_RX_BUFFER_SIZE / 2U`.
- TC occurs when the complete buffer is filled, with
  `Size = UART_RX_BUFFER_SIZE`.
- IDLE occurs after transmission stops, with `Size` at the write-end position
  in the current buffer cycle. At a complete cycle, `Size` equals
  `UART_RX_BUFFER_SIZE`.
- The next DMA write position is:

```c
head = Size % UART_RX_BUFFER_SIZE;
```

New data must be calculated from the previous position; `Size` must not be
accumulated directly.

### 7. Errors and Limitations

Logging inside the interrupt callback can increase interrupt execution time.
The results apply to the current HAL version, a 10-byte buffer, and manual
single-transfer tests; other configurations require verification.

### 8. Author's Reflection

This experiment clarified when HT, TC, and IDLE occur and what callback `Size`
represents.

## License

This project is licensed under the **MIT License**.

Copyright (c) 2026 JesMicro. See [LICENSE](LICENSE) for the full text.
