/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef void (*pFunction)(void); //定义无参数无返回值的函数指针类型
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_FLASH_ADDR 0x08019000 //APP 程序在Flash中的起始地址

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static pFunction JumpToApplication; //APP 入口函数指针,用于跳转并执行APP 程序
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// 按空循环次数进行延时，count表示循环次数
void delay(volatile uint32_t count)
{
    while (count--)
    {
    }
}

// 按秒进行延时，seconds表示延时秒数
void delay_seconds(uint32_t seconds)
{
    uint32_t count = 100000000;

    for (uint32_t i = 0; i < seconds; i++)
    {
        delay(count);
    }
}

/**
 * @brief 跳转到APP前关闭Bootloader使用的部分资源
 */
void DisablePeripherals(void)
{
    // 关闭RTC时钟
    __HAL_RCC_RTC_DISABLE();

    /*
     * 禁止CPU响应可屏蔽中断。
     * 注意：不会关闭外设，也不会清除已经挂起的中断；
     * 跳转到APP后，由APP调用__enable_irq()重新开启中断。
     */
    __disable_irq();
}

/**
 * @brief 检查APP是否有效，并从Bootloader跳转到APP
 *
 * APP向量表的第一个32位数据是初始MSP，
 * 第二个32位数据是APP的复位入口地址。
 */
void JumpToApp(void)
{
    uint16_t i;
    uint32_t jumpAddr;

    delay_seconds(2);

    // 打印Bootloader运行信息
    for (i = 0; i < 500; i++)
    {
        printf("bootloader running...\r\n");
    }

    delay_seconds(1);

    /*
     * __IO相当于volatile，要求编译器每次都从指定的Flash地址读取数据，
     * 不能省略本次访问或直接复用之前缓存的值。
     *
     * APP_FLASH_ADDR处存放APP的初始MSP；
     * 通过按位与判断初始MSP是否位于SRAM地址范围内。
     */
    if (((*(__IO uint32_t *)APP_FLASH_ADDR) & 0x2FFE0000U)
        == 0x20000000U)
    {
        // 读取APP向量表的第二个32位数据，即Reset_Handler入口地址
        jumpAddr = *(__IO uint32_t *)(APP_FLASH_ADDR + 4U);

        // 将APP入口地址转换为函数指针
        JumpToApplication = (pFunction)jumpAddr;

        // 最后一次串口打印已完成，跳转前反初始化串口
        HAL_UART_DeInit(&huart1);

        /*
         * 设置APP的初始栈指针。
         * 修改MSP后不再执行其他处理，立即跳转到APP。
         */
        __set_MSP(*(__IO uint32_t *)APP_FLASH_ADDR);

        // 跳转到APP的Reset_Handler并开始执行APP
        JumpToApplication();
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
    SCB->VTOR=0x08000000 | 0x00000000;// 将Bootloader的中断向量表设置到Flash起始地址
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  
    // 关闭Bootloader使用的部分资源，并禁止中断
    DisablePeripherals();

    // 检查APP是否有效，有效则跳转执行APP
    JumpToApp();
    
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
      printf("TEST 1142!\r\n");
      HAL_Delay(2000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 12;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
