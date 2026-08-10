#include "ds18b20.h"

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

uint8_t DS18B20_Reset(void)
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
	HAL_GPIO_WritePin(DS18B20_DQ_GPIO_Port, DS18B20_DQ_Pin, GPIO_PIN_SET);
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

float DS18B20_ReadTemperature(void)
{
	uint8_t temp_l;
	uint8_t temp_h;
	int16_t raw_temp;

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

	raw_temp = (int16_t)((temp_h << 8) | temp_l);

	return raw_temp / 16.0f;
}

void DS18B20_Init(void)
{
	DWT_Delay_Init();
}
