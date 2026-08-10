/*
 * ds18b20.h
 *
 *  Created on: 2026年8月10日
 *      Author: DarkFlameMaster
 */

#ifndef INC_DS18B20_H_
#define INC_DS18B20_H_

#include "main.h"

void DS18B20_Init(void);
uint8_t DS18B20_Reset(void);
float DS18B20_ReadTemperature(void);

#endif /* INC_DS18B20_H_ */
