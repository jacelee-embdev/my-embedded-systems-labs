**简体中文** | [English](README.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)

# STM32F411 Bootloader 跳转实验

## 项目说明

##### 项目简介

本子工程是 STM32F411CEU6 的 Bootloader，负责检查指定 Flash 地址中的 APP，并在 APP 有效时完成运行环境切换和程序跳转。

##### 主要功能

- Bootloader 运行区域为 `0x08000000–0x08018FFF`。
- 通过 APP 向量表中的初始 MSP 判断 APP 是否有效。
- 释放 USART1、设置 APP 的 MSP，并跳转到 APP 的复位入口。
- 通过 USART1 输出 Bootloader 运行信息；APP 无效时留在 Bootloader 中。

##### 仓库结构

```text
01_Bootloader/
├── Core/       # 用户代码、启动流程及外设初始化
├── Drivers/    # CMSIS 与 STM32F4 HAL 驱动
├── MDK-ARM/    # Keil 工程及启动文件
└── STM32F411CEU6.ioc
```

## 工程使用指南

##### 环境与依赖

- STM32F411CEU6 开发板（25 MHz HSE）
- Keil MDK-ARM V5.32、ARM Compiler 5.06
- STM32CubeMX 6.16.1、STM32F4xx DFP 2.17.1
- 下载器；串口工具与 USB 转串口模块用于观察输出

##### 硬件连接与配置

| 信号 | STM32 引脚 | 配置 |
| --- | --- | --- |
| USART1_TX | PA9 | 连接串口模块 RX |
| USART1_RX | PA10 | 连接串口模块 TX |
| GND | GND | 与串口模块共地 |

串口参数为 115200、8 数据位、无校验、1 停止位；系统时钟为 100 MHz。

##### 编译、运行与观测

1. 使用 Keil 打开 `MDK-ARM/STM32F411CEU6.uvprojx`，编译并下载。
2. 确保配套 APP 已写入 `0x08019000`，然后复位开发板。
3. 串口先输出 `bootloader running...`；APP 有效时随后跳转到 APP。APP 无效时，Bootloader 周期性输出 `TEST 1142!`。

##### 移植说明

移植时需修改 `Core/Src/main.c` 中的 `APP_FLASH_ADDR`，并同步调整 Keil 工程的 IROM 分区。还需根据目标芯片修改时钟和 USART1 引脚配置。

## 作者思考

##### 项目关联思考

无。

##### 项目弱关联思考

无。

## 许可证

本项目采用 **MIT License** 开源许可证。

Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](../LICENSE) 文件。
