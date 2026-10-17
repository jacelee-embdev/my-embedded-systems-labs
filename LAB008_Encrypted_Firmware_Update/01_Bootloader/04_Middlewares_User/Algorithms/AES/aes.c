/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file aes.c
 *
 * @par dependencies
 * - aes.h
 * - string.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Implements AES block encryption and decryption.
 *
 * Processing flow:
 *
 * Process one 16-byte block in place with a 128/192/256-bit key.
 * Use an IV for CBC chaining, or NULL for an unchained block.
 *
 * @version V1.0 2026-10-15
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "aes.h"   /* 用户中间件层：AES 接口 */

#include <string.h>  /* C 标准库 */
/****************************** Includes ******************************/

/************************ Variable Definitions ************************/
/* 原始密钥及各轮扩展密钥。 */
static uint8_t g_aes_round_key[256] = {0U};

/* S 盒字节代换表。 */
static const uint8_t g_aes_s_box[16][16] =
{
    {
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
        0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    },
    {
        0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
        0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    },
    {
        0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
        0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    },
    {
        0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
        0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    },
    {
        0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
        0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    },
    {
        0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
        0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    },
    {
        0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
        0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    },
    {
        0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
        0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    },
    {
        0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
        0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    },
    {
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
        0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    },
    {
        0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
        0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    },
    {
        0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
        0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    },
    {
        0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
        0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    },
    {
        0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
        0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    },
    {
        0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
        0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    },
    {
        0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
        0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
    },
};

/* 逆 S 盒字节代换表。 */
static const uint8_t g_aes_inv_s_box[16][16] =
{
    {
        0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38,
        0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    },
    {
        0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87,
        0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    },
    {
        0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d,
        0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    },
    {
        0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2,
        0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    },
    {
        0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16,
        0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    },
    {
        0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda,
        0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    },
    {
        0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a,
        0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    },
    {
        0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02,
        0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    },
    {
        0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea,
        0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    },
    {
        0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85,
        0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    },
    {
        0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89,
        0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    },
    {
        0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20,
        0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    },
    {
        0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31,
        0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    },
    {
        0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d,
        0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    },
    {
        0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0,
        0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    },
    {
        0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26,
        0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d,
    },
};

/* 列混淆矩阵。 */
static const uint8_t g_aes_mix_columns[4][4] =
{
    {
        0x02, 0x03, 0x01, 0x01,
    },
    {
        0x01, 0x02, 0x03, 0x01,
    },
    {
        0x01, 0x01, 0x02, 0x03,
    },
    {
        0x03, 0x01, 0x01, 0x02,
    },
};

/* 逆列混淆矩阵。 */
static const uint8_t g_aes_inv_mix_columns[4][4] =
{
    {
        0x0E, 0x0B, 0x0D, 0x09,
    },
    {
        0x09, 0x0E, 0x0B, 0x0D,
    },
    {
        0x0D, 0x09, 0x0E, 0x0B,
    },
    {
        0x0B, 0x0D, 0x09, 0x0E,
    },
};

/* 密钥扩展轮常数。 */
static const uint8_t g_aes_round_constants[14] =
{
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
    0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d,
};

/************************ Variable Definitions ************************/

/************************ Function Definitions ************************/

/**
 * @brief 使用 S 盒替换数据字节。
 *
 * @param[in,out] p_data 可写数据缓冲区。
 * @param[in] length 替换字节数。
 *
 * @return 无。
 */
static void AES_SubBytes(uint8_t *p_data, uint32_t length)
{
    uint8_t s_box_row = 0U;     /* S 盒行号 */
    uint8_t s_box_column = 0U;  /* S 盒列号 */
    uint8_t index = 0U;        /* 字节索引 */

    for (index = 0; index < length; index++)
    {
        s_box_row = p_data[index] >> 4;  /* 取高 4 位作为 S 盒行索引 */
        s_box_column = p_data[index] & 0x0f;  /* 取低 4 位作为 S 盒列索引 */
        p_data[index] = g_aes_s_box[s_box_row][s_box_column];  /* 查表替换 */
    }
}

/**
 * @brief 使用逆 S 盒还原数据字节。
 *
 * @param[in,out] p_data 可写数据缓冲区。
 * @param[in] length 替换字节数。
 *
 * @return 无。
 */
static void AES_InvSubBytes(uint8_t *p_data, uint32_t length)
{
    uint8_t s_box_row = 0U;     /* 逆 S 盒行号 */
    uint8_t s_box_column = 0U;  /* 逆 S 盒列号 */
    uint8_t index = 0U;        /* 字节索引 */

    for (index = 0; index < length; index++)
    {
        s_box_row = p_data[index] >> 4;  /* 取高 4 位作为逆 S 盒行索引 */
        s_box_column = p_data[index] & 0x0f;  /* 取低 4 位作为逆 S 盒列索引 */
        /* 查表替换 */
        p_data[index] = g_aes_inv_s_box[s_box_row][s_box_column];
    }
}

/**
 * @brief 循环左移状态矩阵的指定行。
 *
 * @param[in,out] p_state 16 字节状态矩阵。
 * @param[in] row 行号，范围为 0～3。
 * @param[in] shift 左移字节数，范围为 1～3。
 *
 * @return 无。
 * @note 行号或移位数无效时直接返回。
 */
static void AES_ShiftRow(uint8_t *p_state, uint8_t row, uint8_t shift)
{
    uint8_t row_data[10] = {0U};  /* 被移出数据的备份缓存 */
    uint8_t index = 0U;  /* 行内字节索引 */

    if ((row > 3U) || (0U == shift) || (shift > 3U))
    {
        return;
    }
    for (index = 0; index < shift; index++)  /* 先备份将被移出该行的数据 */
    {
        row_data[index] = p_state[row + 4 * index];
    }
    for (index = 0; index < (4 - shift); index++)  /* 行内其余数据依次左移 */
    {
        p_state[row + 4 * index] = p_state[row + 4 * (shift + index)];
    }
    /* 备份数据回填到行尾，完成循环移位 */
    for (index = 0; index < shift; index++)
    {
        p_state[row + 4 * ((4 - shift) + index)] = row_data[index];
    }
}

/**
 * @brief 对状态矩阵执行行移位。
 *
 * @param[in,out] p_state 16 字节状态矩阵。
 *
 * @return 无。
 */
static void AES_ShiftRows(uint8_t *p_state)
{
    /* 第 0 行不移位 */
    AES_ShiftRow(p_state, 1, 1);  /* 第 1 行左移 1 个字节 */
    AES_ShiftRow(p_state, 2, 2);  /* 第 2 行左移 2 个字节 */
    AES_ShiftRow(p_state, 3, 3);  /* 第 3 行左移 3 个字节 */
}

/**
 * @brief 对状态矩阵执行逆行移位。
 *
 * @param[in,out] p_state 16 字节状态矩阵。
 *
 * @return 无。
 */
static void AES_InvShiftRows(uint8_t *p_state)
{
    /* 第 0 行不移位 */
    AES_ShiftRow(p_state, 1, 3);  /* 加密左移 1 字节，逆向左移 3 字节 */
    AES_ShiftRow(p_state, 2, 2);  /* 加密左移 2 字节，逆向左移 2 字节 */
    AES_ShiftRow(p_state, 3, 1);  /* 加密左移 3 字节，逆向左移 1 字节 */
}

/**
 * @brief 计算 GF(2^8) 有限域乘积。
 *
 * @param[in] data0 被乘数。
 * @param[in] data1 乘数。
 *
 * @return 有限域乘积。
 */
static uint8_t AES_MultiplyGF256(uint8_t data0, uint8_t data1)
{
    uint8_t value = 0U;  /* 有限域乘积 */
    uint8_t index = 0U;  /* 乘数位索引 */

    for (index = 0; index < 8; index++)
    {
        if (data1 & 0x01)  /* 当前位为 1，累加一次被乘数 */
        {
            value ^= data0;
        }
        /* 被乘数翻倍，溢出时约减 */
        data0 = (data0 << 1) ^ ((data0 & 0x80) ? 0x1b : 0);
        data1 >>= 1;  /* 扫描下一位 */
    }
    return value;
}

/**
 * @brief 对状态矩阵执行列混淆。
 *
 * @param[in,out] p_state 16 字节状态矩阵。
 *
 * @return 无。
 */
static void AES_MixColumns(uint8_t *p_state)
{
    uint8_t column_data[4] = {0U};  /* 一列数据的备份 */
    uint8_t mixed_column[4] = {0U};  /* 混淆计算后的结果 */
    uint8_t column_index = 0U;  /* 列计数 */

    /* 依次处理 4 列 */
    for (column_index = 0; column_index < 4; column_index++)
    {
        /* 备份当前列数据 */
        memcpy(column_data, &p_state[column_index * 4], 4);

        /* 按列进行有限域乘加。 */
        mixed_column[0] =
            AES_MultiplyGF256(column_data[0], g_aes_mix_columns[0][0]) ^
            AES_MultiplyGF256(column_data[1], g_aes_mix_columns[0][1]) ^
            AES_MultiplyGF256(column_data[2], g_aes_mix_columns[0][2]) ^
            AES_MultiplyGF256(column_data[3], g_aes_mix_columns[0][3]);

        mixed_column[1] =
            AES_MultiplyGF256(column_data[0], g_aes_mix_columns[1][0]) ^
            AES_MultiplyGF256(column_data[1], g_aes_mix_columns[1][1]) ^
            AES_MultiplyGF256(column_data[2], g_aes_mix_columns[1][2]) ^
            AES_MultiplyGF256(column_data[3], g_aes_mix_columns[1][3]);

        mixed_column[2] =
            AES_MultiplyGF256(column_data[0], g_aes_mix_columns[2][0]) ^
            AES_MultiplyGF256(column_data[1], g_aes_mix_columns[2][1]) ^
            AES_MultiplyGF256(column_data[2], g_aes_mix_columns[2][2]) ^
            AES_MultiplyGF256(column_data[3], g_aes_mix_columns[2][3]);

        mixed_column[3] =
            AES_MultiplyGF256(column_data[0], g_aes_mix_columns[3][0]) ^
            AES_MultiplyGF256(column_data[1], g_aes_mix_columns[3][1]) ^
            AES_MultiplyGF256(column_data[2], g_aes_mix_columns[3][2]) ^
            AES_MultiplyGF256(column_data[3], g_aes_mix_columns[3][3]);

        memcpy(&p_state[column_index * 4], mixed_column, 4);  /* 结果写回该列 */
    }
}

/**
 * @brief 对状态矩阵执行逆列混淆。
 *
 * @param[in,out] p_state 16 字节状态矩阵。
 *
 * @return 无。
 */
static void AES_InvMixColumns(uint8_t *p_state)
{
    uint8_t column_data[4] = {0U};  /* 一列数据的备份 */
    uint8_t mixed_column[4] = {0U};  /* 混淆计算后的结果 */
    uint8_t column_index = 0U;  /* 列计数 */

    /* 依次处理 4 列 */
    for (column_index = 0; column_index < 4; column_index++)
    {
        /* 备份当前列数据 */
        memcpy(column_data, &p_state[column_index * 4], 4);

        /* 按列进行有限域乘加。 */
        mixed_column[0] =
            AES_MultiplyGF256(column_data[0], g_aes_inv_mix_columns[0][0]) ^
            AES_MultiplyGF256(column_data[1], g_aes_inv_mix_columns[0][1]) ^
            AES_MultiplyGF256(column_data[2], g_aes_inv_mix_columns[0][2]) ^
            AES_MultiplyGF256(column_data[3], g_aes_inv_mix_columns[0][3]);

        mixed_column[1] =
            AES_MultiplyGF256(column_data[0], g_aes_inv_mix_columns[1][0]) ^
            AES_MultiplyGF256(column_data[1], g_aes_inv_mix_columns[1][1]) ^
            AES_MultiplyGF256(column_data[2], g_aes_inv_mix_columns[1][2]) ^
            AES_MultiplyGF256(column_data[3], g_aes_inv_mix_columns[1][3]);

        mixed_column[2] =
            AES_MultiplyGF256(column_data[0], g_aes_inv_mix_columns[2][0]) ^
            AES_MultiplyGF256(column_data[1], g_aes_inv_mix_columns[2][1]) ^
            AES_MultiplyGF256(column_data[2], g_aes_inv_mix_columns[2][2]) ^
            AES_MultiplyGF256(column_data[3], g_aes_inv_mix_columns[2][3]);

        mixed_column[3] =
            AES_MultiplyGF256(column_data[0], g_aes_inv_mix_columns[3][0]) ^
            AES_MultiplyGF256(column_data[1], g_aes_inv_mix_columns[3][1]) ^
            AES_MultiplyGF256(column_data[2], g_aes_inv_mix_columns[3][2]) ^
            AES_MultiplyGF256(column_data[3], g_aes_inv_mix_columns[3][3]);

        memcpy(&p_state[column_index * 4], mixed_column, 4);  /* 结果写回该列 */
    }
}

/**
 * @brief 旋转密钥字并执行字节代换和轮常数异或。
 *
 * @param[in,out] p_word 4 字节密钥字。
 * @param[in] round_index 轮常数索引，范围为 0～13。
 *
 * @return 无。
 */
static void AES_TransformKeyWord(uint8_t *p_word, uint8_t round_index)
{
    uint8_t temp = 0U;  /* 旋转暂存字节 */

    temp = p_word[0];  /* 循环左移 1 个字节 */
    memmove(p_word, &p_word[1], 3);
    p_word[3] = temp;
    AES_SubBytes(p_word, 4);  /* S 盒替换 */

    p_word[0] ^= g_aes_round_constants[round_index];  /* 与轮常数异或 */
    p_word[1] ^= 0x00;  /* 其余字节异或 0x00，保持不变 */
    p_word[2] ^= 0x00;
    p_word[3] ^= 0x00;
}

/**
 * @brief 将源数据按字节异或到目标缓冲区。
 *
 * @param[in,out] p_destination 接收异或结果的缓冲区。
 * @param[in] p_source 只读源缓冲区。
 * @param[in] length 异或字节数。
 *
 * @return 无。
 */
static void AES_XorBytes(uint8_t *p_destination,
                         const uint8_t *p_source,
                         uint32_t length)
{
    while (length--)
    {
        *p_destination ^= *p_source;
        p_destination++;
        p_source++;
    }
}

/**
 * @brief 在密钥缓冲区中追加各轮扩展密钥。
 *
 * @param[in,out] p_expanded_key 原始密钥及扩展结果缓冲区。
 * @param[in] key_size 原始密钥字节数，仅支持 16/24/32。
 *
 * @return 无。
 * @note 缓冲区容量分别至少为 176/208/240 字节。
 */
static void AES_ExpandKey(uint8_t *p_expanded_key, uint8_t key_size)
{
    uint8_t word_data[4] = {0U};      /* 当前密钥字 */
    uint8_t previous_word[4] = {0U};  /* 前一扩展字 */
    uint8_t word_index = 0U;          /* 扩展字索引 */
    uint8_t round_count = 0U;  /* 轮数：10/12/14 */

    if (16U == key_size)  /* 16 字节密钥对应 10 轮 */
    {
        round_count = 10;
    }
    else if (24U == key_size)  /* 24 字节密钥对应 12 轮 */
    {
        round_count = 12;
    }
    else if (32U == key_size)  /* 32 字节密钥对应 14 轮 */
    {
        round_count = 14;
    }
    else  /* 非法密钥长度，直接返回 */
    {
        return;
    }

    /* 取原始密钥最后一列 */
    memcpy(previous_word, &p_expanded_key[key_size - 4], 4);

    /* 逐字扩展，原始密钥占用的字不重复生成。 */
    for (word_index = 0;
         word_index < (round_count * 4 - ((key_size - 16) / 8) * 2);
         word_index++)
    {
        /* 每隔 Nk 个字执行密钥字变换。 */
        if (0U == (word_index % (key_size / 4)))
        {
            /* 结果保存在原数组中 */
            AES_TransformKeyWord(previous_word, word_index / (key_size / 4));
        }
        else
        {
            /* AES-256 在间隔半组时额外执行字节代换。 */
            if ((4U == (word_index % (key_size / 4))) && (32U == key_size))
            {
                AES_SubBytes(previous_word, 4);  /* S 盒替换 */
            }
        }
        /* 取对应列的数据 */
        memcpy(word_data, &p_expanded_key[word_index * 4], 4);
        AES_XorBytes(previous_word, word_data, 4);  /* 两列数据异或 */
        /* 写回扩展结果 */
        memcpy(&p_expanded_key[key_size + word_index * 4], previous_word, 4);
    }
}

/**
 * @brief 复制原始密钥并生成各轮密钥。
 *
 * @param[in] p_key 只读原始密钥。
 * @param[in] key_size 密钥字节数，仅支持 16/24/32。
 *
 * @return 无。
 * @note 轮密钥保存到模块静态缓存，不可重入。
 */
static void AES_CreateRoundKeys(const uint8_t *p_key, uint8_t key_size)
{
    memcpy(g_aes_round_key, p_key, key_size);  /* 原始密钥拷贝到全局缓存 */
    AES_ExpandKey(g_aes_round_key, key_size);  /* 扩展产生各轮子密钥 */
}

/**
 * @brief 使用 128 位密钥加密一个数据块。
 *
 * @param[in,out] p_iv 16 字节链接向量，NULL 表示不链接。
 * @param[in,out] p_state 16 字节明文输入、密文输出。
 * @param[in] p_key 16 字节只读密钥。
 *
 * @return 无。
 * @note 状态或密钥为空时直接返回。
 * @note 链接时以本块密文更新 IV。
 * @note 缓冲区不得重叠；不提供填充，不可重入。
 */
void AES_Encrypt128(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key)
{
    uint8_t round_index = 0U;  /* 当前轮次 */

    if ((NULL == p_key) || (NULL == p_state))
    {
        return;
    }
    if (NULL != p_iv)  /* 有链接向量时，明文先与向量异或 */
    {
        AES_XorBytes(p_state, p_iv, 16);
    }

    AES_CreateRoundKeys(p_key, 16);  /* 生成轮密钥：共 11 组 x 16 字节 */
    AES_XorBytes(p_state, &g_aes_round_key[0], 16);  /* 第 0 轮密钥与明文异或 */

    for (round_index = 1; round_index < 10; round_index++)  /* 处理中间轮。 */
    {
        AES_SubBytes(p_state, 16);  /* 字节替代 */
        AES_ShiftRows(p_state);  /* 行移位 */
        AES_MixColumns(p_state);  /* 列混淆 */
        /* 与轮密钥异或 */
        AES_XorBytes(p_state, &g_aes_round_key[round_index * 16], 16);
    }

    /* 第 10 轮：无列混淆 */
    AES_SubBytes(p_state, 16);  /* 字节替代 */
    AES_ShiftRows(p_state);  /* 行移位 */
    /* 异或轮密钥 */
    AES_XorBytes(p_state, &g_aes_round_key[16 * round_index], 16);

    if (NULL != p_iv)  /* 密文拷贝到向量缓存，供下一块链接 */
    {
        memcpy(p_iv, p_state, 16);
    }
}

/**
 * @brief 使用 128 位密钥解密一个数据块。
 *
 * @param[in,out] p_iv 16 字节链接向量，NULL 表示不链接。
 * @param[in,out] p_state 16 字节密文输入、明文输出。
 * @param[in] p_key 16 字节只读密钥。
 *
 * @return 无。
 * @note 状态或密钥为空时直接返回。
 * @note 链接时以输入密文更新 IV。
 * @note 缓冲区不得重叠；不提供填充，不可重入。
 */
void AES_Decrypt128(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key)
{
    uint8_t round_index = 0U;  /* 当前轮次 */
    uint8_t ciphertext_block[16] = {0U};  /* 原始密文备份 */

    if ((NULL == p_key) || (NULL == p_state))
    {
        return;
    }
    if (NULL != p_iv)
    {
        memcpy(ciphertext_block, p_state, 16);  /* 备份原始密文 */
    }

    AES_CreateRoundKeys(p_key, 16);  /* 生成轮密钥：共 11 组 x 16 字节 */
    /* 第 10 轮密钥与密文异或 */
    AES_XorBytes(p_state, &g_aes_round_key[10 * 16], 16);
    AES_InvShiftRows(p_state);  /* 逆向行移位 */
    AES_InvSubBytes(p_state, 16);  /* 逆向字节替代 */

    for (round_index = 9; round_index > 0; round_index--)  /* 处理中间轮。 */
    {
        /* 与轮密钥异或 */
        AES_XorBytes(p_state, &g_aes_round_key[round_index * 16], 16);
        AES_InvMixColumns(p_state);  /* 逆向列混淆 */
        AES_InvShiftRows(p_state);  /* 逆向行移位 */
        AES_InvSubBytes(p_state, 16);  /* 逆向字节替代 */
    }

    /* 最后一轮：异或第 0 轮密钥（即原始密钥） */
    AES_XorBytes(p_state, &g_aes_round_key[16 * round_index], 16);

    if (NULL != p_iv)
    {
        AES_XorBytes(p_state, p_iv, 16);  /* 解密结果与向量异或 */
        /* 原始密文拷回向量缓存，供下一块链接 */
        memcpy(p_iv, ciphertext_block, 16);
    }
}

/**
 * @brief 使用 192 位密钥加密一个数据块。
 *
 * @param[in,out] p_iv 16 字节链接向量，NULL 表示不链接。
 * @param[in,out] p_state 16 字节明文输入、密文输出。
 * @param[in] p_key 24 字节只读密钥。
 *
 * @return 无。
 * @note 状态或密钥为空时直接返回。
 * @note 链接时以本块密文更新 IV。
 * @note 缓冲区不得重叠；不提供填充，不可重入。
 */
void AES_Encrypt192(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key)
{
    uint8_t round_index = 0U;  /* 当前轮次 */

    if ((NULL == p_key) || (NULL == p_state))
    {
        return;
    }
    if (NULL != p_iv)  /* 有链接向量时，明文先与向量异或 */
    {
        AES_XorBytes(p_state, p_iv, 16);
    }

    AES_CreateRoundKeys(p_key, 24);  /* 生成轮密钥：共 13 组 x 16 字节 */
    AES_XorBytes(p_state, &g_aes_round_key[0], 16);  /* 第 0 轮密钥与明文异或 */

    for (round_index = 1; round_index < 12; round_index++)  /* 处理中间轮。 */
    {
        AES_SubBytes(p_state, 16);  /* 字节替代 */
        AES_ShiftRows(p_state);  /* 行移位 */
        AES_MixColumns(p_state);  /* 列混淆 */
        /* 与轮密钥异或 */
        AES_XorBytes(p_state, &g_aes_round_key[round_index * 16], 16);
    }

    /* 第 12 轮：无列混淆 */
    AES_SubBytes(p_state, 16);  /* 字节替代 */
    AES_ShiftRows(p_state);  /* 行移位 */
    /* 异或轮密钥 */
    AES_XorBytes(p_state, &g_aes_round_key[round_index * 16], 16);

    if (NULL != p_iv)  /* 密文拷贝到向量缓存，供下一块链接 */
    {
        memcpy(p_iv, p_state, 16);
    }
}

/**
 * @brief 使用 192 位密钥解密一个数据块。
 *
 * @param[in,out] p_iv 16 字节链接向量，NULL 表示不链接。
 * @param[in,out] p_state 16 字节密文输入、明文输出。
 * @param[in] p_key 24 字节只读密钥。
 *
 * @return 无。
 * @note 状态或密钥为空时直接返回。
 * @note 链接时以输入密文更新 IV。
 * @note 缓冲区不得重叠；不提供填充，不可重入。
 */
void AES_Decrypt192(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key)
{
    uint8_t round_index = 0U;  /* 当前轮次 */
    uint8_t ciphertext_block[16] = {0U};  /* 原始密文备份 */

    if ((NULL == p_key) || (NULL == p_state))
    {
        return;
    }
    if (NULL != p_iv)
    {
        memcpy(ciphertext_block, p_state, 16);  /* 备份原始密文 */
    }

    AES_CreateRoundKeys(p_key, 24);  /* 生成轮密钥：共 13 组 x 16 字节 */
    /* 第 12 轮密钥与密文异或 */
    AES_XorBytes(p_state, &g_aes_round_key[12 * 16], 16);
    AES_InvShiftRows(p_state);  /* 逆向行移位 */
    AES_InvSubBytes(p_state, 16);  /* 逆向字节替代 */

    for (round_index = 11; round_index > 0; round_index--)  /* 处理中间轮。 */
    {
        /* 与轮密钥异或 */
        AES_XorBytes(p_state, &g_aes_round_key[round_index * 16], 16);
        AES_InvMixColumns(p_state);  /* 逆向列混淆 */
        AES_InvShiftRows(p_state);  /* 逆向行移位 */
        AES_InvSubBytes(p_state, 16);  /* 逆向字节替代 */
    }

    /* 最后一轮：异或第 0 轮密钥（即原始密钥） */
    AES_XorBytes(p_state, &g_aes_round_key[16 * round_index], 16);

    if (NULL != p_iv)
    {
        AES_XorBytes(p_state, p_iv, 16);  /* 解密结果与向量异或 */
        /* 原始密文拷回向量缓存，供下一块链接 */
        memcpy(p_iv, ciphertext_block, 16);
    }
}

/**
 * @brief 使用 256 位密钥加密一个数据块。
 *
 * @param[in,out] p_iv 16 字节链接向量，NULL 表示不链接。
 * @param[in,out] p_state 16 字节明文输入、密文输出。
 * @param[in] p_key 32 字节只读密钥。
 *
 * @return 无。
 * @note 状态或密钥为空时直接返回。
 * @note 链接时以本块密文更新 IV。
 * @note 缓冲区不得重叠；不提供填充，不可重入。
 */
void AES_Encrypt256(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key)
{
    uint8_t round_index = 0U;  /* 当前轮次 */

    if ((NULL == p_key) || (NULL == p_state))
    {
        return;
    }
    if (NULL != p_iv)  /* 有链接向量时，明文先与向量异或 */
    {
        AES_XorBytes(p_state, p_iv, 16);
    }

    AES_CreateRoundKeys(p_key, 32);  /* 生成轮密钥：共 15 组 x 16 字节 */
    AES_XorBytes(p_state, &g_aes_round_key[0], 16);  /* 第 0 轮密钥与明文异或 */

    for (round_index = 1; round_index < 14; round_index++)  /* 处理中间轮。 */
    {
        AES_SubBytes(p_state, 16);  /* 字节替代 */
        AES_ShiftRows(p_state);  /* 行移位 */
        AES_MixColumns(p_state);  /* 列混淆 */
        /* 与轮密钥异或 */
        AES_XorBytes(p_state, &g_aes_round_key[round_index * 16], 16);
    }

    /* 第 14 轮：无列混淆 */
    AES_SubBytes(p_state, 16);  /* 字节替代 */
    AES_ShiftRows(p_state);  /* 行移位 */
    /* 异或轮密钥 */
    AES_XorBytes(p_state, &g_aes_round_key[round_index * 16], 16);

    if (NULL != p_iv)  /* 密文拷贝到向量缓存，供下一块链接 */
    {
        memcpy(p_iv, p_state, 16);
    }
}

/**
 * @brief 使用 256 位密钥解密一个数据块。
 *
 * @param[in,out] p_iv 16 字节链接向量，NULL 表示不链接。
 * @param[in,out] p_state 16 字节密文输入、明文输出。
 * @param[in] p_key 32 字节只读密钥。
 *
 * @return 无。
 * @note 状态或密钥为空时直接返回。
 * @note 链接时以输入密文更新 IV。
 * @note 缓冲区不得重叠；不提供填充，不可重入。
 */
void AES_Decrypt256(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key)
{
    uint8_t round_index = 0U;  /* 当前轮次 */
    uint8_t ciphertext_block[16] = {0U};  /* 原始密文备份 */

    if ((NULL == p_key) || (NULL == p_state))
    {
        return;
    }
    if (NULL != p_iv)
    {
        memcpy(ciphertext_block, p_state, 16);  /* 备份原始密文 */
    }

    AES_CreateRoundKeys(p_key, 32);  /* 生成轮密钥：共 15 组 x 16 字节 */
    /* 第 14 轮密钥与密文异或 */
    AES_XorBytes(p_state, &g_aes_round_key[14 * 16], 16);
    AES_InvShiftRows(p_state);  /* 逆向行移位 */
    AES_InvSubBytes(p_state, 16);  /* 逆向字节替代 */

    for (round_index = 13; round_index > 0; round_index--)  /* 处理中间轮。 */
    {
        /* 与轮密钥异或 */
        AES_XorBytes(p_state, &g_aes_round_key[round_index * 16], 16);
        AES_InvMixColumns(p_state);  /* 逆向列混淆 */
        AES_InvShiftRows(p_state);  /* 逆向行移位 */
        AES_InvSubBytes(p_state, 16);  /* 逆向字节替代 */
    }

    /* 最后一轮：异或第 0 轮密钥（即原始密钥） */
    AES_XorBytes(p_state, &g_aes_round_key[16 * round_index], 16);

    if (NULL != p_iv)
    {
        AES_XorBytes(p_state, p_iv, 16);  /* 解密结果与向量异或 */
        /* 原始密文拷回向量缓存，供下一块链接 */
        memcpy(p_iv, ciphertext_block, 16);
    }
}

/************************ Function Definitions ************************/
