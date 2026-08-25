/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file bsp_key_config.h
 *
 * @par dependencies
 * - None.
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Provides build-time selection of the key queue transfer mode.
 *
 * Processing flow:
 *
 * Set BSP_KEY_QUEUE_USE_POINTER to 0U to transfer key events by value,
 * or set it to 1U to transfer key-event pointers. The key source files use
 * this setting to compile only the selected queue-transfer implementation.
 *
 * @version V1.0 2026-08-12
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __BSP_KEY_CONFIG_H__
#define __BSP_KEY_CONFIG_H__

/* 0U: queue by value; 1U: queue by pointer. */
#define BSP_KEY_QUEUE_USE_POINTER    (0U)

#if ((BSP_KEY_QUEUE_USE_POINTER != 0U) && \
     (BSP_KEY_QUEUE_USE_POINTER != 1U))
#error "BSP_KEY_QUEUE_USE_POINTER must be 0U or 1U."
#endif

#endif /* __BSP_KEY_CONFIG_H__ */
