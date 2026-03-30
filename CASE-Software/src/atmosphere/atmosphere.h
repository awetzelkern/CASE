#pragma once

class Atmosphere {
    private:
        const float seaLevelPressure = 101325; // Pa
        const float L = 0.0065; // K/m                  (lapse rate)
        const float R = 287.05; // J / kg*K             (gas constant)
        const float g = 9.81; // m/s^2                  (gravity)
        const float T0 = 288.15; // K (15 C)

        float groundPressure;     // Pa
        float groundTemperature;  // K
        float groundAltitude;     // m above sea level

    public:
        Atmosphere(float groundPressure, float groundTemperature, float groundAltitude): 
            groundPressure(groundPressure), groundTemperature(groundTemperature), groundAltitude(groundAltitude) {}
        void setGroundConditions(float pressurePa, float temperatureK, float altitudeM);
        float calculate_altitude(float pressure);
        float calculate_AGL(float pressure, float temperature);
};