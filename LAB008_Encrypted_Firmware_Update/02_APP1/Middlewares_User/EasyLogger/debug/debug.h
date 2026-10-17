/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file debug.h
 *
 * @par dependencies
 * - elog.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares the EasyLogger debug-module initialization interface.
 *
 * Processing flow:
 *
 * Call debug_init() once during application startup before outputting any
 * EasyLogger messages.
 *
 * @version v1.0 2026-09-17
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __DEBUG_H
#define __DEBUG_H

/****************************** Includes ******************************/
#include "elog.h"
/****************************** Includes ******************************/

/****************************** Declaring ******************************/

/**
 * @brief Initializes and starts the EasyLogger debug module.
 *
 * Initializes the EasyLogger core, enables colored output, configures the log
 * formats, and enables synchronous log output.
 *
 * @return None.
 */
void debug_init(void);

/****************************** Declaring ******************************/

#endif /* End of __DEBUG_H */
