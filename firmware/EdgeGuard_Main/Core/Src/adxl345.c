#include "adxl345.h"

#define ADXL345_ADDR         (0x53 << 1)

#define ADXL345_REG_DEVID    0x00
#define ADXL345_REG_POWERCTL 0x2D
#define ADXL345_REG_DATAX0   0x32

#define ADXL345_DEVID_VALUE  0xE5

HAL_StatusTypeDef ADXL345_Init(void)
{
    uint8_t devid = 0;
    uint8_t power_ctl = 0x08;

    if (HAL_I2C_Mem_Read(&hi2c1,
                         ADXL345_ADDR,
                         ADXL345_REG_DEVID,
                         I2C_MEMADD_SIZE_8BIT,
                         &devid,
                         1,
                         HAL_MAX_DELAY) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (devid != ADXL345_DEVID_VALUE)
    {
        return HAL_ERROR;
    }

    return HAL_I2C_Mem_Write(&hi2c1,
                             ADXL345_ADDR,
                             ADXL345_REG_POWERCTL,
                             I2C_MEMADD_SIZE_8BIT,
                             &power_ctl,
                             1,
                             HAL_MAX_DELAY);
}

HAL_StatusTypeDef ADXL345_ReadData(ADXL345_Data *data)
{
    uint8_t raw[6];

    if (HAL_I2C_Mem_Read(&hi2c1,
                         ADXL345_ADDR,
                         ADXL345_REG_DATAX0,
                         I2C_MEMADD_SIZE_8BIT,
                         raw,
                         6,
                         HAL_MAX_DELAY) != HAL_OK)
    {
        return HAL_ERROR;
    }

    data->x = (int16_t)((raw[1] << 8) | raw[0]);
    data->y = (int16_t)((raw[3] << 8) | raw[2]);
    data->z = (int16_t)((raw[5] << 8) | raw[4]);

    data->x_g = data->x / 256.0f;
    data->y_g = data->y / 256.0f;
    data->z_g = data->z / 256.0f;

    return HAL_OK;
}
