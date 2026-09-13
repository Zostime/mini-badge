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
#include "adc.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ST7789V.h"
#include "GUI.h"
#include "Key.h"
#include "ff.h"
#include "Buzzer.h"
#include "Power.h"
#include "rtc_utils.h"
#include "kernel.h"  
#include "sys_path.h"
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  static char *argv[EXEC_MAX_ARGS + 1];
  static char *envp[EXEC_MAX_ENVS + 1];
  int argc;
  execve_load(&argc, argv, envp);	
  /* APP CODE BEGIN */
	FIL fil;  
	FILINFO fno;
    DIR dir;
    FRESULT res;
	
	UINT bw;
	FIL screen_fil;
	char *path = argv[1]; 

    res = f_open(&screen_fil, PATH_SCREEN, FA_WRITE | FA_OPEN_APPEND);
	if(res == FR_OK) {
		//	获取'M'宽度
		uint32_t cp = (uint32_t)'M';
		uint16_t width = 0;
		if (f_open(&fil, PATH_DEFAULT_FONT, FA_READ) == FR_OK) {
			if (f_lseek(&fil, (DWORD)cp * 9) == FR_OK) {
				uint8_t width_byte = 0;
				UINT br;
				if (f_read(&fil, &width_byte, 1, &br) == FR_OK && br == 1) {
					width = width_byte;
				}
			}
			f_close(&fil);
		}
		
		if(!width) width=1;  
		uint8_t char_col = screen_info.xres / width; 

		// ls
		uint16_t max_fname_len = 0;	
		uint16_t max_fname_col = 0;
		uint16_t max_fname_row = 0;
		uint16_t nf_total = 0;
		char *fn;

		res = f_opendir(&dir, path);
		if(res == FR_OK) {
			for(uint8_t i = 0; i <= 1; i++) {
				if(i) {
					f_closedir(&dir);
					res = f_opendir(&dir, path);
					if(res != FR_OK) break;

					max_fname_col = char_col / (max_fname_len + 2);
					if(max_fname_col == 0) max_fname_col=1;
					
					max_fname_row = (nf_total+max_fname_col-1) / max_fname_col;
				
				}

				while(1) {
					res = f_readdir(&dir, &fno);
					if(res != FR_OK || fno.fname[0] == 0) break;

					#if _USE_LFN
						fn = (*fno.lfname) ? fno.lfname : fno.fname;
					#else
						fn = fno.fname;
					#endif

					if(!i) {
						uint16_t len = strlen(fn);
						if(len > max_fname_len) max_fname_len = len;
						nf_total++;                
					} else if(fno.fattrib & AM_DIR) {
						f_write(&screen_fil, fn, strlen(fn), &bw);
						f_write(&screen_fil, " ", 1, &bw);
					} else {
						f_write(&screen_fil, fn, strlen(fn), &bw);
						f_write(&screen_fil, " ", 1, &bw);
					}
				}
			}
		} else {
			// Open DIR Fail
		}
		f_closedir(&dir);
	}
	f_write(&screen_fil, "\n", 1, &bw);
	f_close(&screen_fil);
	
	/* APP CODE END */
	// JMP sh
	argv[0] = "sh";
	argv[1] = NULL;
	execve("0:/bin/sh", argv, NULL);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC
                              |RCC_PERIPHCLK_USB;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
