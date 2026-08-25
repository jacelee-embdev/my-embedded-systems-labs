/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "usart.h"
#include "bsp_key_config.h"

#if (BSP_KEY_QUEUE_USE_POINTER == 0U)
#include "bsp_key_queue_value.h"
#elif (BSP_KEY_QUEUE_USE_POINTER == 1U)
#include "bsp_key_queue_pointer.h"
#else
#error "BSP_KEY_QUEUE_USE_POINTER must be 0U or 1U."
#endif

#include "bsp_led.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */


/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
    if (pdPASS != key_init())
    {
    Error_Handler();
    }

    if (pdPASS != led_init())
    {
        Error_Handler();
    }
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
    key_task_handle = osThreadNew(key_task_entry,
                                  NULL,
                                  &key_task_attributes);
    led_task_handle = osThreadNew(led_task_entry,
                                  NULL,
                                  &led_task_attributes);
                                  
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */

  /* Local variables. */
  key_event_t received_key_event    = KEY_EVENT_NONE;
  led_command_t led_command         = LED_CMD_NONE;
    
  /* Task loop. */
  for (;;)
    {
        // printf("DFT thread test\r\n");

        // 0. Wait until both queues are ready.
        if ((NULL != g_key_queue) && (NULL != g_led_queue))
        {
            // 1. Receive a key event from the key task.
            if (pdPASS == xQueueReceive(g_key_queue,
                                        &received_key_event,
                                        portMAX_DELAY))
            {
                switch (received_key_event)
                {
                    // 2. Map a short press to the LED toggle command.
                    case KEY_EVENT_SHORT_PRESS:
                    {
                        led_command = LED_CMD_TOGGLE;

                        // 2.1 Forward the LED command to the LED task.
                        if (pdPASS != xQueueSend(g_led_queue,
                                                &led_command,
                                                0))
                        {
                            printf("[E][RELAY] LED command send failed.\r\n");
                        }
                        break;
                    }

                    // 3. Map a long press to the three-blink LED command.
                    case KEY_EVENT_LONG_PRESS:
                    {
                        led_command = LED_CMD_BLINK_3_TIMES;

                        // 3.1 Forward the LED command to the LED task.
                        if (pdPASS != xQueueSend(g_led_queue,
                                                &led_command,
                                                0))
                        {
                            printf("[E][RELAY] LED command send failed.\r\n");
                        }
                        break;
                    }

                    // 4. Report an unexpected key-event value.
                    default:
                    {
                        printf("[E][RELAY] Invalid key event: %d\r\n",
                               (int)received_key_event);
                        break;
                    }
                }
            }
        }
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

