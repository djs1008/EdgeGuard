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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
static void DWT_Delay_Init(void)
{

	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CYCCNT =0;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
	uint32_t start = DWT->CYCCNT;
	uint32_t ticks = us * (SystemCoreClock / 1000000U);
	while((DWT->CYCCNT - start) < ticks)
	{
	}
}

static void DS18B20_SetPinOutput(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};

	GPIO_InitStructure.Pin = DS18B20_DQ_Pin;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStructure.Pull = GPIO_NOPULL;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(DS18B20_DQ_GPIO_Port, &GPIO_InitStructure);
}

static void DS18B20_SetPinInput(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = {0};

	GPIO_InitStructure.Pin = DS18B20_DQ_Pin;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(DS18B20_DQ_GPIO_Port, &GPIO_InitStructure);
}

static uint8_t DS18B20_Reset(void)
{
	uint8_t presence;

	DS18B20_SetPinOutput();

	//主机至少拉低480us
	HAL_GPIO_WritePin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin, GPIO_PIN_RESET);
	delay_us(480);

	//释放总线
	HAL_GPIO_WritePin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin, GPIO_PIN_SET);
	DS18B20_SetPinInput();

	//等待DS18B20的Presence Pulse
	delay_us(70);

	presence = HAL_GPIO_ReadPin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin);

	//等待整个复位时序结束
	delay_us(410);

	return presence;
}

static void DS18B20_WriteBit(uint8_t bit)
{
	DS18B20_SetPinOutput();
	HAL_GPIO_WritePin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin, GPIO_PIN_RESET);
	if(bit)
	{
		//写1 只短暂拉低
		delay_us(6);
		HAL_GPIO_WritePin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin, GPIO_PIN_SET);
		delay_us(64);
	}
	else
	{
		//写0 保持较长时间低电平
		delay_us(60);
		HAL_GPIO_WritePin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin, GPIO_PIN_SET);
		delay_us(10);
	}
}

static uint8_t DS18B20_ReadBit(void)
{
	uint8_t bit;

	DS18B20_SetPinOutput();

	//主机先拉低，启动一个读时隙
	HAL_GPIO_WritePin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin, GPIO_PIN_RESET);
	delay_us(3);

	//释放总线，并切换为输入
	HAL_GPIO_WritePin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin, GPIO_PIN_RESET);
	DS18B20_SetPinInput();

	//等待合适时刻采样
	delay_us(10);
	bit = HAL_GPIO_ReadPin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin);

	//等待这个读时隙结束
	delay_us(50);

	return bit;
}

static void DS18B20_WriteByte(uint8_t data)
{
	for(uint8_t i = 0; i < 8; i ++)
	{
		DS18B20_WriteBit(data & 0x01);//取最低位

		data >>= 1;//右移，准备写下一个
	}
}

static uint8_t DS18B20_ReadByte(void)
{
	uint8_t data = 0;
	for(uint8_t i = 0; i < 8; i ++)
	{
		if(DS18B20_ReadBit())
		{
			//如果读到这一位是 1，就把 data 对应的第 i 位设为 1
			data |= (1 << i);
		}
	}
	return data;
}

static float DS18B20_ReadTemperature(void)
{
	uint8_t temp_l;
	uint8_t temp_h;
	uint16_t raw_temp;

	//第一次Reset
	if(DS18B20_Reset() != GPIO_PIN_RESET)
	{
		return -1000.0f;
	}

	//只有一个DS18B20，所以跳过ROM匹配
	DS18B20_WriteByte(0xCC);

	//开始温度转换
	DS18B20_WriteByte(0x44);

	//等待转换成功
	HAL_Delay(750);

	//第二次转换
	if(DS18B20_Reset() != GPIO_PIN_RESET)
	{
		return -1000.0f;
	}

	DS18B20_WriteByte(0xCC);

	//读取 Scratchpad
	DS18B20_WriteByte(0xBE);

	//前两个字节就是温度
	temp_l = DS18B20_ReadByte();
	temp_h = DS18B20_ReadByte();

	raw_temp = (uint16_t)((temp_h << 8) | temp_l);

	return raw_temp / 16.0f;
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
  /* USER CODE BEGIN 2 */
  DWT_Delay_Init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  float temperature = DS18B20_ReadTemperature();

  if (temperature > 30.0f)
  {
	  HAL_GPIO_WritePin(USER_LED_GPIO_Port,
						USER_LED_Pin,
						GPIO_PIN_RESET);
  }
  else
  {
	  HAL_GPIO_WritePin(USER_LED_GPIO_Port,
						USER_LED_Pin,
						GPIO_PIN_SET);
  }

  HAL_Delay(500);
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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
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
