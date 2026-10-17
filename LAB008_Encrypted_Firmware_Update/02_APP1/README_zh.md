**简体中文** | [English](README.md)

# STM32F411 APP1

基于 STM32F411CEU6、HAL 和 FreeRTOS 的应用工程。在运行期间接收加密固件到 W25Q A 区，提交 EEPROM 状态，复位后由 Bootloader 完成安装。APP1 作为初始演示固件，可升级到 APP2。

## 代码入口

| 文件 | 职责 |
| --- | --- |
| [main.c](Core/Src/main.c) | 外设、BSP 初始化和首次启动确认 |
| [os_init.c](Business/Init/os_init.c) | 创建串口接收队列和业务线程 |
| [app.c](Business/Status/app.c) | 状态日志和 LED 翻转 |
| [fw_update.c](Business/OTA/fw_update.c) | 下载命令、固件接收、写入协作和安装确认 |

`BSP/` 提供器件驱动，`Middlewares_User/` 包含 Ymodem、CRC、SFUD 和日志。CubeMX 生成文件的用户修改放在 USER CODE 区域。

## 编译与升级

1. 使用 Keil MDK 5 / ARMCC 5.06 update 7 打开 [工程文件](MDK-ARM/STM32F411CEU6.uvprojx)，APP 链接地址为 `0x08008000`。
2. 将 `MDK-ARM/Objects/STM32F411CEU6.bin` 加密为 `.mxxx`，通过 Bootloader 安装 APP1。
3. APP1 运行后，发送十六进制命令 `11 22 33`，用 YMODEM 发送 APP2 加密包；收到 `44 55 66` 后发送 `77 88 99`，按 PA0 确认复位安装。

产物位于 `MDK-ARM/Objects/`，映射文件位于 `MDK-ARM/Listings/`。接线和完整演示步骤见[总工程说明](../README_zh.md)。

## 运行约定

`fw_update_task` 依次等待下载命令、接收并提交固件、等待安装命令和按键确认；`fw_write_task` 在下载期间负责写入 A 区，停止确认后再释放资源。

下载前写入 `0x11`，数据和长度确认后提交 `0x22`。按键超时返回下载等待，已提交标志保留。首次试运行时，`main.c` 在 BSP 初始化后、RTOS 启动前将 `0x44` 写回 `0x00`，等待 IWDG 复位。状态与失败恢复见 [Bootloader 说明](../01_Bootloader/README_zh.md)。

## 与 APP2 的区别

两者升级流程相同，主要通过状态线程的日志和 LED 翻转间隔区分。

| 固件 | 线程日志 | LED 翻转间隔 |
| --- | --- | --- |
| APP1 | `APP1 running...` | 500 节拍（500 ms） |
| APP2 | `APP2 running...` | 200 节拍（200 ms） |

当前 FreeRTOS 节拍频率为 1 kHz。

## 许可证

本项目采用 **MIT License** 开源许可证。Copyright (c) 2026 JesMicro。完整条款请参阅根目录的 [LICENSE](../../LICENSE) 文件。第三方组件保留原版权与许可。
