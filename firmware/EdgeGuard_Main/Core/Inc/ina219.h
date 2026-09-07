#ifndef __INA219_H
#define __INA219_H

#include "main.h"
#include "i2c.h"

typedef struct
{
    float bus_voltage;
    float current;
    float power;
} INA219_Data;

HAL_StatusTypeDef INA219_Init(void);
HAL_StatusTypeDef INA219_ReadData(INA219_Data *data);

#endif
