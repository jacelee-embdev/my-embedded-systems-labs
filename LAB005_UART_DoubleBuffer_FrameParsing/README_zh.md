**简体中文** | [English](README.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

# STM32F411 UART 双缓冲与数据帧解析实验

## 项目说明

##### 项目简介

本工程基于 STM32F411CEU6 和 FreeRTOS，实现 UART 中断接收、双环形缓冲及数据帧解析。

##### 主要功能

- 双缓冲交替接收并通过队列通知解析线程。
- 状态机完成帧头、长度、载荷、校验和帧尾解析。
- 有效载荷通过队列交给应用线程。

##### 仓库结构

```text
App/                                  数据帧解析与应用线程
BSP/                                  UART 双缓冲接收
Middlewares_User/circular_buffer/     环形缓冲区
Core/                                 STM32 初始化代码
MDK-ARM/STM32F411CEU6.uvprojx         Keil 工程
```

## 工程使用指南

##### 环境与依赖

- STM32F411CEU6，100 MHz
- Keil MDK 5，ARM Compiler 5.06
- FreeRTOS / CMSIS-RTOS2
- USB 转串口、J-Link 与 RTT Viewer

##### 硬件连接与配置

| USB 转串口 | STM32F411 |
|---|---|
| TX | PA10 / USART1_RX |
| GND | GND |

串口配置：`115200-8-N-1`，无硬件流控。

数据帧格式：

```text
FF FE | LEN | DATA[LEN] | CHECK | AA
```

`LEN` 范围为 1～100；`CHECK` 为 `LEN + DATA` 的低 8 位。

当前单次接收 9 字节，每个环形缓冲区为 108 字节。

##### 编译、运行与观测

1. 使用 Keil 打开 `MDK-ARM/STM32F411CEU6.uvprojx`，编译并下载。
2. 通过串口发送测试帧：`FF FE 04 01 02 03 04 0E AA`。
3. 在 RTT Viewer 中查看解析结果。

##### 移植说明

- 在 `Core/Src/usart.c` 中修改串口及引脚配置。
- 在 `BSP/bsp_uart_driver.c` 中修改单次接收长度。
- 缓冲区大小必须是单次接收长度的整数倍。
- 在 `App/app.c` 中修改协议字段及校验规则。

## 作者思考

##### 项目关联思考

- 需求分析与业务流向

![01_串口私有协议：显示双缓冲+本地解包_需求分析与业务流向](Docs/Images/01_串口私有协议：显示双缓冲+本地解包_需求分析与业务流向.png)

源文件: [01_串口私有协议：显示双缓冲+本地解包_需求分析与业务流向.drawio](Docs/01_串口私有协议：显示双缓冲+本地解包_需求分析与业务流向.drawio)



- 软件资源架构

![02_串口私有协议：显示双缓冲+本地解包_软件资源架构](Docs/Images/02_串口私有协议：显示双缓冲+本地解包_软件资源架构.png)

源文件: [02_串口私有协议：显示双缓冲+本地解包_软件资源架构.drawio](Docs/02_串口私有协议：显示双缓冲+本地解包_软件资源架构.drawio)



- 程序分析

![03_串口私有协议：显示双缓冲+本地解包_程序分析](Docs/Images/03_串口私有协议：显示双缓冲+本地解包_程序分析.png)

源文件: [03_串口私有协议：显示双缓冲+本地解包_程序分析.drawio](Docs/03_串口私有协议：显示双缓冲+本地解包_程序分析.drawio)

##### 项目弱关联思考

无

## 许可证

本项目采用 **MIT License** 开源许可证。

Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](LICENSE) 文件。
