**English** | [简体中文](README_zh.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)

# STM32F411 TIM PWM + DMA Experiment for WT588F02B-8S One-Wire Serial Communication

## Project Description

##### Introduction

This repository contains an STM32Cube HAL-based embedded experiment. It uses TIM2 PWM and DMA on an STM32F411CEU6 to generate the WT588F02B-8S one-wire serial control waveform on PA0, and demonstrates continuous playback of voice addresses `0x00`, `0x01`, and `0x02`.

##### Main Features

- Encodes the start level, eight data bits, and inter-byte interval as PWM compare values, with data transmitted least-significant bit first.
- Uses TIM2 update events to trigger DMA transfers from the waveform buffer to `TIM2_CCR1`; a DMA transfer-complete interrupt notifies the upper layer.
- Starts and stops PWM once per continuous-play transaction; the example repeatedly sends `F3 + 0x00/0x01/0x02`.
- Includes a ready-to-use Keil project, STM32CubeMX configuration, and protocol references.

##### Repository Structure

```text
.
├── BSP/bsp_wt588f02b_8s/  # WT588F02B-8S one-wire communication driver
├── Core/                   # Application entry point and peripheral configuration
├── Drivers/                # STM32F4 HAL and CMSIS drivers
├── MDK-ARM/                # Keil MDK-ARM project
├── References/             # Schematics, configuration-tool, and timing documents
├── STM32F411CEU6.ioc       # STM32CubeMX project configuration
├── README_zh.md            # Chinese documentation
└── README.md               # English documentation
```

## Usage Guide

##### Environment and Dependencies

Required hardware:

- STM32F411CEU6 development board (the project uses a 25 MHz HSE)
- WT588F02B-8S voice module, a suitable power supply, and compatible audio-output hardware
- SWD programmer/debugger (J-Link or ST-Link) and jumper wires

The project configuration records the following software versions:

| Software/component | Version |
| --- | --- |
| Keil MDK-ARM | V5.32 |
| Arm Compiler 5 | V5.06 update 7 (build 960) |
| STM32CubeMX (only needed to regenerate the configuration) | 6.16.1 |
| STM32CubeF4 | 1.28.3 (HAL 1.8.5) |
| Keil STM32F4xx DFP | 2.17.1 |

Optional tools: a logic analyzer for checking the PA0 waveform, and a USB-to-UART adapter for viewing debug output.

##### Hardware Connection and Configuration

| STM32F411CEU6 | External connection | Purpose |
| --- | --- | --- |
| PA0 / TIM2_CH1 | WT588F02B-8S one-wire data input | PWM control signal |
| GND | Voice-module GND | A common ground is required |
| PA9 / USART1_TX | USB-to-UART RX (optional) | 115200, 8-N-1 |

Power the voice module according to its specifications and connect a compatible speaker or amplifier. Before powering on, make sure the module is configured for one-wire serial mode and that valid audio has been programmed at addresses `0x00` through `0x02`. Connect the SWD programmer through the development board's standard SWD interface.

Key peripheral settings:

| Item | Configuration |
| --- | --- |
| System clock / TIM2 clock | 100 MHz / 100 MHz |
| TIM2 | Prescaler 99, auto-reload value 799, period 800 μs |
| Compare values | `0`, `200`, `600`, and `800` for low, 25%, 75%, and high levels |
| DMA | DMA1 Stream1 / Channel 3, memory-to-peripheral, Normal mode, 32-bit width |
| DMA trigger and destination | TIM2 Update → `TIM2_CCR1` |

##### Build, Run, and Observe

1. Complete the wiring above and open `MDK-ARM/STM32F411CEU6.uvprojx`.
2. Select the `STM32F411CEU6` target in Keil, build the project, and flash it to the board through SWD.
3. Reset the board. After allowing the voice module to finish powering up, the firmware sends the `F3 + 0x00/0x01/0x02` continuous-play sequence. It waits approximately five seconds after each sequence before sending it again.
4. Confirm through the speaker that the three recordings play in address order. Optionally, use the UART to observe `TEST 1142!` after each sequence, or use a logic analyzer to inspect the start level, data bits, and inter-byte interval on PA0.

##### Porting Notes

- After changing the clock or timer, recalculate the prescaler, auto-reload value, and compare values to preserve the current 1 μs counter resolution and 800 μs waveform period, or redesign the timing for the target protocol.
- When changing the pin or timer channel, update the GPIO alternate function, `TIM_CHANNEL_x`, destination `CCRx`, DMA request, Stream, Channel, IRQ, and handle linkage together.
- The waveform buffers use `uint32_t`; the DMA memory and peripheral widths must remain consistent. If commands are extended, check both buffer length and SRAM usage.
- Playback addresses and the waiting flow are currently defined in `BSP/bsp_wt588f02b_8s/bsp_wt588f02b_8s.c`. For an RTOS or a non-blocking design, replace the busy-wait loops with task notifications or an event mechanism.
- This repository has only been verified with STM32F411CEU6, TIM2_CH1/PA0, and DMA1 Stream1/Channel 3. Other platforms require waveform and electrical validation.

## Author's Notes

##### Project-Related Reflections

###### Background

> When working with voice modules such as the WT588F02B or single-wire communication devices such as the DHT11, communication is commonly implemented in one of two ways:
>
> 1. Using GPIO plus Delay calls to emulate the required timing.
> 2. Using timer PWM to generate the required square wave.
>
> Both methods have drawbacks. Repeated delays in the GPIO-plus-Delay approach block execution and reduce the system's real-time responsiveness. With conventional timer PWM, the CPU must continually update the timer CCR to generate square waves with different duty cycles, so CPU usage remains high.
>
> This leads to a third approach: timer PWM plus DMA.
>
> DMA transfers the required CCR values from SRAM to the timer CCR. There is no need to enable a timer interrupt; only the timer's DMA function needs to be configured:
>
> - A timer update event generates a DMA request.
> - DMA transfers values from a buffer in SRAM to the timer CCR.
> - The timer generates square waves with different duty cycles from those values.
>
> After one complete data transmission, DMA raises a transfer-complete interrupt. Its completion callback then updates the transfer-complete event.
>
> This approach avoids the blocking caused by GPIO plus Delay and the high CPU overhead of generating the waveform with conventional timer PWM, although it does not itself exercise the CPU's ALU, FPU, or other computing resources.
>
> It frees the CPU and improves the system's real-time responsiveness.

###### Important Bug Record

> **Symptoms:** The playback function was initially called immediately after WT588 initialization. Playback often failed, and the board had to be reset repeatedly; occasionally it worked only once.
>
> After moving the playback function into the `while` loop with a two-second delay, playback became random: tracks repeated, played out of order, and used inconsistent intervals.
>
> After changing the delay to five seconds, the module would occasionally play only one recording.

> **Investigation:**
>
> 1. I checked the manual and confirmed that the playback commands `F3 00`, `F3 01`, and `F3 02` were correct. The hardware was also sound, since the hardware demo could play normally.
>
> 2. I used a logic analyzer to inspect communication between the MCU and the driver board. The start level was correct, and neither the timer nor the DMA transfer was at fault. *However, after each byte, the high level lasted only 12–14 microseconds—far short of the 5–10 milliseconds required by the reference protocol.*
>
> 3. Code inspection showed that after DMA finished sending each byte, execution entered the callback and called `HAL_TIM_PWM_Stop`.
>
>    Once the timer stopped, the following `HAL_Delay(2); HAL_Delay(5);` calls could no longer maintain the output level as intended.

> **Solution:**
>
> 1. Improve timer start/stop control:
>
>    I stopped shutting down the timer in the DMA callback after every byte and instead controlled it around one complete continuous-play transaction.
>
>    For example, the timer starts before sending `F3 00`, `F3 01`, and `F3 02`, then stops only after the entire transmission finishes. This avoids timing errors caused by frequent starts and stops while also reducing idle power consumption.
>
> 2. Simplify the DMA callback:
>
>    After a DMA transfer completes, the callback only clears the BZ flag to notify the upper layer that the current transmission is complete. It no longer stops the timer.
>
>    The upper-level flow stops the timer only after confirming that the complete timing sequence has finished.
>
> 3. Put the complete communication timing into the DMA buffer:
>
>    All protocol-required levels are encoded directly into the DMA array, including:
>
>    - The initial low level
>    - The waveform for the eight data bits supplied by the argument
>    - The high-level interval between bytes
>    - The high level after the final byte
>
>    The `HAL_Delay()` calls between bytes are then removed, allowing the timer and DMA to generate the complete timing sequence uniformly and preventing level errors caused by software delays or timer restarts.

##### Other Reflections

The DMA-plus-timer approach can be extended to software emulation of other communication protocols in future projects.

## License

This project is licensed under the **MIT License**.

Copyright (c) 2026 JesMicro. See the [LICENSE](../LICENSE) file in the repository root for the full license text.
