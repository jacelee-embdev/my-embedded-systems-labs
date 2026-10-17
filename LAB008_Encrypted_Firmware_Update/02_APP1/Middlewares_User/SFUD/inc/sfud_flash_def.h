/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It is the flash types and specification macro definition head file
 *     for this library.
 * Created on: 2016-06-09
 */

/******************************************************************************
 * @file sfud_flash_def.h
 * @brief SFUD integration documentation for the local APP1 project.
 * Original upstream copyright, license and file description are retained.
 * Local comment normalization does not change executable code.
 ******************************************************************************/

#ifndef _SFUD_FLASH_DEF_H_
#define _SFUD_FLASH_DEF_H_

/****************************** Includes ******************************/

#include <stdint.h> /* C 标准库 */

#include <sfud_cfg.h> /* 中间件层：SFUD 接口与配置 */

/****************************** Includes ******************************/

#ifdef __cplusplus
extern "C"
{
#endif

/************************** Type Definitions **************************/

/**
 * 闪存编程模式。
 */
enum sfud_write_mode
{
    SFUD_WM_PAGE_256B   = 1 << 0, /**< 每页写入 1 至 256 字节。 */
    SFUD_WM_BYTE        = 1 << 1, /**< 单字节写入。 */
    SFUD_WM_AAI         = 1 << 2, /**< 地址自动递增写入。 */
    SFUD_WM_DUAL_BUFFER = 1 << 3, /**< 双缓冲写入模式，例如 AT45DB 系列。 */
};

/* 厂商信息。 */
typedef struct
{
    char *name; /**< 厂商名称。 */
    uint8_t id; /**< 厂商标识。 */
} sfud_mf;

/* 闪存芯片参数。 */
typedef struct
{
    char *name;             /**< 芯片名称。 */
    uint8_t mf_id;          /**< 厂商标识。 */
    uint8_t type_id;        /**< 存储类型标识。 */
    uint8_t capacity_id;    /**< 容量标识。 */
    uint32_t capacity;      /**< 芯片容量，单位为字节。 */
    uint16_t write_mode;    /**< 写入模式，参见 sfud_write_mode。 */
    uint32_t erase_gran;    /**< 擦除粒度，单位为字节。 */
    uint8_t erase_gran_cmd; /**< 该擦除粒度对应的擦除命令。 */
} sfud_flash_chip;

#ifdef SFUD_USING_QSPI
/* 四线总线芯片相对于单线总线的扩展参数。 */
typedef struct
{
    uint8_t mf_id;       /**< 厂商标识。 */
    uint8_t type_id;     /**< 存储类型标识。 */
    uint8_t capacity_id; /**< 容量标识。 */
    uint8_t read_mode;   /**< 芯片支持的快速读取模式。 */
} sfud_qspi_flash_ext_info;
#endif

/************************** Type Definitions **************************/

/****************************** Defines ******************************/

/* 支持的厂商 JEDEC 标识。 */
#define SFUD_MF_ID_CYPRESS    0x01
#define SFUD_MF_ID_FUJITSU    0x04
#define SFUD_MF_ID_EON        0x1C
#define SFUD_MF_ID_ATMEL      0x1F
#define SFUD_MF_ID_MICRON     0x20
#define SFUD_MF_ID_AMIC       0x37
#define SFUD_MF_ID_NOR_MEM    0x52
#define SFUD_MF_ID_SANYO      0x62
#define SFUD_MF_ID_INTEL      0x89
#define SFUD_MF_ID_ESMT       0x8C
#define SFUD_MF_ID_FUDAN      0xA1
#define SFUD_MF_ID_HYUNDAI    0xAD
#define SFUD_MF_ID_SST        0xBF
#define SFUD_MF_ID_MACRONIX   0xC2
#define SFUD_MF_ID_GIGADEVICE 0xC8
#define SFUD_MF_ID_ISSI       0xD5
#define SFUD_MF_ID_WINBOND    0xEF
#define SFUD_MF_ID_PUYA       0x85

/* 支持的厂商信息表。 */
#define SFUD_MF_TABLE                                                          \
    {                                                                          \
        { "Cypress", SFUD_MF_ID_CYPRESS },                                     \
        { "Fujitsu", SFUD_MF_ID_FUJITSU },                                     \
        { "EON", SFUD_MF_ID_EON },                                             \
        { "Atmel", SFUD_MF_ID_ATMEL },                                         \
        { "Micron", SFUD_MF_ID_MICRON },                                       \
        { "AMIC", SFUD_MF_ID_AMIC },                                           \
        { "Sanyo", SFUD_MF_ID_SANYO },                                         \
        { "Intel", SFUD_MF_ID_INTEL },                                         \
        { "ESMT", SFUD_MF_ID_ESMT },                                           \
        { "Fudan", SFUD_MF_ID_FUDAN },                                         \
        { "Hyundai", SFUD_MF_ID_HYUNDAI },                                     \
        { "SST", SFUD_MF_ID_SST },                                             \
        { "GigaDevice", SFUD_MF_ID_GIGADEVICE },                               \
        { "ISSI", SFUD_MF_ID_ISSI },                                           \
        { "Winbond", SFUD_MF_ID_WINBOND },                                     \
        { "Macronix", SFUD_MF_ID_MACRONIX },                                   \
        { "NOR-MEM", SFUD_MF_ID_NOR_MEM },                                     \
        { "PUYA", SFUD_MF_ID_PUYA },                                           \
    }

#ifdef SFUD_USING_FLASH_INFO_TABLE
/* 静态芯片参数表；未能通过 SFDP 识别时，用芯片标识匹配参数。
 * 表项依次为：名称、厂商标识、类型标识、容量标识、容量、
 * 写入模式、擦除粒度、擦除命令。 */
#define SFUD_FLASH_CHIP_TABLE                                                  \
    {                                                                          \
        { "AT45DB161E", SFUD_MF_ID_ATMEL, 0x26, 0x00, 2L * 1024L * 1024L,      \
          SFUD_WM_BYTE | SFUD_WM_DUAL_BUFFER, 512, 0x81 },                     \
        { "W25Q40BV", SFUD_MF_ID_WINBOND, 0x40, 0x13, 512L * 1024L,            \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "W25X40CL", SFUD_MF_ID_WINBOND, 0x30, 0x13, 512L * 1024L,            \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "W25X16AV", SFUD_MF_ID_WINBOND, 0x30, 0x15, 2L * 1024L * 1024L,      \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "W25Q16BV", SFUD_MF_ID_WINBOND, 0x40, 0x15, 2L * 1024L * 1024L,      \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "W25Q32BV", SFUD_MF_ID_WINBOND, 0x40, 0x16, 4L * 1024L * 1024L,      \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "W25Q64CV", SFUD_MF_ID_WINBOND, 0x40, 0x17, 8L * 1024L * 1024L,      \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "W25Q64DW", SFUD_MF_ID_WINBOND, 0x60, 0x17, 8L * 1024L * 1024L,      \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "W25Q128BV", SFUD_MF_ID_WINBOND, 0x40, 0x18, 16L * 1024L * 1024L,    \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "W25Q256FV", SFUD_MF_ID_WINBOND, 0x40, 0x19, 32L * 1024L * 1024L,    \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "SST25VF080B", SFUD_MF_ID_SST, 0x25, 0x8E, 1L * 1024L * 1024L,       \
          SFUD_WM_BYTE | SFUD_WM_AAI, 4096, 0x20 },                            \
        { "SST25VF016B", SFUD_MF_ID_SST, 0x25, 0x41, 2L * 1024L * 1024L,       \
          SFUD_WM_BYTE | SFUD_WM_AAI, 4096, 0x20 },                            \
        { "M25P32", SFUD_MF_ID_MICRON, 0x20, 0x16, 4L * 1024L * 1024L,         \
          SFUD_WM_PAGE_256B, 64L * 1024L, 0xD8 },                              \
        { "M25P80", SFUD_MF_ID_MICRON, 0x20, 0x14, 1L * 1024L * 1024L,         \
          SFUD_WM_PAGE_256B, 64L * 1024L, 0xD8 },                              \
        { "M25P40", SFUD_MF_ID_MICRON, 0x20, 0x13, 512L * 1024L,               \
          SFUD_WM_PAGE_256B, 64L * 1024L, 0xD8 },                              \
        { "EN25Q32B", SFUD_MF_ID_EON, 0x30, 0x16, 4L * 1024L * 1024L,          \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "GD25Q64B", SFUD_MF_ID_GIGADEVICE, 0x40, 0x17, 8L * 1024L * 1024L,   \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "GD25Q16B", SFUD_MF_ID_GIGADEVICE, 0x40, 0x15, 2L * 1024L * 1024L,   \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "GD25Q32C", SFUD_MF_ID_GIGADEVICE, 0x40, 0x16, 4L * 1024L * 1024L,   \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "S25FL216K", SFUD_MF_ID_CYPRESS, 0x40, 0x15, 2L * 1024L * 1024L,     \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "S25FL032P", SFUD_MF_ID_CYPRESS, 0x02, 0x15, 4L * 1024L * 1024L,     \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "A25L080", SFUD_MF_ID_AMIC, 0x30, 0x14, 1L * 1024L * 1024L,          \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "F25L004", SFUD_MF_ID_ESMT, 0x20, 0x13, 512L * 1024L,                \
          SFUD_WM_BYTE | SFUD_WM_AAI, 4096, 0x20 },                            \
        { "PCT25VF016B", SFUD_MF_ID_SST, 0x25, 0x41, 2L * 1024L * 1024L,       \
          SFUD_WM_BYTE | SFUD_WM_AAI, 4096, 0x20 },                            \
        { "NM25Q128EVB", SFUD_MF_ID_NOR_MEM, 0x21, 0x18, 16L * 1024L * 1024L,  \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "P25D05H", SFUD_MF_ID_PUYA, 0x60, 0x13, 5L * 1024L,                  \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "P25D10H", SFUD_MF_ID_PUYA, 0x60, 0x12, 1L * 1024L * 1024L,          \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "P25D20H", SFUD_MF_ID_PUYA, 0x60, 0x11, 2L * 1024L * 1024L,          \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "P25D40H", SFUD_MF_ID_PUYA, 0x60, 0x10, 4L * 1024L * 1024L,          \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
        { "P25Q80H", SFUD_MF_ID_PUYA, 0x30, 0x14, 8L * 1024L * 1024L,          \
          SFUD_WM_PAGE_256B, 4096, 0x20 },                                     \
    }
#endif /* SFUD_USING_FLASH_INFO_TABLE */

#ifdef SFUD_USING_QSPI
/* 四线总线快速读取指令表，用于选择芯片支持的读取模式。
 * 表项依次为：厂商标识、类型标识、容量标识、读取模式。 */
#define SFUD_FLASH_EXT_INFO_TABLE                                              \
    {                                                                          \
        /* W25Q40BV */                                                         \
        { SFUD_MF_ID_WINBOND, 0x40, 0x13,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT }, /* W25Q80JV */                      \
        { SFUD_MF_ID_WINBOND, 0x40, 0x14,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT }, /* W25Q16BV */                      \
        { SFUD_MF_ID_WINBOND, 0x40, 0x15,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT }, /* W25Q32BV */                      \
        { SFUD_MF_ID_WINBOND, 0x40, 0x16,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT | QUAD_OUTPUT |                        \
              QUAD_IO }, /* W25Q64JV */                                        \
        { SFUD_MF_ID_WINBOND, 0x40, 0x17,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT | DUAL_IO | QUAD_OUTPUT |              \
              QUAD_IO }, /* W25Q128JV */                                       \
        { SFUD_MF_ID_WINBOND, 0x40, 0x18,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT | DUAL_IO | QUAD_OUTPUT |              \
              QUAD_IO }, /* W25Q256FV */                                       \
        { SFUD_MF_ID_WINBOND, 0x40, 0x19,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT | DUAL_IO | QUAD_OUTPUT |              \
              QUAD_IO }, /* EN25Q32B */                                        \
        { SFUD_MF_ID_EON, 0x30, 0x16,                                          \
          NORMAL_SPI_READ | DUAL_OUTPUT | QUAD_IO }, /* S25FL216K */           \
        { SFUD_MF_ID_CYPRESS, 0x40, 0x15,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT }, /* A25L080 */                       \
        { SFUD_MF_ID_AMIC, 0x30, 0x14,                                         \
          NORMAL_SPI_READ | DUAL_OUTPUT | DUAL_IO }, /* A25LQ64 */             \
        { SFUD_MF_ID_AMIC, 0x40, 0x17,                                         \
          NORMAL_SPI_READ | DUAL_OUTPUT | DUAL_IO |                            \
              QUAD_IO }, /* MX25L3206E 与 KH25L3206E。 */                      \
        { SFUD_MF_ID_MACRONIX, 0x20, 0x16,                                     \
          NORMAL_SPI_READ | DUAL_OUTPUT }, /* MX25L51245G */                   \
        { SFUD_MF_ID_MACRONIX, 0x20, 0x1A,                                     \
          NORMAL_SPI_READ | DUAL_OUTPUT | DUAL_IO | QUAD_OUTPUT |              \
              QUAD_IO }, /* GD25Q64B */                                        \
        { SFUD_MF_ID_GIGADEVICE, 0x40, 0x17,                                   \
          NORMAL_SPI_READ | DUAL_OUTPUT }, /* NM25Q128EVB */                   \
        { SFUD_MF_ID_NOR_MEM, 0x21, 0x18,                                      \
          NORMAL_SPI_READ | DUAL_OUTPUT | DUAL_IO | QUAD_OUTPUT | QUAD_IO },   \
    }
#endif /* SFUD_USING_QSPI */

/****************************** Defines ******************************/

#ifdef __cplusplus
}
#endif

#endif /* _SFUD_FLASH_DEF_H_ */
