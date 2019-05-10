/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "crc.h"
#include "usart.h"
#include "gpio.h"
#include "app_x-cube-ai.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AI_BUFFER_NULL(ptr_)  \
 AI_BUFFER_OBJ_INIT( \
   AI_BUFFER_FORMAT_NONE|AI_BUFFER_FMT_FLAG_CONST, \
   0, 0, 0, 0, \
   AI_HANDLE_PTR(ptr_))
   
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static ai_u8 activations[AI_MNETWORK_DATA_ACTIVATIONS_SIZE];
ai_float in_data[AI_MNETWORK_IN_1_SIZE];
ai_float out_data[AI_MNETWORK_OUT_1_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write( int file, char *ptr, int len )
{
  HAL_UART_Transmit(&huart1,(uint8_t*) ptr, len, 1000);
  return len;
}

/* USER CODE BEGIN 0 */


//static ai_handle network = AI_HANDLE_NULL;


static struct ai_network_exec_ctx {
   ai_handle network;
   ai_network_report report;
} net_exec_ctx[AI_MNETWORK_NUMBER] = {0};


static int aiBootstrap(const char *nn_name, const int idx)
{
    ai_error err;

    /* Creating the network */
    printf("Creating the network \"%s\"..\r\n", nn_name);
    err = ai_mnetwork_create(nn_name, &net_exec_ctx[idx].network, NULL);
    if (err.type) {

        printf("ERR:ai_mnetwork_create, %d %d\n\r",err.type,err.code);
        return -1;
    }

    /* Query the created network to get relevant info from it */
    if (ai_mnetwork_get_info(net_exec_ctx[idx].network, &net_exec_ctx[idx].report)){

    }
    else{
        err = ai_mnetwork_get_error(net_exec_ctx[idx].network);

        printf("ERR:ai_mnetwork_get_info, %d %d \n\r",err.type,err.code);
        ai_mnetwork_destroy(net_exec_ctx[idx].network);
        net_exec_ctx[idx].network = AI_HANDLE_NULL;
        return -2;
    }

    /* Initialize the instance */
    printf("Initializing the network\r\n");
    /* build params structure to provide the reference of the
     * activation and weight buffers */
    const ai_network_params params = {
            AI_BUFFER_NULL(NULL),
            AI_BUFFER_NULL(activations) };

    if (!ai_mnetwork_init(net_exec_ctx[idx].network, &params)) {
        err = ai_mnetwork_get_error(net_exec_ctx[idx].network);

        printf("ERR:ai_mnetwork_init, %d %d\n\r",err.type,err.code);
        ai_mnetwork_destroy(net_exec_ctx[idx].network);
        net_exec_ctx[idx].network = AI_HANDLE_NULL;
        return -4;
    }
    return 0;
}


int aiInit(void)
{
    int res = -1;
    const char *nn_name;
    int idx;

    /* Clean all network exec context */
   for (idx=0; idx < AI_MNETWORK_NUMBER; idx++) {
        net_exec_ctx[idx].network = AI_HANDLE_NULL;
    }

    /* Discover and init the embedded network */
	idx = 0;
	do {
		nn_name = ai_mnetwork_find(NULL, idx);
		if (nn_name) {
			printf("\r\nFound network \"%s\"\r\n", nn_name);
			res = aiBootstrap(nn_name, idx);
			if (res)
				nn_name = NULL;
		}
		idx++;
	} while (nn_name);

    return 0;
}


void aiDeinit(void)
{
	ai_error err;
	int idx;

	printf("Releasing the network(s)...\r\n");

	for (idx=0; idx<AI_MNETWORK_NUMBER; idx++) {
		if (net_exec_ctx[idx].network != AI_HANDLE_NULL) {
			if (ai_mnetwork_destroy(net_exec_ctx[idx].network)
					!= AI_HANDLE_NULL) {
				err = ai_mnetwork_get_error(net_exec_ctx[idx].network);
				printf("ERR:ai_mnetwork_destroy, %d %d\n\r",err.type,err.code);
			}
			net_exec_ctx[idx].network = AI_HANDLE_NULL;
		}
	}
}


int aiRun(const int idx, const ai_float *in_data, ai_float *out_data)
{
    ai_i32 nbatch;
    ai_error err;

    /* AI buffer handlers */
    ai_buffer ai_input[AI_MNETWORK_IN_1_SIZE] ;
    ai_buffer ai_output[AI_MNETWORK_OUT_1_SIZE];

    /* Parameters checking */
    if (!in_data || !out_data || !net_exec_ctx[idx].network)
        return -1;

    /* Initialize input/output buffer handlers */
    ai_input[0].n_batches = 1;
    ai_input[0].data = AI_HANDLE_PTR(in_data);
    ai_output[0].n_batches = 1;
    ai_output[0].data = AI_HANDLE_PTR(out_data);

    /* Perform the inference */
    nbatch = ai_mnetwork_run(net_exec_ctx[idx].network, &ai_input[0], &ai_output[0]);

    if (nbatch != 1) {
        err = ai_mnetwork_get_error(net_exec_ctx[idx].network);
        printf("ERR:ai_mnetwork_get_error, %d %d\n\r",err.type,err.code);
        return -1;
    }

    return 0;
}



void aiTest(void)
{
    int nb_run = 20;
    aiInit();

    while (--nb_run) {

        /* fill a float array with random data in the range [-1, 1] */
    	in_data[0]=1.0;
    	in_data[1]=0.0;

        /* perform an inference */
        aiRun(0,in_data, out_data);
        printf("IN0: %f, IN1: %f, OUT: %f \n\r",in_data[0],in_data[1],round(out_data[0]));
        while(1);

        HAL_Delay(100);
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

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

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
  MX_CRC_Init();
  MX_X_CUBE_AI_Init();
  /* USER CODE BEGIN 2 */
  printf("Init OK\n\r");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

  MX_X_CUBE_AI_Process();
    /* USER CODE BEGIN 3 */
    aiTest();
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Activate the Over-Drive mode 
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
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

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
