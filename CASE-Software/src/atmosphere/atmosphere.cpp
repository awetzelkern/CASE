#include <atmosphere/atmosphere.h>
#include <cmath>

/*
 * This file contains the implementation of the functions declared in atmosphere.h.
 * It includes the logic for reading sensor data, calculating altitude, and handling
 * FreeRTOS threads for the barometer and IMU.
 *
 * For more details, see the BMP581 and BMI323 datasheets, as well as the FreeRTOS documentation.
*/

// Accurate up to 11km
float Atmosphere::calculate_altitude(float pressure) {
    float altitude = (T0 / L) * (1.0 - pow((pressure / seaLevelPressure), ((L * R) / g)));
    return altitude;
}