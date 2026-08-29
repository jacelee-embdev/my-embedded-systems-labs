/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * SPDX-License-Identifier: MIT
 *
 * @file bsp_wt588f02b_8s.h
 *
 * @par dependencies
 * - main.h
 * - tim.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the public WT588F02B-8S voice-control APIs.
 *
 * Processing flow:
 *
 * Call wt588f_init() after DMA and TIM2 initialization. Call
 * wt588f_list_play() to send the configured continuous-play sequence through
 * the one-wire interface.
 *
 * @version V1.0 2026-08-29
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __BSP_WT588F02B_8S_H__
#define __BSP_WT588F02B_8S_H__

/****************************** Includes ******************************/

#include "main.h"
#include "tim.h"

/****************************** Includes ******************************/


/****************************** Defines ******************************/

/****************************** Defines ******************************/


/****************************** Declaring ******************************/

/************************ API Declaration ************************/

/**
 * @brief Initializes one-wire communication with the WT588F02B-8S.
 *
 * Registers the DMA transfer-complete callback, enables TIM2 update DMA
 * requests, and waits for the voice chip to complete power-on initialization.
 *
 * @return None.
 *
 * @note Call this function after MX_DMA_Init() and MX_TIM2_Init().
 */
void wt588f_init(void);

/**
 * @brief Sends the configured three-address continuous-play sequence.
 *
 * Sends the F3 continuous-play command with voice addresses 0x00, 0x01, and
 * 0x02 through the one-wire interface.
 *
 * @return None.
 *
 * @note The function waits for every DMA transfer to complete before returning.
 */
void wt588f_list_play(void);

/****************************** Declaring ******************************/

#endif /* End of __BSP_WT588F02B_8S_H__ */
