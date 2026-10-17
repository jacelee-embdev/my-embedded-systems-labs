**简体中文** | [English](README.md)

# STM32F411 Bootloader

基于 STM32F411CEU6 和 SPL 的裸机 Bootloader，负责 YMODEM 下载、AES-256-CBC 解密、APP 安装与旧固件恢复。使用 W25Q64 暂存固件，AT24C02 保存状态，RTT / EasyLogger 输出日志。当前版本为 `v1.0`。

## 工程结构

| 目录 | 职责 |
| --- | --- |
| `01_Drivers/` | CMSIS、SPL 厂商库 |
| `02_Core/` | 启动、中断、外设和延时 |
| `03_BSP/` | 按键、LED、EEPROM、I²C 和 Flash 驱动 |
| `04_Middlewares_User/` | Ymodem、AES、CRC、SFUD 和日志 |
| `05_Business/Bootloader/` | 升级状态机、安装和恢复 |
| `06_MDK-ARM/` | Keil 工程与散装文件 |

入口为 [main.c](02_Core/Entry/main.c)，升级逻辑见 [bootloader.c](05_Business/Bootloader/bootloader.c)。

## 编译与下载

1. 使用 Keil MDK 5 / ARMCC 5.06 update 7 打开 [工程文件](06_MDK-ARM/STM32F411CEU6_SPL.uvprojx)，保留自带散装配置，编译并烧录。
2. 将链接于 `0x08008000` 的 APP BIN 加密为 `.mxxx`，加密密钥和初始 IV 须与 `bootloader.c` 一致。
3. 按 PA0 进入下载，以 USART1、115200、8N1 通过 YMODEM 发送加密包，使用 RTT 查看运行日志。

产物位于 `06_MDK-ARM/Objects/`，映射文件位于 `06_MDK-ARM/Listings/`。接线见[总工程说明](../README_zh.md)。

## 升级与恢复

下载到 A 区 → 解密到 B 区 → 备份有效旧 APP 到 A 区 → 安装 B 区固件 → 首次启动确认。

| EEPROM 状态 | 含义 |
| --- | --- |
| `0x00` | 正常启动，按键可进入下载 |
| `0x11` | 下载未完成，优先启动当前有效 APP |
| `0x22` | 加密包已提交，等待安装 |
| `0x33` | 安装完成，等待首次试运行 |
| `0x44` | 试运行未确认，复位后尝试恢复旧 APP |
| `0x55` | 禁止启动当前 APP，等待按键下载 |

首次试运行前确认写入 `0x44`，启动约 6 秒 IWDG；APP 写回 `0x00` 并等待看门狗复位后进入正常启动。若仍为 `0x44`，则校验旧备份及 CRC 后恢复；无有效备份或恢复失败时等待重新下载。首次安装没有旧版本可恢复。

## 存储约定

| 区域 | 地址范围 | 用途 |
| --- | --- | --- |
| 内部 Flash | `0x08000000–0x08007FFF` | Bootloader，32 KiB |
| 内部 Flash | `0x08008000–0x0801FFFF` | APP 运行区，96 KiB |
| W25Q A 区 | `0x000000–0x00FFFF` | 下载密文，解密后存放旧 APP 备份 |
| W25Q B 区 | `0x010000–0x01FFFF` | 新 APP 明文 |

EEPROM：`0x00` 保存状态，`0x01–0x04` 保存加密包长度，`0x05–0x08` 保存当前 APP 长度，`0x10–0x1F` 保存旧备份记录。

加密包包含 16 字节头部及按 16 字节对齐的 APP 数据，总长不超过 64 KiB，APP 明文最大 **65520 字节**。首次确认发生在 APP 的 BSP 初始化后、RTOS 和业务任务启动前。

## 许可证

本项目采用 **MIT License** 开源许可证。Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](../../LICENSE) 文件。第三方组件保留原版权与许可。
