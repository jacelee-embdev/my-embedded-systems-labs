/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file aes.h
 *
 * @par dependencies
 * - stdint.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares AES block encryption and decryption APIs.
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

#ifndef __AES_H__
#define __AES_H__

/****************************** Includes ******************************/
#include <stdint.h>  /* C 标准库：固定宽度整数类型 */
/****************************** Includes ******************************/

/************************ Function Declarations ************************/

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
void AES_Encrypt128(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key);

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
void AES_Decrypt128(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key);

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
void AES_Encrypt192(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key);

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
void AES_Decrypt192(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key);

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
void AES_Encrypt256(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key);

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
void AES_Decrypt256(uint8_t *p_iv, uint8_t *p_state, const uint8_t *p_key);

/************************ Function Declarations ************************/

#endif /* __AES_H__ */
