**English** | [简体中文](README_zh.md)

# STM32F411 APP1

An STM32F411CEU6 application using HAL and FreeRTOS. It receives encrypted update packages over USART1 / YMODEM, writes them to W25Q through SFUD, records EEPROM update state, and resets into the Bootloader for installation. It also provides the first-start confirmation used by the Bootloader's watchdog recovery protocol. APP1 receives APP2 to demonstrate successive application versions.

## File Structure

```text
Drivers/            CMSIS and HAL vendor libraries
Core/               Entry, peripheral initialization, interrupts, and RTOS integration
BSP/                AT24Cxx, software I²C, W25Qxx, and UART receive-event adapter
Middlewares/        CubeMX-managed FreeRTOS
Middlewares_User/   RTT, EasyLogger (including debug), Ymodem, CRC, and SFUD
Business/           Init, Status, and OTA
MDK-ARM/            Keil project, Objects, and Listings
STM32F411CEU6.ioc   CubeMX configuration
.mxproject          CubeMX previous-generation records
```

CubeMX-managed directories and configuration retain their original paths. User code is organized by device, middleware, and application logic. Keil file groups are independent of disk directories; after regeneration, check user source references, include paths, and file groups.

## Build and Update

1. Open [STM32F411CEU6.uvprojx](MDK-ARM/STM32F411CEU6.uvprojx), build with Keil MDK 5 / ARMCC 5.06 update 7, and keep the APP link address at `0x08008000`.
2. Encrypt `MDK-ARM/Objects/STM32F411CEU6.bin` into a `.mxxx` package and install APP1 through the Bootloader.
3. While APP1 is running, send the hexadecimal bytes `11 22 33`, then send the encrypted APP2 package over YMODEM. After receiving `44 55 66`, send `77 88 99` and press PA0 to reset and install the update.

Build products are in `MDK-ARM/Objects`; listings and linker maps are in `MDK-ARM/Listings`.

## First-Start Confirmation and Recovery

After installing a new APP, the Bootloader saves `0x33` and resets. It then verifies state `0x44`, enables IWDG, and launches the trial APP. In `Core/Src/main.c`, the APP reads EEPROM after peripheral and BSP initialization. On `0x44`, it writes `0x00` and waits for the watchdog to reset the MCU. If `0x00` was saved, the Bootloader starts the new APP normally; on `0x00`, the APP starts FreeRTOS and business tasks. Other state values request a software reset.

If the APP does not reach this confirmation point, or `0x00` is not saved, the state remains `0x44`. After reset, the Bootloader validates and restores a persistent old-APP backup. With no usable backup or failed recovery, it blocks the unconfirmed image and waits for PA0 and a replacement YMODEM download, attempting to persist state `0x55`. First installation has no old version to restore. The backup record at EEPROM `0x10–0x1F`, restoration, CRC checks, and download-only handling are owned by the Bootloader.

This uses the APP's existing confirmation protocol. The confirmation point follows peripheral and BSP initialization, including W25Q, and precedes the RTOS scheduler and business tasks; it confirms reaching that startup point. Failures after saving `0x00` are outside this recovery scope. See the [Bootloader README](../01_Bootloader/README.md) for the state and backup layouts and recovery behavior.

During application-side reception, the OTA task saves `0x11` before downloading, verifies the completed package and its length at `0x01–0x04`, and commits `0x22` before replying `44 55 66`. After the update command and button confirmation, a software reset hands installation to the Bootloader. A failed download or metadata commit returns to download waiting; the button confirmation window is approximately 20 seconds.

## Differences from APP2

In `Business/Status/app.c`, APP1 logs `APP1 running...` and toggles the LED every 500 RTOS ticks; APP2 logs `APP2 running...` every 200 ticks. With the current 1 kHz tick rate, these intervals are 500 ms and 200 ms. The startup log in `Core/Src/main.c` also identifies the application version. Both applications use the same firmware update and confirmation protocol.

APP2 retains the same reception and confirmation protocol and can continue receiving compatible firmware updates. See the [repository README](../README.md) for the full workflow and hardware connections.

## License

This project is licensed under the **MIT License**. Copyright (c) 2026 JesMicro. See the [LICENSE](../../LICENSE) file in the repository root for the full terms. Third-party components retain their original copyrights and licenses.
