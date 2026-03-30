#include <Arduino.h>
#include <pins.h>
#include <SPI.h>
#include <SparkFun_BMP581_Arduino_Library.h>
#include <BMI323.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


BMP581 bmp;
BMI323 bmi;


constexpr float seaLevelPressure = 1013.25; // mb

// put function declarations here:
void setupPins();
void playTone(int freq, int duration);
void blinkLED(int ledPin, int onDuration, int offDuration);
void startupSuccessSong();
void errorTone();
float calculate_altitude(float pressure, float seaLevelPressure);

// FreeRTOS Threads
void barometer_thread(void* arg);
void imu_thread(void* arg);

void setup() {
    // put your setup code here, to run once:
    playTone(1000, 100); // Play a quick tone to indicate startup
    blinkLED(LED_GREEN, 200, 100);
    digitalWrite(LED_YELLOW, HIGH); // Keep yellow LED on to indicate we're in setup

    Serial.begin(115200); // Initialize serial communication at 115200 baud rate
    while(!Serial) { } // Wait for Serial to be ready

    Serial.println("Starting setup...");

    setupPins();

    SPI.begin(SCK, MISO, MOSI); // Initialize SPI communication

    // BMP581 initialization
    int8_t err = bmp.beginSPI(BMP_CS, 1000000);  // 1 MHz to start
    if (err != BMP5_OK) {
        Serial.print("BMP581 init failed, error = ");
        Serial.println(err);
        errorTone();
        while (true) {
        blinkLED(LED_RED, 500, 100);
        }
    }

    // BMI323 initialization
    if (bmi.initBMI323()) {
        Serial.println("BMI323 ready");
        Serial.println("Format: ax,ay,az,gx,gy,gz,temp");
        Serial.println("Units: g,g,g,deg/s,deg/s,deg/s,C");
    } else {
        Serial.println("BMI323 failed to initialize! Check wiring.");
    while (1);  // Stop if setup fails
    }

    digitalWrite(LED_YELLOW, LOW); // Turn off yellow LED to indicate setup is done
    startupSuccessSong();


    xTaskCreatePinnedToCore(barometer_thread, "barometer_thread", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(imu_thread, "imu_thread", 4096, NULL, 1, NULL, 1);

    while(true) { 
        delay(1000); // Keep main thread alive, all work is done in FreeRTOS tasks
    } // Keeps the main thread alive, all work is done in FreeRTOS tasks

    /* 
    The last code I wrote before frying my regulators, never do this again:

    digitalWrite(TVC_X, HIGH); // Center TVC servos
    delayMicroseconds(200);

    digitalWrite(TVC_X, LOW); // Move TVC to max position to test
    delayMicroseconds(1800);
    */
}

void loop() {
  // empty for FreeRTOS tasks, all code runs in the threads
}

// put function definitions here:
void setupPins()
{
    // LEDs
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    // pinMode(LED_ORANGE, OUTPUT); This overrides the GPIO pins when serial initializes, so you can't use USB data, thus Serial doesn't work

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
}

void playTone(int freq, int duration)
{
    tone(BUZZER, freq, duration);
    delay(duration * 1.3); // small gap between notes
}

void blinkLED(int ledPin, int onDuration, int offDuration)
{
    digitalWrite(ledPin, HIGH);
    delay(onDuration);
    digitalWrite(ledPin, LOW);
    delay(offDuration);
}

void startupSuccessSong()
{
    blinkLED(LED_GREEN, 200, 100);
    playTone(523, 120);   // C5
    playTone(659, 120);   // E5
    playTone(784, 120);   // G5
    playTone(1047, 180);  // C6
}

void errorTone()
{
    playTone(200, 500);   // low buzz
    playTone(200, 500);   // low buzz
}

float calculate_altitude(float pressure, float seaLevelPressure) {
    // The constant 44330 and the exponent 0.1903 are derived from the standard atmosphere model
    float altitude = 44330.0f * (1.0f - pow(pressure / seaLevelPressure, 0.1903f));
    return altitude;
}

void barometer_thread(void* arg) {
    while(true) {
        bmp5_sensor_data data;


        int8_t err = bmp.getSensorData(&data);
        if (err == BMP5_OK) {
        Serial.print("Pressure (Pa): ");
        Serial.println(data.pressure);

        float pressure = data.pressure / 100; // convert to milibars

        float altitude = calculate_altitude(pressure, seaLevelPressure);

        // Serial.print("Altitude (m):");
        // Serial.print(altitude);

        // Serial.print(" | Temperature (C): ");
        // Serial.print(data.temperature);
        // } else {
        // Serial.print("Read failed: ");
        // Serial.println(err);
        }
    delay(10);
    }
}

void imu_thread(void* arg) {
    while (true) {
        // Limit output to 20Hz
        unsigned long currentTime = millis();
        unsigned long lastPrintTime = 0;
        const unsigned long printInterval = 1000 / OUTPUT_RATE_HZ; // 20Hz

        if (currentTime - lastPrintTime < printInterval) return;
        lastPrintTime = currentTime;

        // Read sensor data
        uint16_t accX = bmi.readRegister16(ACC_DATA_X_REG);
        uint16_t accY = bmi.readRegister16(ACC_DATA_Y_REG);
        uint16_t accZ = bmi.readRegister16(ACC_DATA_Z_REG);
        uint16_t gyrX = bmi.readRegister16(GYR_DATA_X_REG);
        uint16_t gyrY = bmi.readRegister16(GYR_DATA_Y_REG);
        uint16_t gyrZ = bmi.readRegister16(GYR_DATA_Z_REG);
        uint16_t tempRaw = bmi.readRegister16(TEMP_DATA_REG);

        // Convert to physical units
        float ax = bmi.convertAccelData(accX);
        float ay = bmi.convertAccelData(accY);
        float az = bmi.convertAccelData(accZ);
        float gx = bmi.convertGyroData(gyrX);
        float gy = bmi.convertGyroData(gyrY);
        float gz = bmi.convertGyroData(gyrZ);
        float temp = bmi.convertTempData(tempRaw);

        if (ax > 1) {
            blinkLED(LED_GREEN, 50, 0); // Quick blink on new data
        } else if (ay > 1) {
            blinkLED(LED_YELLOW, 50, 0);
        } else if (az > 1) {
            blinkLED(LED_RED, 50, 0);
        }

        //Print valid data in CSV format
        if (!isnan(ax) && !isnan(ay) && !isnan(az) &&
            !isnan(gx) && !isnan(gy) && !isnan(gz)) {
            Serial.print(ax, 3); Serial.print(",");
            Serial.print(ay, 3); Serial.print(",");
            Serial.print(az, 3); Serial.print(",");
            Serial.print(gx, 2); Serial.print(",");
            Serial.print(gy, 2); Serial.print(",");
            Serial.print(gz, 2); Serial.print(",");
            Serial.println(isnan(temp) ? "NAN" : String(temp, 1));
        }
    }   
}
