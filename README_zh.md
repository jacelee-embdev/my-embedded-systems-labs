**简体中文** | [English](README.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# 嵌入式系统实验记录

## 组织与作者

- 组织：**捷思微**
- 作者：**李知秋**

## 仓库介绍

本仓库用于记录我在日常学习过程中完成的嵌入式系统实验，保存实验源码、工程配置、测试过程及分析结果，便于回顾学习过程和日后查阅。

仓库主要收录以下类型的实验：

- **验证性实验**：验证某种设计、接口或实现方式能否达到预期效果。
- **证明性实验**：通过实际测试数据验证相关原理或技术结论。
- **性能分析实验**：比较不同实现方案的运行时间、资源开销和性能差异。

每个实验使用独立目录保存，并提供相应的中英文 README。后续会根据学习和实践内容持续补充新的实验项目。

## 实验目录

| 编号 | 实验项目 | 简介 |
|---|---|---|
| LAB001 | [FreeRTOS 队列传值与传指针中断耗时对比](LAB001_INT_ResponseTime_Compare/README_zh.md) | 比较在中断中通过 FreeRTOS 队列传输结构体值和结构体指针的耗时。 |
| LAB002 | [串口中断与 DMA 接收耗时对比](LAB002_UART_DMA_Transmit_Compare/README_zh.md) | 比较 USART 中断接收与 DMA 接收对 CPU 执行时间的影响。 |
| LAB003 | [STM32F411CEU6 USART DMA FIFO 与 Burst 传输实验](LAB003_DMA_FIFO_Burst/README_zh.md) | 研究 FIFO 阈值及外设、存储器 Burst Size 对 USART DMA 数据搬运过程的影响。 |
| LAB004 | [STM32F411 TIM PWM + DMA 模拟 WT588F02B-8S 单线串口实验](LAB004_DMA_Driven_PWM/README_zh.md) | 使用 TIM2 PWM 与 DMA 生成 WT588F02B-8S 单线串口控制波形。 |

## 阅读与使用

每个实验均作为独立工程保存在对应的 `LABxxx` 目录中。使用前请先阅读该目录下的 `README_zh.md`，其中记录了实验环境、硬件连接、运行方法、测试数据和实验结论。

不同实验使用的软硬件配置和测量条件可能不同，具体信息以各实验目录内的文档为准。

> 本仓库中的测试数据和结论仅适用于对应实验所记录的硬件、软件及测量条件，不应直接视为跨平台的通用性能指标。

## 联系与交流

##### 问题反馈 · Bug 提交

- GitHub Issues：[提交问题或 Bug](https://github.com/jacelee-embdev/my-embedded-systems-labs/issues)
- Email：[jacelee.embdev@gmail.com](mailto:jacelee.embdev@gmail.com)

##### 交流学习 | MCU / FreeRTOS / Linux / 嵌入式 / 嵌入式英语

- WeChat：`JaceLee_dev`

##### 发现更多的我，查看最新动态

[Bilibili](https://space.bilibili.com/3546924034558422) · [抖音](https://v.douyin.com/mlpZXS97iU4/) · [小红书](https://www.xiaohongshu.com/user/profile/69c744e200000000340199ed)

## 许可证

本项目采用 **MIT License** 开源许可证。

Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](LICENSE) 文件。
