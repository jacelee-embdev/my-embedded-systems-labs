**简体中文** | [English](README.md)

# STM32F411 加密固件升级工程

基于 **STM32F411CEU6** 的加密固件升级示例，完整展示固件接收、外部存储暂存、AES 解密、旧版本备份、内部 Flash 安装、首次启动确认与失败恢复。Bootloader 和 APP 工程均已完成优化、重构与代码规范化，当前版本为 **v1.0**。

工程包含一个基于 SPL 的裸机 Bootloader，以及两个基于 HAL / FreeRTOS 的应用。首次安装可由 Bootloader 接收固件；后续升级可由正在运行的 APP 接收，再复位交给 Bootloader 安装。APP1 和 APP2 共用同一应用运行区，通过不同的日志和 LED 翻转间隔展示版本切换。

| 子工程 | 技术栈 | 职责 |
| --- | --- | --- |
| [Bootloader](01_Bootloader/README_zh.md) | SPL、裸机 | 下载、解密、备份、安装、启动与失败恢复 |
| [APP1](02_APP1/README_zh.md) | HAL、FreeRTOS | 初始演示固件，运行期间接收下一份加密包 |
| [APP2](02_APP2/README_zh.md) | HAL、FreeRTOS | 升级演示固件，也可继续接收兼容固件 |

本文按需求、硬件、存储、工程结构、软件架构和运行流程介绍系统。11 张设计图位于 [05_Docs/Images](05_Docs/Images/)，可点击图片查看原图；各子工程的入口、配置和实现细节见上表链接。

## 目录

- [需求与方案](#需求与方案)
- [硬件架构与接线](#硬件架构与接线)
- [存储分布](#存储分布)
- [工程目录与工程树](#工程目录与工程树)
- [软件架构](#软件架构)
- [固件升级流程](#固件升级流程)
- [升级状态机](#升级状态机)
- [串口命令与 YMODEM 传输](#串口命令与-ymodem-传输)
- [AES 加密包与解密流程](#aes-加密包与解密流程)
- [编译与快速演示](#编译与快速演示)
- [许可证](#许可证)

## 需求与方案

系统围绕五项需求展开：支持首次安装与持续升级、检查接收过程、处理加密固件、跨复位管理状态，以及在具有有效备份时恢复旧应用。

对应方案是使用 USART1 / YMODEM 接收固件，通过 W25Q64 的 A / B 两区暂存密文、明文与旧备份，使用 AT24C02 保存升级状态和长度，由 Bootloader 统一完成解密与安装，再配合 IWDG 和 APP 确认标志判断首次启动结果。RTT、EasyLogger 和 LED 提供运行反馈。

![DSN-01：需求分析与方案制定](05_Docs/Images/DSN-01%20需求分析与方案制定.png)

图中逐项对应需求与实现方案，给出了后续硬件、存储、协议和状态机设计的依据。

## 硬件架构与接线

Bootloader、APP1 和 APP2 共用同一套硬件。W25Q64 负责固件暂存与备份，AT24C02 保存升级配置，PA0 按键用于进入下载或确认安装，PC13 LED 用于反馈运行状态。

![DSN-02：硬件架构设计](05_Docs/Images/DSN-02%20硬件架构设计.png)

| 模块 / 接口 | MCU 引脚与配置 | 用途 |
| --- | --- | --- |
| USART1 | PA9：TX；PA10：RX；115200、8N1 | 串口命令和 YMODEM 固件传输 |
| W25Q64 / SPI1 | PA4：CS；PA5：SCK；PA6：MISO；PA7：MOSI | 外部 SPI Flash |
| AT24C02 / 软件 I²C | PB8：SCL；PB9：SDA | 状态、长度和旧 APP 备份记录 |
| 按键 | PA0，低电平按下 | Bootloader 下载入口、APP 安装确认 |
| LED | PC13，低电平点亮 | 运行状态与演示版本识别 |
| J-Link / SWD | PA13：SWDIO；PA14：SWCLK | 烧录、调试和 RTT 日志 |

使用 3.3 V 逻辑电平并共地，MCU 的 TX 连接模块 RX，MCU 的 RX 连接模块 TX。图中的 CH340 有线串口和 HC-05 蓝牙透明串口是 USART1 的两种替代接入方式，使用时只接入一路，并保证串口参数一致。

## 存储分布

内部 Flash 分开存放 Bootloader 和正在运行的 APP；外部 Flash 的 A / B 两区用于升级过程；EEPROM 保存跨复位需要保留的配置与备份记录。

![DSN-03：存储分布设计](05_Docs/Images/DSN-03%20存储分布设计.png)

| 存储位置 | 地址范围 | 容量 | 用途 |
| --- | --- | --- | --- |
| 内部 Flash | `0x08000000–0x08007FFF` | 32 KiB | Bootloader |
| 内部 Flash | `0x08008000–0x0801FFFF` | 96 KiB | APP1 / APP2 共用的运行区 |
| 内部 Flash | `0x08020000–0x0807FFFF` | 384 KiB | 当前工程未使用 |
| W25Q A 区 | `0x000000–0x00FFFF` | 64 KiB | 先保存新加密包，解密后复用为旧 APP 备份 |
| W25Q B 区 | `0x010000–0x01FFFF` | 64 KiB | 新 APP 的有效明文 |
| W25Q 剩余空间 | `0x020000–0x7FFFFF` | 8064 KiB | 当前工程未分配 |

A / B 是外部存储的处理分区。升级时，Bootloader 将 B 区新固件安装到唯一的内部 APP 运行区；APP1 与 APP2 在该运行区相互替换。回滚来源是 A 区中经过验证的旧 APP 备份。

| EEPROM 地址 | 内容 |
| --- | --- |
| `0x00` | 升级状态 |
| `0x01–0x04` | 完整加密包长度，32 位小端整数 |
| `0x05–0x08` | 内部已安装 APP 的有效明文长度，32 位小端整数 |
| `0x10–0x1F` | 旧 APP 备份记录：有效标志、格式版本、保留字段、长度、明文 CRC 与记录 CRC |

Bootloader 的[散装文件](01_Bootloader/06_MDK-ARM/bootloader_noinit.sct)还保留了 `0x2001FFF0–0x2001FFFF` 共 16 字节 SRAM，用于软件复位后的提前跳转请求。

虽然内部 APP 运行区为 96 KiB，当前**完整加密包最大为 64 KiB，APP 有效明文最大为 65520 字节**，受外部暂存分区与包头开销共同限制。

## 工程目录与工程树

```text
.
├── 01_Bootloader/          SPL 裸机 Bootloader
├── 02_APP1/                HAL / FreeRTOS 应用示例一
├── 02_APP2/                HAL / FreeRTOS 应用示例二
├── 04_BIN_EncryptionBIN/   示例 BIN 与 .mxxx 加密包
├── 05_Docs/
│   └── Images/             需求、架构、流程与算法设计图
├── README_zh.md            中文总说明
└── README.md               英文总说明
```

![DSN-04：工程文件结构与工程树设计](05_Docs/Images/DSN-04%20工程文件结构与工程树设计.png)

图中以 Bootloader 和 APP1 为例，同时展示磁盘目录与 Keil 工程树。Bootloader 按驱动、核心、BSP、中间件和业务分层；APP 保留 CubeMX 管理的 `Core/`、`Drivers/` 和 `Middlewares/`，将用户驱动、中间件和业务分别放入 `BSP/`、`Middlewares_User/` 和 `Business/`。APP2 沿用相同结构。

Keil 文件分组与磁盘目录分别管理。重新生成 CubeMX 代码后，需要检查用户源文件引用、头文件搜索路径和工程分组；生成文件中的用户修改保留在 USER CODE 区域。

## 软件架构

Bootloader 和 APP 分阶段运行，共用硬件接线、外部存储布局与 EEPROM 协议。两者分别使用 SPL 与 HAL / FreeRTOS 完成底层适配，通过复位与应用跳转交接控制权。

![DSN-05：软件架构设计](05_Docs/Images/DSN-05%20软件架构设计.png)

| 层次 | Bootloader | APP |
| --- | --- | --- |
| 芯片基础库 | CMSIS、SPL | CMSIS、HAL |
| 外设与系统支持 | 启动、中断、USART1、SPI1、内部 Flash、IWDG、延时 | 启动、中断、GPIO、USART1 / DMA、SPI1、时基 |
| BSP | 按键、LED、AT24Cxx、W25Q 分区管理 | 按键、LED、AT24Cxx、UART 接收适配、W25Q 分区管理 |
| 中间件 | YMODEM、AES-256-CBC、CRC、SFUD、EasyLogger / RTT | YMODEM、CRC、SFUD、FreeRTOS、EasyLogger / RTT |
| 业务 | 状态分流、解密、备份、安装、恢复和启动 | 任务创建、固件接收、写入协作、安装确认和运行状态 |

Bootloader 入口为 [main.c](01_Bootloader/02_Core/Entry/main.c)，升级核心为 [bootloader.c](01_Bootloader/05_Business/Bootloader/bootloader.c)。APP 的 [main.c](02_APP1/Core/Src/main.c) 完成初始化与首次启动确认，[os_init.c](02_APP1/Business/Init/os_init.c) 创建任务，`fw_update.c` 协调下载与写入，[app.c](02_APP1/Business/Status/app.c) 输出状态日志并翻转 LED。

APP 的接收路径使用 USART1 DMA、双缓冲、长度通知队列、数据指针队列和信号量。接收任务与写入任务按包协作，在确认处理完成后复用缓冲区；结束或失败时先等待写入任务确认停止，再释放资源。

## 固件升级流程

### 整体流程

首次安装与应用内升级共用 Bootloader 的解密、备份、安装和首次启动确认流程。区别在于加密包由 Bootloader 直接接收，还是由正在运行的 APP 提前接收并提交。

![DSN-06-01：固件更新整体流程](05_Docs/Images/DSN-06-01%20流程设计_固件更新整体流程.png)

1. 上位机编译 APP，将 BIN 按约定格式加密为 `.mxxx`。
2. Bootloader 或 APP 通过 YMODEM 将加密包保存到 W25Q A 区。
3. Bootloader 将 A 区密文解密到 B 区，检查长度和应用启动入口。
4. 有效旧 APP 备份到 A 区，并提交长度、CRC 等备份记录。
5. 将 B 区新 APP 写入内部 Flash，回读比较，保存新长度与待试运行状态。
6. 软件复位后进入首次试运行；APP 写回确认标志，等待 IWDG 复位，再正常启动。
7. 首次确认失败时，校验并尝试恢复 A 区旧备份；没有有效备份或恢复失败时等待重新下载。

### Bootloader 侧流程

Bootloader 根据 EEPROM 状态选择正常启动、使用已下载包安装、首次试运行、恢复旧备份或等待下载。正常启动通过保留 RAM 请求和软件复位，在下一次进入启动入口时提前跳转 APP。

![DSN-06-02：Bootloader 侧流程](05_Docs/Images/DSN-06-02%20流程设计_Bootloader侧流程.png)

解密成功后才允许旧备份覆盖 A 区密文。旧 APP 的备份记录与完整明文 CRC 校验通过后，才用于恢复；安装和恢复均检查写入结果。首次试运行前，Bootloader 确认写入 `0x44` 并启动名义超时约 6 秒的 IWDG，APP 未确认时复位仍保留失败状态。

### APP 侧流程

APP 在正常业务运行期间等待下载命令，接收加密包并协调写入 A 区；数据刷新、长度与状态保存确认后，应答下载完成。收到安装命令并检测到按键确认后，软件复位，由 Bootloader 接管。

![DSN-06-03：APP 侧流程](05_Docs/Images/DSN-06-03%20流程设计_APP侧流程.png)

按键确认窗口约为 20 秒，超时后返回下载命令等待，已提交的 `0x22` 状态保留，后续复位仍可触发安装。首次启动确认发生在 `main.c` 完成外设和 BSP 初始化之后、RTOS 与业务任务启动之前；APP 尝试写入 `0x00` 后等待看门狗复位，由 Bootloader 读取实际保存结果。

首次安装没有旧版本可恢复。当前确认机制用于判断是否到达上述启动确认点，确认之后的业务运行故障不在本次回滚判定范围内。

## 升级状态机

系统包含六种跨复位保存的 EEPROM 状态，以及四种仅在 APP 固件更新线程中使用的 RAM 状态。复位后，Bootloader 依据 EEPROM 状态继续处理；APP 线程状态在重新启动时初始化。

![DSN-07：升级状态机设计](05_Docs/Images/DSN-07%20OTA%20状态机设计.png)

| EEPROM 状态 | 含义 | Bootloader 主要动作 |
| --- | --- | --- |
| `0x00` | 正常启动 | 启动有效 APP，或按键进入下载 |
| `0x11` | 下载未完成 | 优先尝试启动当前有效 APP，否则重新下载 |
| `0x22` | 加密包已完整提交 | 恢复包长度，解密、备份并安装 |
| `0x33` | 安装完成，等待首次试运行 | 写入并确认 `0x44`，启动 IWDG，跳转新 APP |
| `0x44` | 升级失败或首次启动未确认 | 校验并尝试恢复旧 APP |
| `0x55` | 必须重新下载 | 禁止启动当前 APP，等待按键下载 |

APP 线程依次经历“等待下载命令 → 接收并完成提交 → 等待安装命令 → 等待按键确认”。`0x22` 在下载提交时已经保存，安装命令与按键用于决定软件复位的时机。线程等待状态与 EEPROM 状态分别管理。

## 串口命令与 YMODEM 传输

### 应用层控制命令

以下均为**十六进制原始字节**，使用串口工具的 HEX 模式发送。

| 方向 | 字节序列 | 含义 |
| --- | --- | --- |
| 上位机 → APP | `11 22 33` | 请求接收下一份加密包 |
| APP → 上位机 | `44 55 66` | 下载、写入刷新及元数据提交已完成，准备接收安装命令 |
| 上位机 → APP | `77 88 99` | 请求安装，随后等待 PA0 按键确认 |

Bootloader 直接下载由 PA0 触发；上述三字节命令用于运行中的 APP。

### YMODEM 单文件传输

![DSN-08：YMODEM 单文件典型传输流程](05_Docs/Images/DSN-08%20Ymodem单文件典型传输流程分析.png)

接收端发送 `C` 请求 CRC 模式，上位机发送带文件名与长度的 0 号首包，再逐包传输数据。本实现通过 EOT、ACK、`C` 和空文件名尾包结束一次单文件会话。接收端检查序号、序号反码、CRC-16/XMODEM 与声明长度，上一包的有效重传不会重复写入。

Bootloader 采用轮询接收并同步处理存储；APP 采用 DMA / IDLE 或传输完成通知、双缓冲和写入任务协作。YMODEM 的最终 ACK 表示协议会话结束，APP 随后还需完成尾部刷新与元数据提交，因此上位机应等待 `44 55 66` 再发安装命令。

分包 CRC 用于传输校验；旧 APP 备份及其记录另用 CRC-32/ISO-HDLC 校验。接收流程只保存文件的有效字节，YMODEM 尾包填充不计入固件长度。

## AES 加密包与解密流程

上位机生成 AES-256-CBC 加密包，Bootloader 使用同一密钥和初始 IV 解密。APP 负责接收和保存加密包，解密与新 APP 安装由 Bootloader 完成。

![DSN-09：AES 算法与固件解密流程](05_Docs/Images/DSN-09%20AES加密算法流程展示.png)

图左侧展示 AES-256 的密钥扩展、加密与解密轮次，右侧对应工程中的 A 区密文到 B 区明文处理流程。

| 项目 | 约定 |
| --- | --- |
| 算法 | AES-256-CBC；32 字节密钥，16 字节分组和 IV |
| 包内容 | 16 字节文件头与按 16 字节对齐的 APP 数据一起加密 |
| APP 长度 | 解密后文件头的第 12–15 字节，32 位小端整数 |
| CBC 链 | 每份新包初始化一次工作 IV，头部与后续数据块连续处理 |
| 包长度 | `16 + ceil(APP有效长度 / 16) × 16`，总长不超过 65536 字节 |
| 明文保存 | 按头部记录的有效长度写入 B 区，丢弃末尾对齐填充 |
| 安装前检查 | 包长、分组对齐、明文长度关系、写入长度与 APP 启动入口 |

密钥和初始 IV 定义在 [bootloader.c](01_Bootloader/05_Business/Bootloader/bootloader.c) 的 `g_app_aes_key` 与 `g_app_aes_initial_iv` 中。自行生成加密包时，加密工具必须与这些配置和包格式一致；当前实现按有效长度截取明文。

## 编译与快速演示

### 准备环境

- Keil MDK 5，工程配置使用 ARMCC 5.06 update 7。
- J-Link，用于 SWD 烧录和 RTT 日志查看。
- 支持 YMODEM 单文件发送和 HEX 命令的串口工具，参数为 115200、8N1。
- 与 Bootloader 的 AES 配置及包格式兼容的加密工具；仓库提供示例加密包，加密工具需另行准备。

| 工程 | Keil 工程文件 | 地址与产物 |
| --- | --- | --- |
| Bootloader | [STM32F411CEU6_SPL.uvprojx](01_Bootloader/06_MDK-ARM/STM32F411CEU6_SPL.uvprojx) | 起始地址 `0x08000000`；产物在 `06_MDK-ARM/Objects/` |
| APP1 | [STM32F411CEU6.uvprojx](02_APP1/MDK-ARM/STM32F411CEU6.uvprojx) | 链接地址 `0x08008000`；BIN 为 `MDK-ARM/Objects/STM32F411CEU6.bin` |
| APP2 | [STM32F411CEU6.uvprojx](02_APP2/MDK-ARM/STM32F411CEU6.uvprojx) | 链接地址 `0x08008000`；BIN 为 `MDK-ARM/Objects/STM32F411CEU6.bin` |

编译时保留各工程现有链接配置，Bootloader 使用自带的散装文件。各工程的 `Listings/` 保存列表和映射文件。APP1 与 APP2 的 BIN 文件名相同，分别位于各自的工程目录，加密时需要区分来源。

### 首次安装 APP1

1. 按硬件接线表连接设备，使用 J-Link 编译并烧录 Bootloader。
2. 编译 APP1，将生成的 BIN 加密为 `.mxxx`；也可使用 [APP1 示例包](04_BIN_EncryptionBIN/APP1_EncryptionBIN.mxxx)演示。
3. 复位后按 PA0 进入 Bootloader 下载，在串口工具中使用 YMODEM 发送 APP1 加密包。
4. 在 RTT 中观察接收、解密与安装日志。新 APP 首次试运行确认后会等待 IWDG 复位，随后正常运行。
5. 确认出现 `APP1 running...`，LED 每 500 ms 翻转。

### 从 APP1 升级到 APP2

1. 编译并加密 APP2，或使用 [APP2 示例包](04_BIN_EncryptionBIN/APP2_EncryptionBIN.mxxx)。
2. APP1 正常运行时，发送 HEX 命令 `11 22 33`。
3. 使用 YMODEM 发送 APP2 加密包，等待协议传输结束及应用应答 `44 55 66`。
4. 发送 HEX 命令 `77 88 99`，并在约 20 秒的确认窗口内按 PA0。
5. 设备软件复位，由 Bootloader 完成解密、旧 APP 备份、新 APP 安装与首次启动确认。
6. 确认出现 `APP2 running...`，LED 每 200 ms 翻转。APP2 可继续以相同方式接收兼容固件。

| 演示固件 | 运行日志 | LED 翻转间隔 |
| --- | --- | --- |
| APP1 | `APP1 running...` | 500 节拍，当前为 500 ms |
| APP2 | `APP2 running...` | 200 节拍，当前为 200 ms |

当前 FreeRTOS 节拍频率为 1 kHz。[示例目录](04_BIN_EncryptionBIN/)保存 BIN 与加密包；验证当前源码或修改后的代码时，应重新编译并重新加密。若无有效 APP 或恢复失败，留在 Bootloader 后按 PA0 重新发送兼容加密包。

## 许可证

本项目采用 **MIT License** 开源许可证。Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](../LICENSE) 文件。第三方组件保留原版权与许可。
