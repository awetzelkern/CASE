#pragma once // does #ifindef for you
#include <SPI.h>
#include <Arduino.h>
#include <pins.h>

// Pin and register definitions
#define BMI323_CS BMI_CS
#define CHIP_ID_REG 0x00
#define ERR_REG 0x01
#define STATUS_REG 0x02
#define ACC_CONF_REG 0x20
#define GYR_CONF_REG 0x21
#define ACC_DATA_X_REG 0x03
#define ACC_DATA_Y_REG 0x04
#define ACC_DATA_Z_REG 0x05
#define GYR_DATA_X_REG 0x06
#define GYR_DATA_Y_REG 0x07
#define GYR_DATA_Z_REG 0x08
#define TEMP_DATA_REG 0x09
#define CMD_REG 0x7E
#define FEATURE_IO0_REG 0x10
#define FEATURE_IO1_REG 0x11
#define FEATURE_IO2_REG 0x12
#define FEATURE_IO_STATUS_REG 0x14
#define FEATURE_CTRL_REG 0x40

// Configuration constants
#define SPI_FREQ 8000000  // 8MHz SPI clock
#define OUTPUT_RATE_HZ 20  // 20Hz data output
#define ACC_RANGE_LSB_PER_G 4096.0  // +-8g range
#define GYR_RANGE_LSB_PER_DPS 16.384  // +-2000 deg/s range
#define SOFT_RESET_CMD 0xDEAF  // Soft reset command
#define ACC_CONF_NORMAL_100HZ_8G 0x4028  // Accel: 100Hz, +-8g
#define GYR_CONF_NORMAL_100HZ_2000DPS 0x4048  // Gyro: 100Hz, +-2000 deg/s

class BMI323 {
    private:
    
    public:
    uint16_t readRegister16(uint8_t reg);
    void writeRegister16(uint8_t reg, uint16_t data);
    float convertAccelData(uint16_t rawData);
    float convertGyroData(uint16_t rawData);
    float convertTempData(uint16_t rawData);
    bool initializeFeatureEngine();
    bool initBMI323();
};