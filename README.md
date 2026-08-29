**English** | [简体中文](README_zh.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# Embedded Systems Lab Notes

## Organization and Author

- Organization: **JesMicro**
- Author: **Jace Lee**

## About This Repository

This repository records embedded systems experiments completed during my day-to-day learning. It preserves the source code, project configurations, test procedures, and analysis results so that I can review the learning process and refer to them in the future.

The repository mainly includes the following types of experiments:

- **Validation experiments**: verify whether a design, interface, or implementation behaves as expected.
- **Demonstration experiments**: use actual test data to demonstrate relevant principles or technical conclusions.
- **Performance-analysis experiments**: compare the execution time, resource overhead, and performance of different implementations.

Each experiment is stored in a separate directory and includes both Chinese and English README files. New experiments will be added as my learning and practical work continue.

## Experiment Index

| ID | Experiment | Description |
|---|---|---|
| LAB001 | [FreeRTOS Queue-by-Value vs. Queue-by-Pointer ISR Timing Comparison](LAB001_INT_ResponseTime_Compare/README.md) | Compares the time required to transfer structure values and structure pointers through a FreeRTOS queue from an interrupt. |
| LAB002 | [UART Interrupt vs. DMA Receive-Time Comparison](LAB002_UART_DMA_Transmit_Compare/README.md) | Compares how USART interrupt-driven and DMA reception affect CPU execution time. |
| LAB003 | [STM32F411CEU6 USART DMA FIFO and Burst Transfer Experiment](LAB003_DMA_FIFO_Burst/README.md) | Examines how FIFO Threshold and Peripheral/Memory Burst Size affect USART DMA data transfers. |
| LAB004 | [STM32F411 TIM PWM + DMA Experiment for WT588F02B-8S One-Wire Serial Communication](LAB004_DMA_Driven_PWM/README.md) | Uses TIM2 PWM and DMA to generate the WT588F02B-8S one-wire serial control waveform. |

## How to Use This Repository

Each experiment is an independent project stored in its corresponding `LABxxx` directory. Before using a project, read the `README.md` in that directory for its environment, hardware connections, operating instructions, test data, and conclusions.

Software and hardware configurations and measurement conditions may differ between experiments. Refer to the documentation within each experiment directory for the applicable details.

> The test data and conclusions in this repository apply only to the hardware, software, and measurement conditions documented for the corresponding experiment. They should not be treated as general cross-platform performance metrics.

## Contact and Communication

##### Issue Feedback · Bug Reports

- GitHub Issues: [Report an issue or bug](https://github.com/jacelee-embdev/my-embedded-systems-labs/issues)
- Email: [jacelee.embdev@gmail.com](mailto:jacelee.embdev@gmail.com)

##### Learning and Discussion | MCU / FreeRTOS / Linux / Embedded Systems / English for Embedded Systems

- WeChat: `JaceLee_dev`

##### Discover More About Me and See My Latest Updates

[Bilibili](https://space.bilibili.com/3546924034558422) · [Douyin](https://v.douyin.com/mlpZXS97iU4/) · [Xiaohongshu](https://www.xiaohongshu.com/user/profile/69c744e200000000340199ed)

## License

This project is released under the **MIT License**.

Copyright (c) 2026 JesMicro. See the [LICENSE](LICENSE) file in the repository root for the complete license terms.
