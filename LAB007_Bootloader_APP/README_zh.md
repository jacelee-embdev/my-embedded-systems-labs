**简体中文** | [English](README.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# STM32F411 Bootloader 与 APP 跳转实验

## 项目说明

##### 项目简介

本仓库包含一组基于 STM32F411CEU6 的 Bootloader 与 APP 子工程，用于演示程序分区、APP 有效性检查以及从 Bootloader 跳转到 APP 的基本流程。

##### 主要功能

- Bootloader 上电后检查位于 `0x08019000` 的 APP。
- APP 有效时，Bootloader 设置主栈指针并跳转到 APP。
- 两个子工程均通过 USART1 输出运行信息，便于观察跳转结果。

##### 仓库结构

```text
task026_Bootloader_APP/
├── 01_Bootloader/    # Bootloader 子工程，位于 Flash 起始区域
└── 02_APP/           # APP 子工程，从 0x08019000 开始运行
```

## 工程使用指南

##### 环境与依赖

- STM32F411CEU6 开发板（25 MHz HSE）
- Keil MDK-ARM V5.32、ARM Compiler 5.06
- STM32CubeMX 6.16.1、STM32F4xx DFP 2.17.1
- 下载器；串口工具与 USB 转串口模块用于观察输出

##### 硬件连接与配置

两个子工程使用相同的 USART1 配置：PA9 为 TX、PA10 为 RX，串口参数为 115200、8 数据位、无校验、1 停止位。使用 USB 转串口模块时需交叉连接 TX/RX，并与开发板共地。

Flash 分区如下：

| 区域 | 起始地址 | 大小 |
| --- | --- | --- |
| Bootloader | `0x08000000` | `0x19000` |
| APP | `0x08019000` | `0x67000` |

##### 编译、运行与观测

1. 分别打开两个子工程的 `MDK-ARM/STM32F411CEU6.uvprojx` 并编译。
2. 将 APP 和 Bootloader 下载到目标芯片，保持工程中已有的 Flash 地址配置。
3. 复位开发板并打开串口终端；先显示 Bootloader 信息，随后持续显示 APP 的 `TEST 1143!`，表示跳转成功。

##### 移植说明

移植时需同步修改 Bootloader 的 APP 起始地址、APP 的 IROM 起始地址与大小，以及 APP 的中断向量表偏移；同时重新配置目标芯片的时钟、串口和 Flash 容量。

## 作者思考

##### 项目关联思考

无。

##### 项目弱关联思考

无。

## 许可证

本项目采用 **MIT License** 开源许可证。

Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](LICENSE) 文件。
