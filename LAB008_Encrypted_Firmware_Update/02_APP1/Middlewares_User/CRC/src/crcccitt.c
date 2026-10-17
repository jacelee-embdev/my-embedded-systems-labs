/*
 * Library: libcrc
 * File:    src/crcccitt.c
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
 * The module src/crcccitt.c contains routines which are used to calculate the
 * CCITT CRC values of a string of bytes.
 */

/****************************** Includes ******************************/

#include <stdbool.h>  /* C 标准库 */
#include <stdlib.h>

#include "checksum.h" /* 中间件层：libcrc 校验接口 */

/****************************** Includes ******************************/

/****************************** Declaring ******************************/

static uint16_t crc_ccitt_generic(const unsigned char *input_str,
                                  size_t num_bytes,
                                  uint16_t start_value);
static void init_crcccitt_tab(void);

/****************************** Declaring ******************************/

/************************ Variable Definitions ************************/

/* CCITT 查找表初始化完成标志。 */
static bool crc_tabccitt_init = false;

/* CCITT 字节查找表，共 256 项，首次计算时生成。 */
static uint16_t crc_tabccitt[256];

/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/

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
uint16_t crc_xmodem(const unsigned char *input_str, size_t num_bytes)
{
    return crc_ccitt_generic(input_str, num_bytes, CRC_START_XMODEM);
}

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
uint16_t crc_ccitt_1d0f(const unsigned char *input_str, size_t num_bytes)
{
    return crc_ccitt_generic(input_str, num_bytes, CRC_START_CCITT_1D0F);
}

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
uint16_t crc_ccitt_ffff(const unsigned char *input_str, size_t num_bytes)
{
    return crc_ccitt_generic(input_str, num_bytes, CRC_START_CCITT_FFFF);
}

/**
 * @brief 使用指定初始值，通过查表一次性计算字节序列的 CCITT CRC。
 *
 * 处理步骤：
 * 1. 首次调用时生成 CCITT 查找表。
 * 2. 使用指定初始值，逐字节查表并更新 CRC。
 * 3. 返回累计结果，不执行最终异或或字节交换。
 *
 * @param[in] input_str   : 输入序列，非空时至少包含 num_bytes 个字节。
 * @param[in] num_bytes   : 参与计算的字节数。
 * @param[in] start_value : 当前算法使用的 CRC 初始值。
 *
 * @return uint16_t : 16 位 CCITT CRC 值。
 *
 * @note input_str 为 NULL 或 num_bytes 为 0 时返回 start_value。
 * @note 查找表按需初始化，首次初始化不含并发同步保护。
 */
static uint16_t crc_ccitt_generic(const unsigned char *input_str,
                                  size_t num_bytes,
                                  uint16_t start_value)
{
    uint16_t crc;             /* 当前累计的 CRC 值 */
    uint16_t tmp;             /* CRC 高字节与输入字节异或后的查表索引 */
    uint16_t short_c;         /* 扩展至 16 位的当前输入字节 */
    const unsigned char *ptr; /* 当前读取位置 */
    size_t a;                 /* 已处理的字节计数 */

    /* 1. 首次计算前生成查找表，后续调用复用。 */
    if (!crc_tabccitt_init)
        init_crcccitt_tab();

    /* 2. 设置初始值；空指针或零长度输入不进入计算循环。 */
    crc = start_value;
    ptr = input_str;

    if (ptr != NULL)
        for (a = 0; a < num_bytes; a++)
        {
            /* 以 CRC 高字节和当前输入字节查表，更新 16 位 CRC。 */
            short_c = 0x00ff & (unsigned short)*ptr;
            tmp     = (crc >> 8) ^ short_c;
            crc     = (crc << 8) ^ crc_tabccitt[tmp];

            ptr++;
        }

    return crc;
}

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
uint16_t update_crc_ccitt(uint16_t crc, unsigned char c)
{
    int16_t tmp;              /* CRC 高字节与输入字节异或后的查表索引 */
    int16_t short_c;          /* 扩展后的当前输入字节 */

    short_c = 0x00ff & (uint16_t)c;

    /* 首次更新前生成查找表。 */
    if (!crc_tabccitt_init)
        init_crcccitt_tab();

    /* 将新字节合入已有累计值。 */
    tmp = (crc >> 8) ^ short_c;
    crc = (crc << 8) ^ crc_tabccitt[tmp];

    return crc;
}

/**
 * @brief 使用 CCITT 多项式 0x1021 生成 256 项字节查找表。
 *
 * 处理步骤：
 * 1. 遍历所有字节取值，分别执行 8 轮移位和条件异或。
 * 2. 将结果写入查找表，全部完成后设置初始化标志。
 *
 * @return 无返回值。
 *
 * @note 由 CRC 计算和更新函数在首次使用查找表时调用。
 */
static void init_crcccitt_tab(void)
{
    uint16_t i;               /* 查找表索引，对应一个字节的取值 */
    uint16_t j;               /* 当前字节内的位循环计数 */
    uint16_t crc;             /* 当前表项的 CRC 累计值 */
    uint16_t c;               /* 左对齐后逐位移入计算的字节值 */

    /* 1. 为每个字节取值计算对应的 CRC 表项。 */
    for (i = 0; i < 256; i++)
    {
        crc = 0;
        c   = i << 8;

        for (j = 0; j < 8; j++)
        {
            /* 最高位异或结果为 1 时，移位后再异或 CCITT 多项式。 */
            if ((crc ^ c) & 0x8000)
                crc = (crc << 1) ^ CRC_POLY_CCITT;
            else
                crc = crc << 1;

            c = c << 1;
        }

        crc_tabccitt[i] = crc;
    }

    /* 2. 所有表项写入完成后，标记查找表可用。 */
    crc_tabccitt_init = true;
}

/************************ Function Definitions ************************/
