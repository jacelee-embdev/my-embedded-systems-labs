**English** | [简体中文](README_zh.md)

# STM32F411 Encrypted Firmware Updates

A firmware update example for **STM32F411CEU6**, covering reception, external storage staging, AES decryption, previous-version backup, internal Flash installation, first-start confirmation, and failure recovery. Optimization, refactoring, and code standardization are complete for the Bootloader and APP projects. The current version is **v1.0**.

The repository contains an SPL-based bare-metal Bootloader and two HAL / FreeRTOS applications. The Bootloader can receive the first application package directly. For later updates, the running APP can receive a package and reset into the Bootloader for installation. APP1 and APP2 share one execution region; different logs and LED toggle intervals demonstrate the version change.

| Project | Software stack | Responsibility |
| --- | --- | --- |
| [Bootloader](01_Bootloader/README.md) | SPL, bare metal | Download, decryption, backup, installation, startup, and failure recovery |
| [APP1](02_APP1/README.md) | HAL, FreeRTOS | Initial demonstration firmware; receives the next encrypted package while running |
| [APP2](02_APP2/README.md) | HAL, FreeRTOS | Update demonstration firmware; can also receive subsequent compatible packages |

This guide introduces requirements, hardware, storage, project structure, software architecture, and runtime behavior. The 11 design diagrams are in [05_Docs/Images](05_Docs/Images/); click an image to view the original. Diagram labels remain in Chinese, with English explanations below. Use the project links above for individual entry points, configuration, and implementation details.

## Contents

- [Requirements and Approach](#requirements-and-approach)
- [Hardware and Wiring](#hardware-and-wiring)
- [Storage Layout](#storage-layout)
- [Directories and Project Trees](#directories-and-project-trees)
- [Software Architecture](#software-architecture)
- [Firmware Update Flow](#firmware-update-flow)
- [Update State Machine](#update-state-machine)
- [Serial Commands and YMODEM Transfer](#serial-commands-and-ymodem-transfer)
- [AES Packages and Decryption](#aes-packages-and-decryption)
- [Build and Quick Start](#build-and-quick-start)
- [License](#license)

## Requirements and Approach

The system addresses five requirements: first installation and subsequent updates, reception checks, encrypted firmware handling, persistent state across resets, and recovery when a valid previous-APP backup is available.

The implementation uses USART1 / YMODEM for reception, W25Q64 regions A and B for ciphertext, plaintext, and backup staging, and AT24C02 for update states and lengths. The Bootloader owns decryption and installation. IWDG and an APP confirmation flag determine the first-start result, while RTT, EasyLogger, and the LED provide feedback.

![DSN-01: Requirements and implementation approach](05_Docs/Images/DSN-01%20需求分析与方案制定.png)

The diagram maps each requirement to its implementation and establishes the basis for the hardware, storage, protocol, and state-machine designs that follow.

## Hardware and Wiring

The Bootloader, APP1, and APP2 use the same hardware. W25Q64 stages packages and backups, AT24C02 stores update configuration, PA0 enters download mode or confirms installation, and the PC13 LED indicates runtime status.

![DSN-02: Hardware architecture](05_Docs/Images/DSN-02%20硬件架构设计.png)

| Module / interface | MCU pins and configuration | Purpose |
| --- | --- | --- |
| USART1 | PA9: TX; PA10: RX; 115200, 8N1 | Serial commands and YMODEM firmware transfer |
| W25Q64 / SPI1 | PA4: CS; PA5: SCK; PA6: MISO; PA7: MOSI | External SPI Flash |
| AT24C02 / software I²C | PB8: SCL; PB9: SDA | State, lengths, and previous-APP backup record |
| Button | PA0, active low | Bootloader download entry and APP installation confirmation |
| LED | PC13, active low | Runtime status and demonstration version identification |
| J-Link / SWD | PA13: SWDIO; PA14: SWCLK | Flashing, debugging, and RTT logs |

Use 3.3 V logic and a common ground. Connect MCU TX to module RX and MCU RX to module TX. The diagram shows CH340 wired serial and HC-05 transparent Bluetooth serial as alternative USART1 connections. Connect one at a time and use matching serial parameters.

## Storage Layout

Internal Flash holds the Bootloader and the running APP in separate regions. External Flash regions A and B support the update process, while EEPROM retains configuration and backup records across resets.

![DSN-03: Storage layout](05_Docs/Images/DSN-03%20存储分布设计.png)

| Location | Address range | Capacity | Purpose |
| --- | --- | --- | --- |
| Internal Flash | `0x08000000–0x08007FFF` | 32 KiB | Bootloader |
| Internal Flash | `0x08008000–0x0801FFFF` | 96 KiB | Shared APP1 / APP2 execution region |
| Internal Flash | `0x08020000–0x0807FFFF` | 384 KiB | Currently unused |
| W25Q region A | `0x000000–0x00FFFF` | 64 KiB | New encrypted package, then reused for the previous APP backup after decryption |
| W25Q region B | `0x010000–0x01FFFF` | 64 KiB | Valid plaintext of the new APP |
| Remaining W25Q space | `0x020000–0x7FFFFF` | 8064 KiB | Currently unallocated |

A and B are external storage processing regions. During an update, the Bootloader installs the new firmware from B into the single internal APP execution region, where APP1 and APP2 replace each other. Recovery uses the verified previous-APP backup in A.

| EEPROM address | Contents |
| --- | --- |
| `0x00` | Update state |
| `0x01–0x04` | Complete encrypted package length, little-endian uint32 |
| `0x05–0x08` | Valid plaintext length of the installed internal APP, little-endian uint32 |
| `0x10–0x1F` | Previous-APP backup record: valid marker, format version, reserved fields, length, plaintext CRC, and record CRC |

The Bootloader [scatter file](01_Bootloader/06_MDK-ARM/bootloader_noinit.sct) also reserves 16 bytes of SRAM at `0x2001FFF0–0x2001FFFF` for an early-jump request retained across a software reset.

Although the internal APP execution region is 96 KiB, the current **complete encrypted package limit is 64 KiB, and the maximum valid APP plaintext length is 65520 bytes**, constrained by external staging capacity and header overhead.

## Directories and Project Trees

```text
.
├── 01_Bootloader/          SPL bare-metal Bootloader
├── 02_APP1/                HAL / FreeRTOS application example 1
├── 02_APP2/                HAL / FreeRTOS application example 2
├── 04_BIN_EncryptionBIN/   Example BIN files and .mxxx encrypted packages
├── 05_Docs/
│   └── Images/             Requirements, architecture, flow, and algorithm diagrams
├── README_zh.md            Chinese repository guide
└── README.md               English repository guide
```

![DSN-04: File structure and Keil project trees](05_Docs/Images/DSN-04%20工程文件结构与工程树设计.png)

Using the Bootloader and APP1 as examples, the diagram shows both disk directories and Keil project trees. The Bootloader separates drivers, core support, BSP, middleware, and business logic. The APP retains CubeMX-managed `Core/`, `Drivers/`, and `Middlewares/` directories, with user drivers, middleware, and business logic in `BSP/`, `Middlewares_User/`, and `Business/`. APP2 follows the same structure.

Keil file groups and disk directories are managed separately. After regenerating CubeMX code, check user source references, include paths, and project groups. Keep user changes to generated files inside USER CODE sections.

## Software Architecture

The Bootloader and APP run in separate phases, sharing wiring, external storage layout, and the EEPROM protocol. They use SPL and HAL / FreeRTOS respectively for platform support and hand control over through resets and application jumps.

![DSN-05: Software architecture](05_Docs/Images/DSN-05%20软件架构设计.png)

| Layer | Bootloader | APP |
| --- | --- | --- |
| Chip libraries | CMSIS, SPL | CMSIS, HAL |
| Peripherals and system support | Startup, interrupts, USART1, SPI1, internal Flash, IWDG, delays | Startup, interrupts, GPIO, USART1 / DMA, SPI1, timebase |
| BSP | Button, LED, AT24Cxx, W25Q region management | Button, LED, AT24Cxx, UART receive adapter, W25Q region management |
| Middleware | YMODEM, AES-256-CBC, CRC, SFUD, EasyLogger / RTT | YMODEM, CRC, SFUD, FreeRTOS, EasyLogger / RTT |
| Business logic | State dispatch, decryption, backup, installation, recovery, startup | Task creation, firmware reception, coordinated writes, installation confirmation, runtime status |

The Bootloader entry is [main.c](01_Bootloader/02_Core/Entry/main.c), and its update logic is in [bootloader.c](01_Bootloader/05_Business/Bootloader/bootloader.c). In the APP, [main.c](02_APP1/Core/Src/main.c) performs initialization and first-start confirmation, [os_init.c](02_APP1/Business/Init/os_init.c) creates tasks, `fw_update.c` coordinates reception and writes, and [app.c](02_APP1/Business/Status/app.c) logs status and toggles the LED.

The APP receive path uses USART1 DMA, two buffers, a length-notification queue, a data-pointer queue, and semaphores. The receive and write tasks coordinate per packet before reusing a buffer. On completion or failure, resource cleanup waits for the write task to acknowledge stopping.

## Firmware Update Flow

### Overall Flow

First installation and application-side updates share the Bootloader's decryption, backup, installation, and first-start confirmation process. The package is either received directly by the Bootloader or received and committed in advance by the running APP.

![DSN-06-01: Overall firmware update flow](05_Docs/Images/DSN-06-01%20流程设计_固件更新整体流程.png)

1. Build the APP on the host and encrypt its BIN into the agreed `.mxxx` format.
2. Receive the encrypted package into W25Q region A through Bootloader or APP YMODEM reception.
3. The Bootloader decrypts A into B and checks lengths and the application entry vector.
4. Back up a valid previous APP into A and commit its length, CRC, and other backup metadata.
5. Write the new APP from B into internal Flash, compare the readback, and save its length and pending-trial state.
6. Reset into the first trial run. The APP writes its confirmation flag and waits for IWDG reset before normal startup.
7. If first-start confirmation fails, validate and attempt to restore the previous backup in A. Without a valid backup, or if recovery fails, wait for another download.

### Bootloader Flow

The Bootloader uses EEPROM state to select normal startup, installation of a downloaded package, first trial run, previous-APP recovery, or download waiting. Normal startup uses a retained-RAM request and software reset to jump to the APP early on the next entry into the startup code.

![DSN-06-02: Bootloader flow](05_Docs/Images/DSN-06-02%20流程设计_Bootloader侧流程.png)

The previous backup can overwrite the ciphertext in A only after successful decryption. Recovery requires a valid backup record and complete plaintext CRC; installation and recovery both check the write results. Before the first trial run, the Bootloader verifies that `0x44` was saved and starts IWDG with a nominal timeout of about 6 seconds. An unconfirmed APP leaves the failure state in place after reset.

### APP Flow

While normal business tasks run, the APP waits for a download command, receives an encrypted package, and coordinates writing it to A. It replies with download completion after verifying the data flush, length, and saved state. An installation command followed by a button press triggers a software reset and hands control to the Bootloader.

![DSN-06-03: APP flow](05_Docs/Images/DSN-06-03%20流程设计_APP侧流程.png)

The button confirmation window is approximately 20 seconds. A timeout returns the task to download-command waiting while retaining the committed `0x22` state, so a later reset can still trigger installation. First-start confirmation occurs in `main.c` after peripheral and BSP initialization and before the RTOS and business tasks start. The APP attempts to write `0x00` and waits for watchdog reset; the Bootloader then reads the state that was actually saved.

First installation has no previous version to restore. This mechanism checks whether the APP reaches that startup confirmation point. Business-runtime failures after confirmation are outside the current rollback decision.

## Update State Machine

The system has six EEPROM states retained across resets and four RAM states used only by the APP firmware update thread. After reset, the Bootloader resumes processing from EEPROM state, while APP thread state is initialized again at startup.

![DSN-07: Update state machine](05_Docs/Images/DSN-07%20OTA%20状态机设计.png)

| EEPROM state | Meaning | Main Bootloader action |
| --- | --- | --- |
| `0x00` | Normal startup | Start a valid APP or enter download on a button press |
| `0x11` | Incomplete download | Try the current valid APP first; otherwise download again |
| `0x22` | Complete encrypted package committed | Restore package length, decrypt, back up, and install |
| `0x33` | Installed; first trial run pending | Save and verify `0x44`, start IWDG, and jump to the new APP |
| `0x44` | Update failed or first start unconfirmed | Validate and attempt to restore the previous APP |
| `0x55` | Another download required | Block the current APP and wait for a button-triggered download |

The APP thread progresses through “wait for download command → receive and commit → wait for installation command → wait for button confirmation.” State `0x22` is already saved when the download is committed; the installation command and button determine when to reset. Thread waiting state and EEPROM state are managed separately.

## Serial Commands and YMODEM Transfer

### Application Control Commands

The following sequences are **raw hexadecimal bytes**. Send them using the serial tool's HEX mode.

| Direction | Bytes | Meaning |
| --- | --- | --- |
| Host → APP | `11 22 33` | Request reception of the next encrypted package |
| APP → host | `44 55 66` | Download, storage flush, and metadata commit completed; installation-command reception ready |
| Host → APP | `77 88 99` | Request installation, then wait for PA0 button confirmation |

Direct Bootloader downloading is triggered by PA0. These three-byte commands apply to the running APP.

### YMODEM Single-File Transfer

![DSN-08: Typical YMODEM single-file transfer](05_Docs/Images/DSN-08%20Ymodem单文件典型传输流程分析.png)

The receiver sends `C` to request CRC mode. The host sends block 0 with a filename and length, followed by data packets. This implementation ends a single-file session using EOT, ACK, `C`, and an empty-filename final block. The receiver checks the sequence number, its complement, CRC-16/XMODEM, and declared length. Valid retransmissions of the previous packet are acknowledged without duplicate writes.

The Bootloader uses polling reception with synchronous storage handling. The APP uses DMA / IDLE or transfer-complete notifications, two buffers, and a cooperating write task. The final YMODEM ACK ends the protocol session; the APP still needs to flush the remaining data and commit metadata, so the host should wait for `44 55 66` before sending the installation command.

Packet CRC checks transmission. The previous-APP backup and its record use separate CRC-32/ISO-HDLC checks. Reception saves valid file bytes only; YMODEM padding is excluded from the firmware length.

## AES Packages and Decryption

The host creates an AES-256-CBC encrypted package, and the Bootloader decrypts it using the same key and initial IV. The APP receives and stores the package; the Bootloader owns decryption and installation of the new APP.

![DSN-09: AES algorithm and firmware decryption](05_Docs/Images/DSN-09%20AES加密算法流程展示.png)

The left side illustrates AES-256 key expansion and encryption / decryption rounds. The right side maps the algorithm to the project's ciphertext-in-A to plaintext-in-B processing flow.

| Item | Convention |
| --- | --- |
| Algorithm | AES-256-CBC; 32-byte key, 16-byte blocks and IV |
| Package contents | A 16-byte header and APP data padded to a 16-byte boundary, encrypted together |
| APP length | Bytes 12–15 of the decrypted header, little-endian uint32 |
| CBC chain | Initialize the working IV once per package; process the header and subsequent blocks continuously |
| Package length | `16 + ceil(valid APP length / 16) × 16`, at most 65536 bytes in total |
| Saved plaintext | Write the valid length recorded in the header to B, discarding alignment padding |
| Pre-installation checks | Package length, block alignment, plaintext-length relationship, written length, and APP entry vector |

The key and initial IV are defined as `g_app_aes_key` and `g_app_aes_initial_iv` in [bootloader.c](01_Bootloader/05_Business/Bootloader/bootloader.c). A package-generation tool must match those settings and the package format. The current implementation extracts plaintext according to the recorded valid length.

## Build and Quick Start

### Prerequisites

- Keil MDK 5; the projects are configured for ARMCC 5.06 update 7.
- J-Link for SWD flashing and RTT log viewing.
- A serial tool supporting YMODEM single-file sending and HEX commands at 115200, 8N1.
- An encryption tool compatible with the Bootloader AES configuration and package format. Example packages are provided; the encryption tool must be obtained separately.

| Project | Keil project file | Address and output |
| --- | --- | --- |
| Bootloader | [STM32F411CEU6_SPL.uvprojx](01_Bootloader/06_MDK-ARM/STM32F411CEU6_SPL.uvprojx) | Starts at `0x08000000`; output in `06_MDK-ARM/Objects/` |
| APP1 | [STM32F411CEU6.uvprojx](02_APP1/MDK-ARM/STM32F411CEU6.uvprojx) | Linked at `0x08008000`; BIN at `MDK-ARM/Objects/STM32F411CEU6.bin` |
| APP2 | [STM32F411CEU6.uvprojx](02_APP2/MDK-ARM/STM32F411CEU6.uvprojx) | Linked at `0x08008000`; BIN at `MDK-ARM/Objects/STM32F411CEU6.bin` |

Keep the existing linker settings, including the Bootloader scatter file. Each project's `Listings/` contains listings and maps. APP1 and APP2 produce BIN files with the same name in their respective project directories; distinguish the source when encrypting them.

### First Installation of APP1

1. Connect the hardware according to the wiring table, build the Bootloader, and flash it using J-Link.
2. Build APP1 and encrypt its BIN into `.mxxx`, or use the [APP1 example package](04_BIN_EncryptionBIN/APP1_EncryptionBIN.mxxx) for a demonstration.
3. After reset, press PA0 to enter Bootloader download mode and send the encrypted APP1 package using the serial tool's YMODEM sender.
4. Follow reception, decryption, and installation through RTT logs. After confirming its first trial start, the new APP waits for IWDG reset and then starts normally.
5. Check for `APP1 running...` and a 500 ms LED toggle interval.

### Update APP1 to APP2

1. Build and encrypt APP2, or use the [APP2 example package](04_BIN_EncryptionBIN/APP2_EncryptionBIN.mxxx).
2. While APP1 runs normally, send the HEX command `11 22 33`.
3. Send the APP2 encrypted package over YMODEM. Wait for protocol completion and the application reply `44 55 66`.
4. Send the HEX command `77 88 99` and press PA0 within the approximately 20-second confirmation window.
5. The device resets, and the Bootloader decrypts the package, backs up the previous APP, installs the new APP, and performs first-start confirmation.
6. Check for `APP2 running...` and a 200 ms LED toggle interval. APP2 can receive subsequent compatible firmware using the same procedure.

| Demonstration firmware | Runtime log | LED toggle interval |
| --- | --- | --- |
| APP1 | `APP1 running...` | 500 ticks, currently 500 ms |
| APP2 | `APP2 running...` | 200 ticks, currently 200 ms |

The current FreeRTOS tick rate is 1 kHz. The [example directory](04_BIN_EncryptionBIN/) contains BIN files and encrypted packages. Rebuild and encrypt again when validating the current source or your own changes. If there is no valid APP or recovery fails, stay in the Bootloader and press PA0 to send a replacement compatible package.

## License

This project is licensed under the **MIT License**. Copyright (c) 2026 JesMicro. See the [LICENSE](../LICENSE) file in the repository root for the full terms. Third-party components retain their original copyrights and licenses.
