/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file debug.c
 *
 * @par dependencies
 * - debug.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Provides EasyLogger initialization and log-format configuration.
 *
 * Processing flow:
 *
 * Initialize EasyLogger, enable colored output, configure the selected log
 * format for every severity level, and start synchronous log output.
 *
 * @version v1.0 2026-09-17
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

/****************************** Includes ******************************/
#include "debug.h"
/****************************** Includes ******************************/

/**
 * @brief Initializes and starts the EasyLogger debug module.
 *
 * Steps:
 * 1. Initializes the EasyLogger core and its platform port.
 * 2. Enables colored log output.
 * 3. Configures the selected format for every log severity level.
 * 4. Starts synchronous EasyLogger output.
 *
 * @return None.
 */
void debug_init(void)
{
    /* Step 1: Initialize the EasyLogger core and platform port. */
    elog_init();
    
    /* Step 2: Explicitly enable the compiled-in color output feature. */
    elog_set_text_color_enabled(true);

#if 0 
    /* Step 3: Configure the optional compact log format. */
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_DEBUG,  ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_ERROR,  ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_INFO,   ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_WARN,   ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG);
    elog_set_fmt(ELOG_LVL_VERBOSE,ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG);
#endif 

#if 1 
    /* Step 3: Configure the active detailed log format. */
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG |
                                  ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_DEBUG,  ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG |
                                  ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_ERROR,  ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG |
                                  ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_INFO,   ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG |
                                  ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_WARN,   ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG |
                                  ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC);
    elog_set_fmt(ELOG_LVL_VERBOSE,ELOG_FMT_TIME | ELOG_FMT_LVL | ELOG_FMT_TAG |
                                  ELOG_FMT_DIR | ELOG_FMT_LINE | ELOG_FMT_FUNC);

#endif
    /* Step 4: Enable EasyLogger output after configuration completes. */
    elog_start();
}
