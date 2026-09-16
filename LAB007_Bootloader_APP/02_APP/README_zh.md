**简体中文** | [English](README.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)

# STM32F411 Bootloader APP 实验

## 项目说明

##### 项目简介

本子工程是与 Bootloader 配套的 STM32F411CEU6 APP。程序链接到 `0x08019000`，被 Bootloader 跳转执行后，通过串口输出信息验证 APP 已正常运行。

##### 主要功能

- APP 运行区域为 `0x08019000–0x0807FFFF`。
- 将中断向量表重定位到 `0x08019000`，并重新开启全局中断。
- 初始化系统时钟和 USART1。
- 每 2 秒通过 USART1 输出一次 `TEST 1143!`。

##### 仓库结构

```text
02_APP/
├── Core/       # APP 主程序及外设初始化
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

1. 使用 Keil 打开 `MDK-ARM/STM32F411CEU6.uvprojx`，确认 IROM 起始地址为 `0x08019000`、大小为 `0x67000`。
2. 编译并将 APP 下载到目标芯片，再写入配套 Bootloader。
3. 复位后观察串口；Bootloader 跳转成功后，终端每 2 秒输出一次 `TEST 1143!`。

##### 移植说明

移植时需保持 APP 的 IROM 起始地址与 `Core/Src/main.c` 中的 `SCB->VTOR` 偏移一致，并与 Bootloader 中配置的 APP 起始地址对应。同时按目标芯片调整时钟、串口和可用 Flash 大小。

## 作者思考

##### 项目关联思考

无。

##### 项目弱关联思考

无。

## 许可证

本项目采用 **MIT License** 开源许可证。

Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](../LICENSE) 文件。
