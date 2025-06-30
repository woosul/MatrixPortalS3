#ifndef MODE_COUNTDOWN_H
#define MODE_COUNTDOWN_H

#include "config.h"
#include <Adafruit_PixelDust.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

// Forward declarations
class Utils;
class MatrixPanel_I2S_DMA;

class ModeCountdown {
public:
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr);
    void run();
    void cleanup();
    void drawSandClock();
    void drawTextCountdown();
    void sandBox();
    
    // Add methods to control countdown
    void startCountdown(unsigned long durationMs);
    void setCountdown(int minutes, int seconds);
    bool isCountdownFinished();
    bool independantSandboxEnable; // true: use sandbox, false: use text box

    private:
    Utils* m_utils;
    MatrixPanel_I2S_DMA* m_matrix;
    unsigned long lastUpdate;
    unsigned long countdownStart;    // Add this line
    unsigned long countdownDuration; // Add this line
    bool useTextBox;      // true: use top 8px for text area, false: use full area
    uint8_t sandSimHeight; // Actual height for sand simulation
    

};

#endif