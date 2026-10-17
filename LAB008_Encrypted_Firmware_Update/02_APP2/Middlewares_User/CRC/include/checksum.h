/*
 * Library: libcrc
 * File:    include/checksum.h
 * Author:  Lammert Bies
 *
 * This file is licensed under the MIT License as stated below
 *
 * Copyright (c) 1999-2016 Lammert Bies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Description
 * -----------
 * The headerfile include/checksum.h contains the definitions and prototypes
 * for routines that can be used to calculate several kinds of checksums.
 */

#ifndef DEF_LIBCRC_CHECKSUM_H
#define DEF_LIBCRC_CHECKSUM_H

/****************************** Includes ******************************/

#include <stdint.h> /* C 标准库 */

/****************************** Includes ******************************/

/****************************** Defines ******************************/

/* 各 CRC 算法使用的多项式，保留原库采用的正向或反射表示。 */

#define CRC_POLY_16          0xA001
#define CRC_POLY_32          0xEDB88320L
#define CRC_POLY_CCITT       0x1021
#define CRC_POLY_DNP         0xA6BC
#define CRC_POLY_KERMIT      0x8408
#define CRC_POLY_SICK        0x8005

/* 各 CRC 算法开始计算前使用的累计值。 */

#define CRC_START_8          0x00
#define CRC_START_16         0x0000
#define CRC_START_MODBUS     0xFFFF
#define CRC_START_XMODEM     0x0000
#define CRC_START_CCITT_1D0F 0x1D0F
#define CRC_START_CCITT_FFFF 0xFFFF
#define CRC_START_KERMIT     0x0000
#define CRC_START_SICK       0x0000
#define CRC_START_DNP        0x0000
#define CRC_START_32         0xFFFFFFFFL

/****************************** Defines ******************************/

/****************************** Declaring ******************************/

/* 接口使用 size_t，包含本文件前需先引入 <stddef.h> 或 <stdlib.h>。 */

/**
 * @brief 计算 NMEA 语句的异或校验值，输出两位大写十六进制字符串。
 *
 * @param[in] input_str : NMEA 字符串，可包含开头的 '$'。
 * @param[out] result   : 结果缓冲区，至少 3 字节，包含字符串结束符。
 *
 * @return unsigned char * : 成功时返回 result。
 * @retval NULL 任一参数为空，结果缓冲区保持不变。
 *
 * @note 跳过开头的 '$'，遇到 '\r'、'\n'、'*' 或 '\0' 时停止计算。
 * @note 仅计算校验值，不检查 NMEA 语法或核对语句中的已有校验值。
 */
unsigned char *checksum_NMEA(const unsigned char *input_str,
                             unsigned char *result);

/**
 * @brief 使用初始值 0x00，计算 SHT1x / SHT7x 传感器使用的 8 位 CRC。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint8_t : 8 位 CRC 值。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回初始值 0x00。
 */
uint8_t crc_8(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0x0000，一次性计算字节序列的 CRC-16。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint16_t : 16 位 CRC 值。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回初始值 0x0000。
 */
uint16_t crc_16(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0xFFFFFFFF，计算 CRC-32 并执行最终异或。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint32_t : 累计值与 0xFFFFFFFF 异或后的 32 位 CRC。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回 0。
 */
uint32_t crc_32(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0x1D0F，一次性计算字节序列的 CCITT CRC。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint16_t : 16 位 CCITT CRC 值。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回初始值 0x1D0F。
 */
uint16_t crc_ccitt_1d0f(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0xFFFF，一次性计算字节序列的 CCITT CRC。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint16_t : 16 位 CCITT CRC 值。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回初始值 0xFFFF。
 */
uint16_t crc_ccitt_ffff(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0x0000，计算 DNP CRC 并执行取反及高低字节交换。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint16_t : 完成取反和字节交换后的 16 位 DNP CRC。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回 0xFFFF。
 */
uint16_t crc_dnp(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0x0000，计算 Kermit CRC 并交换结果的高低字节。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint16_t : 完成字节交换后的 16 位 Kermit CRC。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回 0x0000。
 */
uint16_t crc_kermit(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0xFFFF，一次性计算字节序列的 Modbus CRC。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint16_t : 16 位 Modbus CRC 值。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回初始值 0xFFFF。
 */
uint16_t crc_modbus(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0x0000，计算 SICK CRC 并交换结果的高低字节。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint16_t : 完成字节交换后的 16 位 SICK CRC。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回 0x0000。
 */
uint16_t crc_sick(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 使用初始值 0x0000，一次性计算字节序列的 X-Modem CRC。
 *
 * @param[in] input_str : 输入字节序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes : 参与计算的字节数。
 *
 * @return uint16_t : 16 位 X-Modem CRC 值。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回初始值 0x0000。
 */
uint16_t crc_xmodem(const unsigned char *input_str, size_t num_bytes);

/**
 * @brief 基于已有 CRC 和一个新字节，更新传感器使用的 8 位 CRC。
 *
 * @param[in] crc : 上一次累计值，首次调用时使用 CRC_START_8。
 * @param[in] c   : 本次参与计算的数据字节。
 *
 * @return uint8_t : 加入当前字节后的 8 位 CRC 值。
 */
uint8_t update_crc_8(uint8_t crc, unsigned char c);

/**
 * @brief 基于已有 CRC 和一个新字节，更新 CRC-16 或 Modbus CRC。
 *
 * @param[in] crc : 上一次累计值，首次使用 CRC_START_16 或
 *                 CRC_START_MODBUS，取决于所需算法。
 * @param[in] c   : 本次参与计算的数据字节。
 *
 * @return uint16_t : 加入当前字节后的 16 位 CRC 值。
 */
uint16_t update_crc_16(uint16_t crc, unsigned char c);

/**
 * @brief 基于已有 CRC 和一个新字节，更新 CRC-32 内部累计值。
 *
 * @param[in] crc : 上一次内部累计值，首次调用时使用 CRC_START_32。
 * @param[in] c   : 本次参与计算的数据字节。
 *
 * @return uint32_t : 尚未执行最终异或的 32 位 CRC 累计值。
 *
 * @note 全部字节处理后，与 0xFFFFFFFF 异或才得到 crc_32() 的结果。
 */
uint32_t update_crc_32(uint32_t crc, unsigned char c);

/**
 * @brief 基于已有 CRC 和一个新字节，更新 CCITT CRC 累计值。
 *
 * @param[in] crc : 上一次累计值，首次调用时传入所需算法的初始值。
 * @param[in] c   : 本次参与计算的数据字节。
 *
 * @return uint16_t : 加入当前字节后的 16 位 CCITT CRC 值。
 *
 * @note 多字节数据按顺序重复调用，并将返回值传入下一次调用。
 * @note 查找表按需初始化，首次初始化不含并发同步保护。
 */
uint16_t update_crc_ccitt(uint16_t crc, unsigned char c);

/**
 * @brief 基于已有 CRC 和一个新字节，更新 DNP CRC 内部累计值。
 *
 * @param[in] crc : 上一次内部累计值，首次调用时使用 CRC_START_DNP。
 * @param[in] c   : 本次参与计算的数据字节。
 *
 * @return uint16_t : 尚未取反及交换高低字节的 16 位 CRC 累计值。
 *
 * @note 全部字节处理后，先取反再交换高低字节，得到 crc_dnp() 的结果。
 */
uint16_t update_crc_dnp(uint16_t crc, unsigned char c);

/**
 * @brief 基于已有 CRC 和一个新字节，更新 Kermit CRC 内部累计值。
 *
 * @param[in] crc : 上一次内部累计值，首次调用时使用 CRC_START_KERMIT。
 * @param[in] c   : 本次参与计算的数据字节。
 *
 * @return uint16_t : 尚未交换高低字节的 16 位 CRC 累计值。
 *
 * @note 全部字节处理后，交换高低字节才得到 crc_kermit() 的结果。
 */
uint16_t update_crc_kermit(uint16_t crc, unsigned char c);

/**
 * @brief 基于已有 CRC、当前字节及前一字节，更新 SICK CRC 累计值。
 *
 * @param[in] crc       : 上一次累计值，首次调用时使用 CRC_START_SICK。
 * @param[in] c         : 本次参与计算的数据字节。
 * @param[in] prev_byte : 前一数据字节，处理首字节时传入 0。
 *
 * @return uint16_t : 尚未交换高低字节的 16 位 CRC 累计值。
 *
 * @note 全部字节处理后，交换高低字节才得到 crc_sick() 的结果。
 */
uint16_t update_crc_sick(uint16_t crc,
                         unsigned char c,
                         unsigned char prev_byte);

/****************************** Declaring ******************************/

#endif /* DEF_LIBCRC_CHECKSUM_H */
