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

#include <stdlib.h>
#include <string.h>   
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
#define NAME_BUF_SIZE  2048    
#define MAX_ENTRIES    256     
#define MAX_COLS       64 

static uint16_t col_max[MAX_COLS];
static char    	name_buf[NAME_BUF_SIZE];
static uint16_t name_off[MAX_ENTRIES];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static int cmp_name(const void *a, const void *b)
{
    uint16_t ia = *(const uint16_t *)a;
    uint16_t ib = *(const uint16_t *)b;
    return strcmp(name_buf + ia, name_buf + ib);
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
	char *fn;

	char path_buf[256];
	if(argc >= 2 && argv[1] != NULL && argv[1][0] != '\0') {
		if(argv[1][0] == '/')
			snprintf(path_buf, sizeof(path_buf), "0:%s", argv[1]);
		else
			snprintf(path_buf, sizeof(path_buf), "0:/%s", argv[1]);
	} else {
		snprintf(path_buf, sizeof(path_buf), "0:/");
	}
	size_t plen = strlen(path_buf);
	if(plen > 0 && path_buf[plen-1] != '/' && plen < sizeof(path_buf) - 1) {
		path_buf[plen]   = '/';
		path_buf[plen+1] = '\0';
	}
	char *path = path_buf;

	static char font_path[64] = "0:/sys/fonts/UNICODE-SYS-Regular-8x8";
    FIL file;
    char line[128];

    if (f_open(&file, PATH_VCONSOLE_CONF, FA_READ) != FR_OK) {
        return 1;
    }

    while (f_gets(line, sizeof(line), &file)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';

        /* 跳过注释和空行 */
        if (line[0] == '#' || line[0] == '\0') continue;

        /* 分割 KEY=VALUE */
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        char *key   = line;
        char *value = eq + 1;

        if (strcmp(key, "FONT") == 0) {
            snprintf(font_path, sizeof(font_path),
                     "%s/%s", PATH_FONTS, value);
        }
    }
    f_close(&file);

    res = f_open(&screen_fil, PATH_SCREEN, FA_WRITE | FA_OPEN_APPEND);
	if(res == FR_OK) {
		//	获取'M'宽度
		uint32_t cp = (uint32_t)'M';
		uint16_t width = 0;
		if (f_open(&fil, font_path, FA_READ) == FR_OK) {
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
		uint16_t char_col = screen_info.xres / width;
		if(char_col == 0) char_col = 1;

		// ls
		uint16_t n_entries     = 0;
		uint16_t buf_used      = 0;
		uint16_t max_fname_len = 0;

		res = f_opendir(&dir, path);
		if(res == FR_OK) {
			while(1) {
				res = f_readdir(&dir, &fno);
				if(res != FR_OK || fno.fname[0] == 0) break;
				if(n_entries >= MAX_ENTRIES) break;

				#if _USE_LFN
					fn = (*fno.lfname) ? fno.lfname : fno.fname;
				#else
					fn = fno.fname;
				#endif

				uint16_t len = strlen(fn);
				if(buf_used + len + 1 > NAME_BUF_SIZE) break;   // 缓冲满

				name_off[n_entries] = buf_used;
				memcpy(name_buf + buf_used, fn, len + 1);
				buf_used += len + 1;
				n_entries++;

				if(len > max_fname_len) max_fname_len = len;
			}
			f_closedir(&dir);
		}

		if(n_entries > 1)
			qsort(name_off, n_entries, sizeof(uint16_t), cmp_name);

		// 迭代尝试列数
		uint16_t C_hi = (n_entries < MAX_COLS) ? n_entries : MAX_COLS;
		if(C_hi < 1) C_hi = 1;

		uint16_t max_fname_col = 1;
		uint16_t max_fname_row = n_entries;

		for(uint16_t tryC = C_hi; ; tryC--) {
			uint16_t tryR = (n_entries + tryC - 1) / tryC;
			uint16_t total = 0;
			for(uint16_t c = 0; c < tryC; c++) {
				uint16_t m = 0;
				for(uint16_t r = 0; r < tryR; r++) {
					uint16_t idx = c * tryR + r;
					if(idx >= n_entries) break;
					uint16_t len = strlen(name_buf + name_off[idx]);
					if(len > m) m = len;
				}
				col_max[c] = m + 2;
				total += m + 2;
			}
			if(total - 2 <= char_col || tryC == 1) {
				max_fname_col = tryC;
				max_fname_row = tryR;
				break;
			}
		}

		for(uint16_t r = 0; r < max_fname_row; r++) {
			for(uint16_t c = 0; c < max_fname_col; c++) {
				uint16_t idx = c * max_fname_row + r;
				if(idx >= n_entries) {
					for(uint16_t k = 0; k < col_max[c]; k++)
						f_write(&screen_fil, " ", 1, &bw);
					continue;
				}
				char *name = name_buf + name_off[idx];
				uint16_t n   = strlen(name);
				uint16_t pad = (col_max[c] > n) ? (col_max[c] - n) : 0;

				f_write(&screen_fil, name, n, &bw);
				while(pad--) f_write(&screen_fil, " ", 1, &bw);
			}
			f_write(&screen_fil, "\r\n", 2, &bw);
		}

		f_close(&screen_fil);
	}

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
