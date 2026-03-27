/*
 * bmi323_spi_example.ino
 * 
 * Simple Arduino example for interfacing with the BMI323 6-axis IMU over SPI.
 * Reads accelerometer, gyroscope, and temperature data at 20Hz.
 * Outputs data in CSV format to the Serial monitor.
 * 
 * Arduino (Nano) pinout:
 *   D10 -> CS
 *   D11 -> MOSI/SDI/SDX
 *   D12 -> MISO/SDO
 *   D13 -> SCK/SCX
 *
 * Output Format: ax,ay,az,gx,gy,gz,temp
 * Units: g,g,g,deg/s,deg/s,deg/s,C
 *
 * For more details, see the BMI323 datasheet.
*/

#include <SPI.h>
#include <BMI323.h>
#include <Arduino.h>

SPISettings bmi323Settings(SPI_FREQ, MSBFIRST, SPI_MODE0);

uint16_t BMI323::readRegister16(uint8_t reg) {
  SPI.beginTransaction(bmi323Settings);
  digitalWrite(BMI323_CS, LOW);
  delayMicroseconds(1);

  SPI.transfer(reg | 0x80);  // Read command
  SPI.transfer(0x00);        // Dummy byte
  uint8_t lsb = SPI.transfer(0x00);
  uint8_t msb = SPI.transfer(0x00);

  digitalWrite(BMI323_CS, HIGH);
  SPI.endTransaction();
  delayMicroseconds(2);

  return (msb << 8) | lsb;
}

void BMI323::writeRegister16(uint8_t reg, uint16_t data) {
  SPI.beginTransaction(bmi323Settings);
  digitalWrite(BMI323_CS, LOW);
  delayMicroseconds(1);

  SPI.transfer(reg & 0x7F);        // Write command
  SPI.transfer(data & 0xFF);       // LSB
  SPI.transfer((data >> 8) & 0xFF); // MSB

  digitalWrite(BMI323_CS, HIGH);
  SPI.endTransaction();
  delayMicroseconds(2);
}

float BMI323::convertAccelData(uint16_t rawData) {
  int16_t signedData = (int16_t)rawData;
  if (signedData == -32768) return NAN;
  return signedData / ACC_RANGE_LSB_PER_G;
}

float BMI323::convertGyroData(uint16_t rawData) {
  int16_t signedData = (int16_t)rawData;
  if (signedData == -32768) return NAN;
  return signedData / GYR_RANGE_LSB_PER_DPS;
}

float BMI323::convertTempData(uint16_t rawData) {
  int16_t signedData = (int16_t)rawData;
  if (signedData == -32768) return NAN;
  return (signedData / 512.0) + 23.0;
}

bool BMI323::initializeFeatureEngine() {
  // Clear feature config
  writeRegister16(FEATURE_IO0_REG, 0x0000);
  delay(1);

  // Set startup config
  writeRegister16(FEATURE_IO2_REG, 0x012C);
  delay(1);

  // Trigger startup
  writeRegister16(FEATURE_IO_STATUS_REG, 0x0001);
  delay(1);

  // Enable feature engine
  writeRegister16(FEATURE_CTRL_REG, 0x0001);
  delay(10);

  // Check feature engine status
  int timeout = 0;
  uint16_t featureIO1Status;
  do {
    delay(10);
    featureIO1Status = readRegister16(FEATURE_IO1_REG);
    uint8_t errorStatus = featureIO1Status & 0x0F;
    if (errorStatus == 0x01) return true;
    if (errorStatus == 0x03) {
      Serial.println("Feature engine error");
      return false;
    }
    timeout++;
  } while ((featureIO1Status & 0x0F) == 0x00 && timeout < 50);

  if (timeout >= 50) {
    Serial.println("Feature engine timeout");
    return false;
  }
  return true;
}

bool BMI323::initBMI323() {
  // Switch to SPI mode
  readRegister16(CHIP_ID_REG);
  delayMicroseconds(200);

  // Check chip ID
  uint16_t chipID = readRegister16(CHIP_ID_REG);
  if ((chipID & 0xFF) != 0x43 && (chipID & 0xFF) != 0x41) {
    Serial.print("Invalid chip ID: 0x");
    Serial.println(chipID, HEX);
    return false;
  }

  // Reset sensor
  writeRegister16(CMD_REG, SOFT_RESET_CMD);
  delay(5);

  // Set up feature engine
  if (!initializeFeatureEngine()) {
    Serial.println("Feature engine setup failed");
    return false;
  }

  // Configure accelerometer and gyroscope
  writeRegister16(ACC_CONF_REG, ACC_CONF_NORMAL_100HZ_8G);
  writeRegister16(GYR_CONF_REG, GYR_CONF_NORMAL_100HZ_2000DPS);
  delay(50);

  return true;
}