#include "mode_countdown.h"
#include "common.h"
#include "utils.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <Adafruit_PixelDust.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

// Sand simulation variables - EXACTLY like Adafruit example
Adafruit_PixelDust *sand = nullptr;
Adafruit_LIS3DH accel = Adafruit_LIS3DH();

// EXACTLY like Adafruit example
#define N_COLORS   8
#define BOX_HEIGHT 8
#define N_GRAINS (BOX_HEIGHT*N_COLORS*8)
#define MAX_FPS 40 // Increased for more responsiveness

uint16_t colors[N_COLORS];
uint32_t prevTime = 0; // Used for frames-per-second throttle - EXACTLY like Adafruit example
// bool independantSandboxEnable = false; // REMOVE THIS GLOBAL VARIABLE

void ModeCountdown::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) {
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;

    lastUpdate = 0;
    countdownStart = millis();
    countdownDuration = (5 * 60 * 1000UL); // Default to 5 minutes

    useTextBox = true; // true: display accelerometer data in top 8px, false: use full screen for sand
    sandSimHeight = useTextBox ? (MATRIX_HEIGHT - 8) : MATRIX_HEIGHT;

    // Initialize PixelDust - EXACTLY like Adafruit example
    sand = new Adafruit_PixelDust(MATRIX_WIDTH, sandSimHeight, N_GRAINS, 1, 128, false);
    
    if(!sand->begin()) {
        Serial.println("Couldn't start sand");
        this->independantSandboxEnable = false; // Use class member: Disable sandbox if initialization fails
        delete sand;
        sand = nullptr;
        return;
    }

    // Initialize accelerometer - EXACTLY like Adafruit example
    if (!accel.begin(0x19)) {
        Serial.println("Couldn't find accelerometer");
        return;
    }
    accel.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!

    // Set up initial sand coordinates, in 8x8 blocks - EXACTLY like Adafruit example
    int n = 0;
    for(int i=0; i<N_COLORS; i++) {
        int xx = i * MATRIX_WIDTH / N_COLORS;
        int yy = sandSimHeight - BOX_HEIGHT; // Change Y coordinate reference to sandSimHeight
        for(int y=0; y<BOX_HEIGHT; y++) {
            for(int x=0; x < MATRIX_WIDTH / N_COLORS; x++) {
                sand->setPosition(n++, xx + x, yy + y);
            }
        }
    }
    Serial.printf("%d total pixels\n", n);

    // EXACTLY like Adafruit example colors
    colors[0] = m_utils->hexToRgb565(0x565656);      // DeepGrey #565656
    colors[1] = m_utils->hexToRgb565(0xFF8000);      // Brown #FF8000
    colors[2] = m_utils->hexToRgb565(0x909090);      // LightGrey #909090
    colors[3] = m_utils->hexToRgb565(0x00FFFF);      // Cyan #00FFFF
    colors[4] = m_utils->hexToRgb565(0xFFED00);      // Yellow #FFED00
    colors[5] = m_utils->hexToRgb565(0x008024);      // Green #008024
    colors[6] = m_utils->hexToRgb565(0xCDCDCD);      // LightGrey #CDCDCD
    colors[7] = m_utils->hexToRgb565(0x75FF00);      // LightGreen #75FF00

    // Set independent sandbox mode based on the value from config.h
    this->independantSandboxEnable = INDEPENDENT_SANDBOX_ENABLE;

    Serial.println("Countdown mode setup complete");
}

void ModeCountdown::run() {
    if (this->independantSandboxEnable) { // Use class member
        // Independent (blocking) sandbox loop
        Serial.println("Entering independent sandbox loop (blocking).");
        while (true) {
            uint32_t t;
            while(((t = micros()) - prevTime) < (1000000L / MAX_FPS));
            prevTime = t;
            sandBox(); // Blocking call
            if (m_utils->isButtonDownPressed()) {
                Serial.println("ButtonDown pressed. Exiting independent sandbox loop.");
                this->independantSandboxEnable = false; // Use class member: Disable independent mode
                break; // Exit the loop
            }
        }
    } else {
        // Integrated (non-blocking) sandbox with frame rate limit

        // Limit the animation frame rate to MAX_FPS - EXACTLY like Adafruit example
        uint32_t t;
        while(((t = micros()) - prevTime) < (1000000L / MAX_FPS));
        prevTime = t;
        sandBox();
        if (m_utils->isButtonUpPressed()) {     // Not usefule because we are in non-blocking mode. The mode change will be first checked.
            Serial.println("ButtonUp pressed. Exiting independent sandbox loop.");
            this->independantSandboxEnable = true; // Use class member: Disable independent mode
        }
    }
}

void ModeCountdown::cleanup() {
    Serial.println("Countdown mode cleanup. Stopping sandbox if active.");
    if (sand) {
        delete sand;
        sand = nullptr;
    }
    this->independantSandboxEnable = false; // Use class member: Ensure disabled
    m_matrix->fillScreen(0);
}

void ModeCountdown::sandBox() {
    if (!sand || !m_matrix) return;
    
    // Read accelerometer - EXACTLY like Adafruit example
    sensors_event_t event;
    accel.getEvent(&event);

    if (useTextBox) {
        // Display accelerometer values (for debugging)
        m_matrix->fillRect(0, 0, MATRIX_WIDTH, 8, 0); // Clear text area
        m_matrix->setFont(); // Use system font
        m_matrix->setTextWrap(false);
        m_matrix->setCursor(1, 1);
        m_matrix->setTextColor(0xFFFF);
        m_matrix->setTextSize(1);
        
        char accel_str[32];
        sprintf(accel_str, "%.1f %.1f %.1f", event.acceleration.x, event.acceleration.y, event.acceleration.z);
        m_matrix->print(accel_str);
    }

    // EXACTLY like Adafruit example - but fix X axis direction
    double xx, yy, zz;
    xx = -event.acceleration.x * 2000; // Flip X axis and increase sensitivity (adafruit example uses 1000)
    yy = event.acceleration.y * 2000;
    zz = event.acceleration.z * 2000;  

    // Run one frame of the simulation - EXACTLY like Adafruit example
    sand->iterate(xx, yy, zz);

    // Clear the area where sand will be drawn
    uint8_t sand_draw_offset_y = useTextBox ? 8 : 0;
    m_matrix->fillRect(0, sand_draw_offset_y, MATRIX_WIDTH, sandSimHeight, 0);
    
    dimension_t sim_x, sim_y;
    for(int i=0; i<N_GRAINS; i++) {
        sand->getPosition(i, &sim_x, &sim_y);
        
        // sim_y will have values from 0 to sandSimHeight-1
        // Assume PixelDust library returns valid coordinates
        // Assume X-axis correction with (sim_x + 1) % MATRIX_WIDTH is already applied
        if(sim_x >= 0 && sim_x < MATRIX_WIDTH && sim_y >= 0 && sim_y < sandSimHeight) {
            // Apply global X_OFFSET for physical display and wrap around
            dimension_t screen_x_corrected = setPhysicalX(sim_x);
            dimension_t screen_y_to_draw = sim_y + sand_draw_offset_y;

            // Safety check: ensure screen_y_to_draw does not exceed actual matrix height
            if (screen_y_to_draw < MATRIX_HEIGHT) {
                int n = i / ((MATRIX_WIDTH / N_COLORS) * BOX_HEIGHT); // Color index
                if(n >= N_COLORS) n = N_COLORS - 1; // Safety check
                uint16_t flakeColor = colors[n];
                m_matrix->drawPixel(screen_x_corrected, screen_y_to_draw, flakeColor);
            }
        }
    }
    
    m_utils->displayShow(); // DMA equivalent of matrix.show()
}

// Keep existing methods
void ModeCountdown::startCountdown(unsigned long durationMs) {
    countdownStart = millis();
    countdownDuration = durationMs;
}

void ModeCountdown::setCountdown(int minutes, int seconds) {
    countdownDuration = (minutes * 60 + seconds) * 1000;
    countdownStart = millis();
}

bool ModeCountdown::isCountdownFinished() {
    unsigned long elapsed = millis() - countdownStart;
    return elapsed >= countdownDuration;
}

void ModeCountdown::drawSandClock() {
    // Keep existing implementation
}

void ModeCountdown::drawTextCountdown() {
    // Keep existing implementation
}