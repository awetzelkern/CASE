#include <Arduino.h>
#include <pins.h>
#include <SPI.h>
#include <SparkFun_BMP581_Arduino_Library.h>

#define ACC_CONF 0x20
#define GYR_CONF 0x21

BMP581 bmp;

SPISettings bmiSPISettings(1000000, MSBFIRST, SPI_MODE0);  // start safe at 1 MHz


// put function declarations here:
void setupPins();
void bmiSelect();
void bmiDeselect();
void bmiBeginSPI();

uint8_t bmiReadReg8(uint8_t reg);
uint16_t bmiReadReg16(uint8_t reg);
void bmiWriteReg16(uint8_t reg, uint16_t value);
bool initBMI323();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // Initialize serial communication at 115200 baud rate
  delay(1000); // Wait for a moment to ensure the serial connection is established
  
  Serial.println("Starting setup...");

  setupPins();

  SPI.begin(); // Initialize SPI communication

  // BMP581 initialization
  int8_t err = bmp.beginSPI(BMP_CS, 1000000);  // 1 MHz to start
  if (err != BMP5_OK) {
    Serial.print("BMP581 init failed, error = ");
    Serial.println(err);
    while (1) {}
  }

  // BMI323 initialization
  if (!initBMI323()) {
    Serial.println("BMI323 init failed");
    while (1) {}
  }
  uint16_t chipID16 = bmiReadReg16(0x00);
  Serial.print("BMI323 CHIP_ID reg16: 0x");
  Serial.println(chipID16, HEX);

  // Success!!
  Serial.println("BMP581 ready");
  Serial.println("BMI323 ready");
}

void loop() {
  bmp5_sensor_data data;

  int8_t err = bmp.getSensorData(&data);
  if (err == BMP5_OK) {
    Serial.print("Pressure (Pa): ");
    Serial.println(data.pressure);

    Serial.print("Temperature (C): ");
    Serial.println(data.temperature);
  } else {
    Serial.print("Read failed: ");
    Serial.println(err);
  }

  delay(100);
  
}

// put function definitions here:
void setupPins()
{
    // LEDs
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_ORANGE, OUTPUT);

    // Sensors
    pinMode(BMI_INT1, INPUT);
    pinMode(BMI_INT2, INPUT);
    pinMode(BMP_INT, INPUT);

    pinMode(BMP_CS, OUTPUT);
    pinMode(BMI_CS, OUTPUT);
    digitalWrite(BMP_CS, HIGH); // Deselect BMP581
    digitalWrite(BMI_CS, HIGH); // Deselect BMI323

    // TVC
    pinMode(TVC_X, OUTPUT);
    pinMode(TVC_Y, OUTPUT);

    // Buzzer
    pinMode(BUZZER, OUTPUT);

    // Boot switch
    pinMode(BOOT_SW, INPUT_PULLUP);
}

void bmiSelect() {
  SPI.beginTransaction(bmiSPISettings);
  digitalWrite(BMI_CS, LOW);
}

void bmiDeselect() {
  digitalWrite(BMI_CS, HIGH);
  SPI.endTransaction();
}

// By default the BMI323 is in I2C mode, so we need to do a dummy SPI transaction to switch it to SPI mode
void bmiBeginSPI() {
  bmiSelect();
  SPI.transfer(0x80);   // dummy read-like access
  SPI.transfer(0x00);   // dummy byte
  SPI.transfer(0x00);   // extra clocks
  bmiDeselect();
  delay(1);
}

// Read one byte from the BMI323 over SPI
uint8_t bmiReadReg8(uint8_t reg) {
  bmiSelect();

  SPI.transfer(reg | 0x80);   // read
  SPI.transfer(0x00);         // discard dummy byte
  uint8_t data = SPI.transfer(0x00);

  bmiDeselect();
  return data;
}

// Read two bytes from the BMI323 over SPI
uint16_t bmiReadReg16(uint8_t reg) {
  bmiSelect();

  SPI.transfer(reg | 0x80);   // read
  SPI.transfer(0x00);         // discard dummy byte

  uint8_t lo = SPI.transfer(0x00);
  uint8_t hi = SPI.transfer(0x00);

  bmiDeselect();
  return (uint16_t(hi) << 8) | lo;
}

// Write two bytes to the BMI323 over SPI
void bmiWriteReg16(uint8_t reg, uint16_t value) {
  bmiSelect();

  SPI.transfer(reg & 0x7F);        // write
  SPI.transfer(value & 0xFF);      // low byte
  SPI.transfer((value >> 8) & 0xFF); // high byte

  bmiDeselect();
}

bool initBMI323() {
  bmiBeginSPI();

  uint8_t chipID = bmiReadReg8(0x00);   // CHIP_ID
  Serial.print("BMI323 Chip ID: 0x");
  Serial.println(chipID, HEX);

  if (chipID != 0x43) {   // use the BMI323 expected chip ID from the datasheet
    Serial.println("ERROR: BMI323 not detected correctly");
    return false;
  }

  // Replace these with the real register addresses from your header/datasheet
  // ACC_CONF and GYR_CONF are 16-bit config registers
  bmiWriteReg16(ACC_CONF, 0x708B);
  bmiWriteReg16(GYR_CONF, 0x708B);

  delay(10);

  uint16_t accConfReadback = bmiReadReg16(ACC_CONF);
  uint16_t gyrConfReadback = bmiReadReg16(GYR_CONF);

  Serial.print("ACC_CONF readback: 0x");
  Serial.println(accConfReadback, HEX);
  Serial.print("GYR_CONF readback: 0x");
  Serial.println(gyrConfReadback, HEX);

  if (accConfReadback != 0x708B || gyrConfReadback != 0x708B) {
    Serial.println("BMI323 config readback mismatch");
    return false;
  }

  return true;
}