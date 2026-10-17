**English** | [简体中文](README_zh.md)

# STM32F411 Bootloader

A bare-metal Bootloader for STM32F411CEU6 using SPL. It receives AES-256-CBC encrypted packages over USART1 / YMODEM, accesses W25Q64 through SFUD, and installs the APP into internal Flash. AT24C02 stores update states and backup records; IWDG supports first-start confirmation, and RTT / EasyLogger provide logs.

The Bootloader is finalized as **v1.0** (2026-10-16), with optimization, refactoring, and code standardization complete.

## Directories and Entry Points

| Directory | Responsibility |
| --- | --- |
| `01_Drivers/` | CMSIS and SPL vendor libraries |
| `02_Core/` | Startup, interrupts, peripherals, and delays; entry: `Entry/main.c` |
| `03_BSP/` | Button, LED, AT24Cxx, software I²C, and W25QXX |
| `04_Middlewares_User/` | Ymodem, AES, CRC, SFUD, EasyLogger, and RTT |
| `05_Business/Bootloader/` | APP validation, decryption, backup, installation, recovery, and state machine |
| `06_MDK-ARM/` | Keil project and scatter file |
| `07_QBuildViewer/` | Build result viewer |

The business layer exposes three APIs: `Bootloader_JumpToApp()`, `Bootloader_DownloadApp()`, and `Bootloader_ProcessState()`.

`main()` explicitly initializes the EEPROM bus through `AT24Cxx_Init()`. `IWDG_Init()` only configures the prescaler without starting the counter. The first trial-run branch calls `IWDG_Start(6000U)` for a nominal timeout of about 6 seconds; the actual duration depends on the LSI frequency.

## Build and Download

1. Open [STM32F411CEU6_SPL.uvprojx](06_MDK-ARM/STM32F411CEU6_SPL.uvprojx) with Keil MDK 5 / ARMCC 5.06 update 7, build, and flash.
2. Encrypt an APP BIN linked at `0x08008000` into `.mxxx` using a compatible tool. Press PA0 to enter download mode and send it over YMODEM at 115200, 8N1.
3. Use RTT to monitor installation, first-start confirmation, and APP logs. If no valid APP is available or recovery fails, press PA0 to download again.

Build products are in `06_MDK-ARM/Objects/`; linker maps are in `06_MDK-ARM/Listings/`. See the [repository documentation](../README.md) for wiring and application-side updates.

## Storage Layout

| Location | Address range | Purpose |
| --- | --- | --- |
| Internal Flash | `0x08000000–0x08007FFF` | Bootloader, 32 KiB |
| Internal Flash | `0x08008000–0x0801FFFF` | APP execution region; current code allows 96 KiB |
| W25Q region A | `0x000000–0x00FFFF` | 64 KiB; ciphertext staging, then reusable for the old APP backup after decryption |
| W25Q region B | `0x010000–0x01FFFF` | 64 KiB; new APP plaintext staging |
| Retained RAM | `0x2001FFF0–0x2001FFFF` | 16-byte UNINIT region; the first 4 bytes hold the jump request |

[bootloader_noinit.sct](06_MDK-ARM/bootloader_noinit.sct) reserves this RAM region. `g_app_jump_request` has no initializer. After the Bootloader writes the request and resets, `main()` clears it before attempting to jump to the APP.

The package consists of a 16-byte header and APP data padded to a 16-byte boundary, encrypted together using AES-256-CBC. The last 4 bytes of the decrypted header contain the little-endian APP length. The encryption tool's key and initial IV must match the read-only configurations `g_app_aes_key` and `g_app_aes_initial_iv` in [bootloader.c](05_Business/Bootloader/bootloader.c). The working IV is initialized for each new package, then CBC chaining continues across its blocks. The complete encrypted package is limited to 64 KiB, so the maximum updatable APP plaintext size is **65520 bytes**.

## States and Recovery

Installation proceeds as follows: receive into A → decrypt into B → back up a valid old APP into A and commit its CRC record → verify state `0x44` → install the new APP → save its length and state `0x33` → software reset. Failure to read the old APP length aborts the installation.

| EEPROM state | Action |
| --- | --- |
| `0x00` | Normal startup; a button press can enter download mode |
| `0x11` | Incomplete download; try the current valid APP, otherwise download again |
| `0x22` | Restore the saved package length, then decrypt and install directly |
| `0x33` | Set and verify update-failed state `0x44`, start IWDG for about 6 seconds, and trial-start the new APP |
| `0x44` | Failure marker set before the trial run; if retained after reset, validate and restore the old APP |
| `0x55` | Block the unconfirmed image and wait for a button-triggered download |

At its existing confirmation point, the APP writes `0x00` and waits for IWDG reset. If the state is still `0x44` after reset, the Bootloader checks the backup record, entry vector, and complete plaintext CRC before restoring the old APP and verifying the internal Flash result. The first installation has no backup; missing backups or failed recovery lead to download waiting.

| EEPROM address | Contents |
| --- | --- |
| `0x00` | Update state |
| `0x01–0x04` | Complete encrypted package length, little-endian uint32 |
| `0x05–0x08` | Current internal APP plaintext length, little-endian uint32 |
| `0x10–0x1F` | Backup record: valid marker, version, reserved bytes, old length, plaintext CRC, and record CRC |

The old backup uses CRC-32/ISO-HDLC. The valid marker `0xA5` is committed last, and the previous record is invalidated before overwriting A. APP confirmation follows peripheral and BSP initialization, including W25Q, and precedes the RTOS and business tasks. Runtime failures after saving `0x00` are outside this rollback scope.

## License

This project is licensed under the **MIT License**. Copyright (c) 2026 JesMicro. See the [LICENSE](../../LICENSE) file in the repository root for the full terms. Third-party components retain their original copyrights and licenses.
