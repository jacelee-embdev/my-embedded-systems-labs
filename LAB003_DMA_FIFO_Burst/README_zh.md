**简体中文** | [English](README.md)

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](../LICENSE)

# STM32F411CEU6 USART DMA FIFO 与 Burst 传输实验

## 项目说明

##### 项目简介

本项目基于 STM32F411CEU6 和 HAL 库，通过 USART1 DMA 接收观察 FIFO Threshold 与 Burst Size 对数据搬运过程的影响。

##### 主要功能

- USART1 DMA 数据接收
- DMA FIFO Threshold 对比
- Peripheral 与 Memory Burst Size 对比
- 通过 MDK Memory Window 观察接收缓冲区

##### 仓库结构

```text
Core/                  核心源代码与头文件
Drivers/               STM32 HAL 与 CMSIS 驱动
MDK-ARM/               Keil MDK 工程
Docs/Images/            实验截图
STM32F411CEU6.ioc       STM32CubeMX 配置文件
README_zh.md            中文说明
```

## 工程使用指南

##### 环境与依赖

- STM32F411CEU6 开发板
- 3.3 V USB 转串口模块与 SWD 调试器
- Keil MDK-ARM、STM32CubeMX

##### 硬件连接与配置

| USB 转串口 | STM32F411CEU6 |
| ---------- | ------------- |
| TX         | PA10（USART1_RX） |
| RX         | PA9（USART1_TX）  |
| GND        | GND               |

串口参数：115200、8 数据位、1 停止位、无校验、无流控。

##### 编译、运行与观测

1. 使用 Keil 打开 `MDK-ARM/STM32F411CEU6.uvprojx`，编译并下载程序。
2. 全速运行程序，通过串口助手发送十六进制数据。
3. 暂停程序，在 Memory Window 中观察 `recv_data_buf`。

##### 移植说明

移植时需重新确认 USART 引脚、DMA Stream/Channel、系统时钟及串口参数。不同芯片的 FIFO 容量和 Burst 组合可能不同，应以对应参考手册为准。

## 实验报告

##### 1. 实验背景与原理

1. 探究清楚 DMA 的 FIFO 尺寸大小?
2. 探究清楚不同 Burst Size 的 影响?

##### 2. 实验假设

无

##### 3. 实验平台与配置

- STM32F411CEU6
- MDK

##### 4. 变量与控制条件

###### 子实验一:测量FIFO 尺寸

- 自变量：FIFO阈值
- 因变量：串口接收缓冲区 单次更新的大小.
- 控制变量：其它所有因素。

###### 子实验二:Burst Size

- 自变量1：Peripheral 的 Burst size
- 自变量2：Memory 的 Burst size
- 因变量1：搬完16字节FIFO所需的AHB传输组数
- 因变量2: 同一 Peripheral Register 的连续读取次数
- 控制变量：其它所有因素。

##### 5.实验步骤

无

##### 6. 实验数据、分析与结论

###### 子实验一:测量FIFO 尺寸

1. 半满与全满 

   <img src="Docs/Images/hallfull.png" alt="hallfull" style="zoom:10%;" /><img src="Docs/Images/full.png" alt="full" style="zoom:10%;" />

| FIFO阈值  | 串口接收缓冲区 单次更新的大小 |
| --------- | ----------------------------- |
| Half Full | 8                             |
| Full      | 16                            |

测试得出结论:DMA FIFO 大小16字节.

###### 子实验二:Burst Size

1. Memory Burst Size 对 FIFO 数据的搬运方式

​	控制条件：FIFO阈值为全满，外设突发为单次传输，外设与存储器的数据宽度均为1字节。

| 存储器突发模式 | 每个AHB传输组的数据量 | 搬完16字节FIFO所需的AHB传输组数 |
| -------------- | --------------------- | ------------------------------- |
| 单次传输       | 1字节                 | 16组                            |
| 4次增量传输    | 4字节                 | 4组                             |
| 8次增量传输    | 8字节                 | 2组                             |
| 16次增量传输   | 16字节                | 1组                             |



2. Peripheral Register 的连续读取次数

​	控制条件：FIFO阈值为全满，存储器突发为单次传输，外设与存储器的数据宽度均为1字节，外设地址禁止递增。

| 外设突发模式 | 同一 Peripheral Register 的连续读取次数 |
| :----------- | :-------------------------------------: |
| 单次传输     |                   1次                   |
| 4次增量传输  |                   4次                   |
| 8次增量传输  |                   8次                   |
| 16次增量传输 |                  16次                   |

4次增量传输,如图:<img src="Docs/Images/Burst_Size_4 _Increment.png" alt="Burst_Size_4 _Increment" style="zoom:10%;" />



##### 7. 误差与局限性

无,不涉及精度.本实验无法直接观察瞬时 AHB Burst 过程；

##### 8. 作者思考

1. 五个配置项

![5elements](Docs/Images/5elements.png)

| Element               | 作用                                 |
| --------------------- | ------------------------------------ |
| FIFO Threshold        | 决定FIFO积累多少数据后开始写入Memory |
| Peripheral Data Width | 决定每次从Peripheral读取多少字节     |
| Memory Data Width     | 决定每次向Memory写入多少字节         |
| Peripheral Burst Size | 决定一次连续读取Peripheral多少次     |
| Memory Burst Size     | 决定一次连续写入Memory多少次         |

五个配置,我们可以建立一个“生产端—仓库—消费端”模型：

**Peripheral（SRC）→ FIFO（仓库）→ Memory（DST）**

    1. **FIFO Threshold**：决定仓库中的数据积累到多少时，开始向消费端交付。仓库总容量不变，改变的是触发交付的水位。
    2. **Peripheral Data Width**：决定每次从生产端 `SRC` 读取多大规格的数据，例如1、2或4字节。
    3. **Peripheral Burst Size**：决定每轮连续从生产端 `SRC` 读取多少次数据。
    4. **Memory Data Width**：决定每次向消费端 `DST` 写入多大规格的数据，例如1、2或4字节。
    5. **Memory Burst Size**：决定每轮连续向消费端 `DST` 写入多少次数据。


2. Direct Mode 与 FIFO Mode 下的 FIFO 使用情况
   - 需要强调的是，未启用 FIFO 时，DMA 使能后仍会通过内部的缓冲区预加载一个字节数据。这是为了当 request 到来的时候立即传输，提高响应。<img src="Docs/Images/Direct mode.png" alt="Direct mode" style="zoom:25%;" />
   - 而启用 FIFO 时，DMA 会预先装载，装满整个 FIFO 16 个字节。请求到来的时候，会一次性全部传输，效率接近 CPU 独立操作。如果说 RAM SRAM 的数据不足（例如仅 13 字节），仍会尝试传输 16 字节，这可能会导致异常。<img src="Docs/Images/Fifo mode .png" alt="Fifo mode " style="zoom:25%;" />



## 许可证

本项目采用 **MIT License** 开源许可证。

Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](../LICENSE) 文件。
