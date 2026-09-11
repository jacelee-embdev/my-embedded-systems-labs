/******************************************************************************
 * Copyright (C) 2026 JesMicro.
 *
 * All Rights Reserved.
 *
 * @file circular_buffer.h
 *
 * @par dependencies
 * - stdlib.h
 * - stdint.h
 * - stdio.h
 * - string.h
 * - elog.h
 *
 * @author Jace | Development Dept. | JesMicro
 *
 * @brief Declares circular-buffer data types and operation interfaces.
 *
 * Processing flow:
 *
 * Call create_empty_circular_buf() to allocate an empty buffer. Use
 * buf_write_data() and buf_read_data() to store and retrieve data, and call
 * check_buf_status() when the current buffer state is required.
 *
 * @version V1.3 2026-09-11
 *
 * @note 1 tab == 4 spaces!
 *
 ******************************************************************************/

#ifndef __CIRCULAR_BUFFER_H__
#define __CIRCULAR_BUFFER_H__

/****************************** Includes ******************************/
#include <stdlib.h>             // The Compiler Library
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "elog.h"               // Middleware layer
/****************************** Includes ******************************/

/****************************** Defines *******************************/
#define BUFFER_SIZE    (108U)

typedef uint8_t data_type_t;

typedef struct
{
    uint8_t data[BUFFER_SIZE];
    /* Points to the position where the next data will be written. */
    uint32_t head;
    /* Points to the position where the next data will be read. */
    uint32_t tail;
} circular_buf_t;

typedef enum
{
    BUFFER_NULL   = 0,  /* The buffer pointer is NULL. */
    BUFFER_NORMAL = 1,  /* The buffer can be read and written. */
    BUFFER_EMPTY  = 2,  /* The buffer contains no data. */
    BUFFER_FULL   = 3,  /* The buffer has no writable position. */
} buf_status_t;

typedef enum
{
    BUFFER_WR_OK    = 0,  /* The operation completed successfully. */
    BUFFER_WR_ERROR = 1,  /* The operation failed. */
} buf_wr_status_t;
/****************************** Defines *******************************/

/************************ Function Declarations ************************/

/**
 * @brief Creates an empty circular buffer.
 *
 * Allocates memory for a circular-buffer object and initializes all fields
 * to zero.
 *
 * @return circular_buf_t * Pointer to the created circular buffer.
 * @retval NULL Memory allocation failed.
 */
circular_buf_t *create_empty_circular_buf(void);

/**
 * @brief Checks the current state of a circular buffer.
 *
 * @param[in] p_bufer Pointer to the circular buffer to be checked.
 *
 * @return buf_status_t Current state of the circular buffer.
 * @retval BUFFER_NULL   The buffer pointer is NULL.
 * @retval BUFFER_NORMAL The buffer is neither empty nor full.
 * @retval BUFFER_EMPTY  The buffer contains no data.
 * @retval BUFFER_FULL   The buffer has no writable position.
 */
buf_status_t check_buf_status(circular_buf_t *p_bufer);

/**
 * @brief Writes one data item to a circular buffer.
 *
 * @param[in,out] p_bufer Pointer to the destination circular buffer.
 * @param[in] data Data item to be written.
 *
 * @return buf_wr_status_t Result of the write operation.
 * @retval BUFFER_WR_OK    The data was written successfully.
 * @retval BUFFER_WR_ERROR The buffer pointer is NULL or the buffer is full.
 */
buf_wr_status_t buf_write_data(circular_buf_t *p_bufer, data_type_t data);

/**
 * @brief Reads one data item from a circular buffer.
 *
 * @param[in,out] p_bufer Pointer to the source circular buffer.
 * @param[out] p_data Pointer to the variable that receives the data item.
 *
 * @return buf_wr_status_t Result of the read operation.
 * @retval BUFFER_WR_OK    The data was read successfully.
 * @retval BUFFER_WR_ERROR An input pointer is NULL or the buffer is empty.
 */
buf_wr_status_t buf_read_data(circular_buf_t *p_bufer,
                              data_type_t *p_data);

/************************ Function Declarations ************************/

#endif /* __CIRCULAR_BUFFER_H__ */
