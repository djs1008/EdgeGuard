#ifndef __ADXL345_H
#define __ADXL345_H

#include "main.h"
#include "i2c.h"

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;

    float x_g;
    float y_g;
    float z_g;
} ADXL345_Data;

HAL_StatusTypeDef ADXL345_Init(void);
HAL_StatusTypeDef ADXL345_ReadData(ADXL345_Data *data);

#endif
