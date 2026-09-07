#include "ina219.h"

#define INA219_ADDR              (0x40 << 1)

#define INA219_REG_CONFIG        0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01
#define INA219_REG_BUS_VOLTAGE   0x02
#define INA219_REG_POWER         0x03
#define INA219_REG_CURRENT       0x04
#define INA219_REG_CALIBRATION   0x05

#define INA219_CALIBRATION_VALUE 0x1000

#define INA219_CURRENT_LSB       0.0001f
#define INA219_POWER_LSB         0.002f

HAL_StatusTypeDef INA219_Init(void)
{
    uint8_t calib_data[2];

    calib_data[0] = (INA219_CALIBRATION_VALUE >> 8) & 0xFF;
    calib_data[1] = INA219_CALIBRATION_VALUE & 0xFF;

    return HAL_I2C_Mem_Write(&hi2c1,
                             INA219_ADDR,
                             INA219_REG_CALIBRATION,
                             I2C_MEMADD_SIZE_8BIT,
                             calib_data,
                             2,
                             HAL_MAX_DELAY);
}

HAL_StatusTypeDef INA219_ReadData(INA219_Data *data)
{
    uint8_t raw[2];
    uint16_t bus_reg;
    int16_t current_reg;
    uint16_t power_reg;

    if (HAL_I2C_Mem_Read(&hi2c1,
                         INA219_ADDR,
                         INA219_REG_BUS_VOLTAGE,
                         I2C_MEMADD_SIZE_8BIT,
                         raw,
                         2,
                         HAL_MAX_DELAY) != HAL_OK)
    {
        return HAL_ERROR;
    }

    bus_reg = ((uint16_t)raw[0] << 8) | raw[1];
    data->bus_voltage = ((bus_reg >> 3) * 4.0f) / 1000.0f;

    if (HAL_I2C_Mem_Read(&hi2c1,
                         INA219_ADDR,
                         INA219_REG_CURRENT,
                         I2C_MEMADD_SIZE_8BIT,
                         raw,
                         2,
                         HAL_MAX_DELAY) != HAL_OK)
    {
        return HAL_ERROR;
    }

    current_reg = (int16_t)(((uint16_t)raw[0] << 8) | raw[1]);
    data->current = current_reg * INA219_CURRENT_LSB;

    if (HAL_I2C_Mem_Read(&hi2c1,
                         INA219_ADDR,
                         INA219_REG_POWER,
                         I2C_MEMADD_SIZE_8BIT,
                         raw,
                         2,
                         HAL_MAX_DELAY) != HAL_OK)
    {
        return HAL_ERROR;
    }

    power_reg = ((uint16_t)raw[0] << 8) | raw[1];
    data->power = power_reg * INA219_POWER_LSB;

    return HAL_OK;
}
