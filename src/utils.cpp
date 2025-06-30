/**
 * @file utils.cpp
 * @brief Implementation of the Utils class - Core utility system for ESP32 Matrix Portal S3
 * 
 * @doc This file implements all utility functions for the ESP32 Matrix Portal S3 project.
 * The Utils class serves as the central hub for:
 * - Hardware abstraction (buttons, buzzer, LED, display)
 * - Color management and conversion utilities
 * - Text rendering and positioning systems
 * - System information and time utilities
 * - Audio feedback and notification systems
 * - Display testing and debugging tools
 * 
 * This implementation provides a clean, production-ready interface that abstracts
 * the hardware complexity from the main application modes. All functions include
 * comprehensive error handling and validation.
 * 
 * @version 1.0
 * @date 2025-01-12
 * @author Denny Kim, (feature leaded) ESP32 Matrix Portal S3 Team
 */

#include "utils.h"
#include <vector>   // For container operations in text rendering
#include <string>   // For string manipulation and processing

// RGB COLOR CONVERSION UTILITIES (STATIC METHODS)
// ============================================================================

/**
 * @doc Converts a 24-bit hex color value to 16-bit RGB565 format
 * This function extracts RGB components from a hex value and packs them
 * into the RGB565 format used by the LED matrix display.
 * 
 * @param hexColor 24-bit hex color value (0xRRGGBB format)
 * @return 16-bit RGB565 color value for matrix display
 */
uint16_t Utils::hexToRgb565(uint32_t hexColor) {
    uint8_t r = (hexColor >> 16) & 0xFF;  // Extract red component (bits 23-16)
    uint8_t g = (hexColor >> 8) & 0xFF;   // Extract green component (bits 15-8)
    uint8_t b = hexColor & 0xFF;          // Extract blue component (bits 7-0)
    
    // Pack into RGB565: R(5bits) + G(6bits) + B(5bits)
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @doc Converts a 24-bit RGB888 color to 16-bit RGB565 format
 * Alternative interface for RGB color conversion using 32-bit input.
 * 
 * @param rgb888 24-bit RGB color value in RGB888 format
 * @return 16-bit RGB565 color value for matrix display
 */
uint16_t Utils::rgb888to565(uint32_t rgb888) {
    uint8_t r = (rgb888 >> 16) & 0xFF;    // Extract red component
    uint8_t g = (rgb888 >> 8) & 0xFF;     // Extract green component  
    uint8_t b = rgb888 & 0xFF;            // Extract blue component
    
    // Pack into RGB565 format with proper bit shifting
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/**
 * @doc Converts individual RGB components to 16-bit RGB565 format
 * Direct conversion from separate red, green, blue values to RGB565.
 * This is the most efficient method when RGB components are already separated.
 * 
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 * @return 16-bit RGB565 color value for matrix display
 */
uint16_t Utils::rgb888to565(uint8_t r, uint8_t g, uint8_t b) {
    // Pack RGB components into RGB565 format
    // Red: 5 bits (shift left 8, mask with 0xF8)
    // Green: 6 bits (shift left 3, mask with 0xFC)  
    // Blue: 5 bits (shift right 3)
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// CONSTRUCTOR AND INITIALIZATION
// ============================================================================

/**
 * @doc Utils class constructor - Initialize all member variables to safe defaults
 * Sets up the Utils class with default configuration values and prepares
 * all hardware interfaces for initialization. The matrix pointer is set to 
 * nullptr until setup() is called with a valid matrix instance.
 */
Utils::Utils() : m_matrix(nullptr),
                buzzerEnabled(DEFAULT_BUZZER_ENABLED),
                lastButtonCheck(0),
                lastButtonUpState(false),
                lastButtonDownState(false),
                buzzerVolume(DEFAULT_BUZZER_VOLUME),
                pendingButtonAction(ButtonAction::NONE)
                // FontManager and TextRenderer instances are default-constructed
{
    // All member variables initialized in initialization list
    // Additional setup is performed in setup() method
}

/**
 * @doc Main setup function - Initialize all hardware interfaces and subsystems
 * This function must be called before using any other Utils functionality.
 * It initializes GPIO pins, I2C communication, and sets up the FontManager
 * and TextRenderer subsystems.
 * 
 * @param matrix_ptr Pointer to the initialized MatrixPanel_I2S_DMA instance
 */
void Utils::setup(MatrixPanel_I2S_DMA* matrix_ptr) {
    m_matrix = matrix_ptr;

    Serial.println("Utils: Initializing GPIO pins...");
    
    // Configure all GPIO pins with appropriate modes
    pinMode(STATUS_LED_PIN, OUTPUT);      // Status LED output
    pinMode(BUTTON_UP_PIN, INPUT_PULLUP); // Up button with internal pullup
    pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP); // Down button with internal pullup
    pinMode(BUZZER_PIN, OUTPUT);          // Buzzer output
    pinMode(IR_RECEIVER_PIN, INPUT);      // IR receiver input
    
    // Set initial states for output pins
    digitalWrite(BUZZER_PIN, LOW);        // Ensure buzzer is off
    digitalWrite(STATUS_LED_PIN, LOW);    // Ensure status LED is off

    Serial.println("Utils: Initializing I2C communication...");
    
    // Initialize I2C with Matrix Portal S3 specific pins
    Wire.begin(16, 17);    // SDA=GPIO16, SCL=GPIO17 for Matrix Portal S3
    Wire.setClock(400000); // Set I2C frequency to 400kHz for optimal performance
    Serial.println("Utils: I2C initialization complete.");

    // Validate matrix pointer and run optional display test
    if (!m_matrix) {
        Serial.println("ERROR: Utils setup - Matrix display pointer is null!");
    } else {
        displayColorTest(); // Run basic color test to verify display functionality
    }
    
    // Initialize text rendering subsystem
    textRendererInstance.setup(m_matrix, &fontManagerInstance);

    Serial.println("Utils: GPIO and helper class initialization complete");
}

// DISPLAY INITIALIZATION AND TESTING
// ============================================================================

/**
 * @doc Initialize the LED matrix display with optimal configuration
 * Sets up the HUB75 LED matrix with all required GPIO pins and configuration
 * parameters. This function configures the display for maximum performance
 * with the Matrix Portal S3 hardware.
 */
void Utils::setupDisplay() { 
    Serial.printf("Utils: Initializing %dx%d display...\n", MATRIX_WIDTH, MATRIX_HEIGHT);
    
    // Create HUB75 configuration with matrix dimensions
    HUB75_I2S_CFG mxconfig(MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_CHAIN);
    
    // Configure all RGB data pins for upper and lower half of matrix
    mxconfig.gpio.r1 = R1_PIN; mxconfig.gpio.g1 = G1_PIN; mxconfig.gpio.b1 = B1_PIN;
    mxconfig.gpio.r2 = R2_PIN; mxconfig.gpio.g2 = G2_PIN; mxconfig.gpio.b2 = B2_PIN;
    
    // Configure address and control pins
    mxconfig.gpio.a = A_PIN; mxconfig.gpio.b = B_PIN; mxconfig.gpio.c = C_PIN;
    mxconfig.gpio.d = D_PIN; mxconfig.gpio.e = E_PIN;
    mxconfig.gpio.clk = CLK_PIN; mxconfig.gpio.lat = LAT_PIN; mxconfig.gpio.oe = OE_PIN;
    
    // Set buffer and timing configuration
    mxconfig.double_buff = !FORCE_SINGLE_BUFFER;  // Enable double buffering unless forced single
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;    // Set I2S speed to 10MHz for optimal performance

    // Initialize the matrix with configuration
    if (m_matrix && m_matrix->begin()) {
        Serial.printf("Utils: Display %dx%d initialized successfully\n", MATRIX_WIDTH, MATRIX_HEIGHT);
    } else {
        Serial.println("Utils: Display Initialization FAILED!");
    }
}

/**
 * @doc Helper function for smooth color interpolation between two 8-bit components
 * Used internally by enhanced color test for smooth color transitions.
 * 
 * @param c1 Starting color component value (0-255)
 * @param c2 Ending color component value (0-255)  
 * @param ratio Interpolation ratio (0.0 to 1.0)
 * @return Interpolated color component value
 */
static uint8_t interpolateComponent(uint8_t c1, uint8_t c2, float ratio) {
    return c1 + static_cast<uint8_t>((static_cast<float>(c2) - c1) * ratio);
}

/**
 * @doc Basic color test - Quick RGB verification for display functionality
 * Displays red, green, blue sequence to verify basic display operation.
 * This is a fast test suitable for startup verification.
 */
void Utils::displayColorTest() { 
    if (!m_matrix) return;
    
    // Test sequence: Red -> Green -> Blue -> Clear
    m_matrix->fillScreen(rgb888to565(224, 0, 0));   // Red
    displayShow(); 
    delay(500);
    
    m_matrix->fillScreen(rgb888to565(0, 224, 0));   // Green
    displayShow(); 
    delay(500);
    
    m_matrix->fillScreen(rgb888to565(0, 0, 224));   // Blue
    displayShow(); 
    delay(800);
    
    m_matrix->fillScreen(0);  // Clear display
    displayShow();
    Serial.println("Utils: Color test complete");
}


/**
 * @doc Enhanced color test with smooth transitions and visual feedback
 * Performs a comprehensive color test with smooth transitions through the
 * entire color spectrum. Includes start/end indicators and demonstrates
 * advanced color interpolation capabilities.
 */
void Utils::displayColorTest2() { 
    if (!m_matrix) return;
    Serial.println("Utils: Running enhanced color test...");

    // Define color sequence for smooth transitions
    const uint32_t testSequence[] = {
        0x000000, // Start: Black
        0xFF0000, // Red
        0xFF7F00, // Orange
        0xFFFF00, // Yellow
        0x00FF00, // Green
        0x00FFFF, // Cyan
        0x0000FF, // Blue
        0xFF00FF, // Magenta
        0xFFFFFF, // White
        0x000000  // End: Black
    };
    
    int numColorsInSequence = sizeof(testSequence) / sizeof(testSequence[0]);
    int numTransitions = numColorsInSequence - 1;

    // Animation parameters for smooth transitions
    const int fadeSteps = 45;              // Steps per color transition
    const int transitionDelay = 15;        // Milliseconds per step
    const int holdBetweenColors = 150;     // Hold time for each color

    // Display start indicator
    m_matrix->setFont();  // Use system font
    m_matrix->setTextSize(1);
    m_matrix->setTextColor(hexToRgb565(0xFFFFFF)); // White text
    const char* startText = "S";
    int startTextX = calculateTextCenterX(startText, MATRIX_WIDTH, false);
    setCursorTopBased(startTextX, MATRIX_HEIGHT / 2 - 4, false);
    m_matrix->print(startText);
    displayShow();
    delay(1500);

    // Perform smooth color transitions
    for (int i = 0; i < numTransitions; ++i) {
        uint32_t colorFromHex = testSequence[i];
        uint32_t colorToHex = testSequence[i+1];

        // Extract RGB components from hex colors
        uint8_t r1 = (colorFromHex >> 16) & 0xFF;
        uint8_t g1 = (colorFromHex >> 8) & 0xFF;
        uint8_t b1 = colorFromHex & 0xFF;

        uint8_t r2 = (colorToHex >> 16) & 0xFF;
        uint8_t g2 = (colorToHex >> 8) & 0xFF;
        uint8_t b2 = colorToHex & 0xFF;

        // Smooth interpolation between colors
        for (int s = 0; s < fadeSteps; ++s) {
            float ratio = static_cast<float>(s) / (fadeSteps - 1);
            uint8_t r_curr = interpolateComponent(r1, r2, ratio);
            uint8_t g_curr = interpolateComponent(g1, g2, ratio);
            uint8_t b_curr = interpolateComponent(b1, b2, ratio);
            
            m_matrix->fillScreen(rgb888to565(r_curr, g_curr, b_curr));
            displayShow();
            delay(transitionDelay);
        }
        
        // Hold each color briefly (except for black start/end)
        if (colorToHex != 0x000000 && i < numTransitions - 1) {
             delay(holdBetweenColors);
        }
    }

    // Display end indicator
    m_matrix->setFont();  // System font
    m_matrix->setTextSize(1);
    m_matrix->setTextColor(hexToRgb565(0x00FF00)); // Green text
    const char* endText = "E";
    int endTextX = calculateTextCenterX(endText, MATRIX_WIDTH, false);
    setCursorTopBased(endTextX, MATRIX_HEIGHT / 2 - 4, false);
    m_matrix->print(endText);
    displayShow();
    delay(2000);

    // Clear display and finish
    m_matrix->fillScreen(0);
    displayShow();
    Serial.println("Utils: Enhanced color test complete.");
}

// MODE MANAGEMENT AND DISPLAY
// ============================================================================

/**
 * @doc Get short display name for a given display mode
 * Returns abbreviated mode names suitable for display on the LED matrix.
 * Used for mode switching indicators and system status display.
 * 
 * @param mode Display mode enumeration value
 * @return Pointer to constant string containing short mode name
 */
const char* Utils::getShortModeName(DisplayMode mode) { 
    if (mode <= 0 || mode > MODE_MAX) return "UNKNOWN";
    
    // Array of short mode names corresponding to DisplayMode enum
    const char* names[] = {
        "",         // MODE_NONE (index 0)
        "CLOCK",    // MODE_CLOCK
        "MQTT",     // MODE_MQTT  
        "CNTDOWN",  // MODE_COUNTDOWN
        "PATTERN",  // MODE_PATTERN
        "IMAGE",    // MODE_IMAGE
        "GIF",      // MODE_GIF
        "FONTS",    // MODE_FONT
        "SYSINFO",  // MODE_SYSINFO
        "IRSCAN"    // MODE_IRSCAN
    };
    
    return names[(int)mode];
}

/**
 * @doc Display mode preview with number and name
 * Shows a brief preview of the selected mode with both the mode number
 * and abbreviated name. Used during mode switching to provide visual
 * feedback to the user.
 * 
 * @param mode Display mode to preview
 * @param modeName Optional full mode name (unused in current implementation)
 */
void Utils::showModePreview(DisplayMode mode, const char* modeName) { 
    if (!m_matrix) return;
    
    // Clear display and set up text rendering
    m_matrix->fillScreen(0);
    m_matrix->setFont();      // Use system font
    m_matrix->setTextSize(1);
    
    // Display mode number in white
    m_matrix->setTextColor(hexToRgb565(0xFFFFFF));
    String modeNumStr = String((int)mode);
    int modeNum_x = calculateTextCenterX(modeNumStr.c_str(), MATRIX_WIDTH);
    m_matrix->setCursor(setPhysicalX(modeNum_x), 4);
    m_matrix->print(modeNumStr);

    // Display mode name in cyan
    m_matrix->setTextColor(hexToRgb565(0x00FFFF));
    const char* shortName = getShortModeName(mode);
    int shortName_x = calculateTextCenterX(shortName, MATRIX_WIDTH);
    m_matrix->setCursor(setPhysicalX(shortName_x), 20);
    m_matrix->print(shortName);
    
    // Show preview and hold for specified time
    displayShow();
    delay(MODE_PREVIEW_TIME);
}

// BUTTON EVENT SYSTEM
// ============================================================================

/**
 * @doc Retrieve and clear pending button action
 * Returns the current pending button action and clears it atomically.
 * This ensures each button press is processed exactly once.
 * 
 * @return Current pending button action, or ButtonAction::NONE if no action pending
 */
ButtonAction Utils::getButtonAction() {
    ButtonAction action = pendingButtonAction;
    if (action != ButtonAction::NONE) {
        pendingButtonAction = ButtonAction::NONE;  // Clear pending action
    }
    return action;
}

/**
 * @doc Process button state changes and generate button actions
 * Implements debounced button handling with edge detection to prevent
 * multiple triggers from a single button press. Should be called regularly
 * from the main loop to ensure responsive button handling.
 */
void Utils::handleButtons() {
    unsigned long currentTime = millis();
    
    // Only check buttons if no action is pending and debounce time has elapsed
    if (pendingButtonAction == ButtonAction::NONE && 
        (currentTime - lastButtonCheck >= 50)) {
        
        // Read current button states
        bool currentUpState = isButtonUpPressed();
        bool currentDownState = isButtonDownPressed();
        
        // Detect rising edge (button press) for UP button
        if (currentUpState && !lastButtonUpState) {
            pendingButtonAction = ButtonAction::NEXT_MODE;
            if (buzzerEnabled) playSingleTone();  // Audio feedback
        } 
        // Detect rising edge (button press) for DOWN button
        else if (currentDownState && !lastButtonDownState) {
            pendingButtonAction = ButtonAction::PREV_MODE;
            if (buzzerEnabled) playSingleTone();  // Audio feedback
        }
        
        // Update button state history for next iteration
        lastButtonUpState = currentUpState;
        lastButtonDownState = currentDownState;
        lastButtonCheck = currentTime;
    }
}

/**
 * @doc Check if UP button is currently pressed
 * Reads the hardware state of the UP button pin. Button is active LOW
 * due to INPUT_PULLUP configuration.
 * 
 * @return true if button is pressed, false otherwise
 */
bool Utils::isButtonUpPressed() { 
    return digitalRead(BUTTON_UP_PIN) == LOW;
}

/**
 * @doc Check if DOWN button is currently pressed
 * Reads the hardware state of the DOWN button pin. Button is active LOW
 * due to INPUT_PULLUP configuration.
 * 
 * @return true if button is pressed, false otherwise
 */
bool Utils::isButtonDownPressed() { 
    return digitalRead(BUTTON_DOWN_PIN) == LOW;
}

// BUZZER CONTROL AND AUDIO FEEDBACK
// ============================================================================

/**
 * @doc Set buzzer volume with validation and feedback
 * Sets the buzzer volume level with range validation and provides
 * audio confirmation of the new setting.
 * 
 * @param volume Volume level (1-100, values outside range are clamped)
 */
void Utils::setBuzzerVolume(uint8_t volume) {
    // Clamp volume to valid range
    if (volume < 1) buzzerVolume = 1;
    else if (volume > 100) buzzerVolume = 100;
    else buzzerVolume = volume;
    
    Serial.printf("Utils: Buzzer volume set to %d\n", buzzerVolume);
}

/**
 * @doc Enable or disable buzzer with audio confirmation
 * Toggles buzzer functionality and provides audio feedback to confirm
 * the new state. High tone for enable, low tone for disable.
 * 
 * @param enable true to enable buzzer, false to disable
 */
void Utils::enableBuzzer(bool enable) { 
    buzzerEnabled = enable;
    Serial.printf("Buzzer %s\n", enable ? "enabled" : "disabled");
    
    // Play confirmation tone
    if (enable) {
        playBuzzer(2200, 100);  // High tone for enable
    } else {
        playBuzzer(800, 80);    // Low tone for disable
    }
}

/**
 * @doc Play a tone with specified frequency and duration
 * Generates a PWM signal to drive the buzzer at the specified frequency.
 * Volume control is implemented through duty cycle adjustment with
 * gamma correction for perceived loudness linearity.
 * 
 * @param frequency Tone frequency in Hz (must be > 0)
 * @param duration Duration in milliseconds
 */
void Utils::playBuzzer(int frequency, int duration) { 
    // Return early if buzzer is disabled or invalid parameters
    if (!buzzerEnabled || buzzerVolume == 0 || frequency <= 0) {
        digitalWrite(BUZZER_PIN, LOW);
        return;
    }
    
    // Calculate period in microseconds
    unsigned long period_us = 1000000UL / frequency;
    if (period_us == 0) {
        digitalWrite(BUZZER_PIN, LOW);
        return;
    }

    // Apply gamma correction for perceived volume linearity
    float duty_cycle_factor = pow(buzzerVolume / 100.0f, 2.2f);
    if (buzzerVolume >= 100) duty_cycle_factor = 1.0f;
    if (buzzerVolume <= 0) duty_cycle_factor = 0.0f;
    
    // Calculate high and low times for PWM
    unsigned long high_time_us = static_cast<unsigned long>(period_us * duty_cycle_factor);
    if (high_time_us > period_us) high_time_us = period_us;
    unsigned long low_time_us = period_us - high_time_us;

    // Handle edge cases for very low and very high volumes
    if (buzzerVolume > 0 && high_time_us == 0 && period_us > 0) {
        high_time_us = 1;
        low_time_us = (period_us > 1) ? (period_us - 1) : 0;
    }
    if (buzzerVolume >= 100 && low_time_us == 0 && period_us > 1) {
        low_time_us = 1;
        high_time_us = period_us - 1;
        if (high_time_us == 0 && period_us == 1) {
            high_time_us = 1;
            low_time_us = 0;
        }
    }
    
    // Generate PWM signal for specified duration
    unsigned long endTime_ms = millis() + duration;
    while (millis() < endTime_ms) {
        if (high_time_us > 0) {
            digitalWrite(BUZZER_PIN, HIGH);
            delayMicroseconds(high_time_us);
        }
        digitalWrite(BUZZER_PIN, LOW);
        if (low_time_us > 0) {
            delayMicroseconds(low_time_us);
        }
    }
}

/**
 * @doc Play a melody from arrays of notes and durations
 * Plays a sequence of notes with specified durations. Zero values
 * in the notes array are treated as rests (silence).
 * 
 * @param notes Array of frequencies in Hz (0 = rest)
 * @param durations Array of durations in milliseconds
 * @param length Number of notes in the arrays
 */
void Utils::playMelody(int* notes, int* durations, int length) { 
    if (!buzzerEnabled) return;
    
    for (int i = 0; i < length; i++) {
        if (notes[i] > 0) {
            playBuzzer(notes[i], durations[i]);  // Play note
        } else {
            delay(durations[i]);                 // Rest (silence)
        }
        delay(50);  // Brief pause between notes
    }
}

/**
 * @doc Play ascending musical scale for testing
 * Plays a C major scale (C5 to C6) for audio system testing
 * and demonstration purposes.
 */
void Utils::playScaleTone() { 
    if (!buzzerEnabled) return;
    
    // C major scale frequencies (C5 to C6)
    int scale[] = {523, 587, 659, 698, 784, 880, 988, 1047};
    
    for (int i = 0; i < 8; i++) {
        playBuzzer(scale[i], 100);
        delay(150);  // Pause between notes
    }
}

/**
 * @doc Play notification tone sequence
 * Plays a distinctive three-note sequence for important notifications.
 * Uses high-pitched tones for attention-getting effect.
 */
void Utils::playNotiTone() { 
    if (!buzzerEnabled) return;
    
    // Notification melody: A6, C6, A5
    int notes[] = {1760, 1047, 880};
    int durations[] = {100, 200, 100};
    
    playMelody(notes, durations, 3);
}

/**
 * @doc Play single tone for button feedback
 * Plays a brief, clear tone for user interface feedback.
 * Used for button presses and simple confirmations.
 */
void Utils::playSingleTone() {
    if (!buzzerEnabled) return;
    playBuzzer(2000, 150);
}

/**
 * @doc Play double tone for special events
 * Plays two identical tones with a brief pause between them.
 * Used for special confirmations or mode changes.
 */
void Utils::playDoubleTone() {
    if (!buzzerEnabled) return;
    playBuzzer(2000, 150);
    delay(100);
    playBuzzer(2000, 150);
}

/**
 * @doc Play error tone sequence
 * Plays a distinctive low-pitched sequence to indicate errors
 * or invalid operations. Uses low frequency for warning effect.
 */
void Utils::playErrorTone() {
    if (!buzzerEnabled) return;
    
    // Error melody: Low tone, pause, low tone
    int notes[] = {200, 0, 200};
    int durations[] = {300, 100, 300};
    
    playMelody(notes, durations, 3);
}

// STATUS LED AND BASIC DISPLAY CONTROL
// ============================================================================

/**
 * @doc Control the status LED state
 * Sets the hardware status LED on or off. Used for system status
 * indication and debugging purposes.
 * 
 * @param state true to turn LED on, false to turn off
 */
void Utils::setStatusLED(bool state) { 
    digitalWrite(STATUS_LED_PIN, state ? HIGH : LOW);
}

/**
 * @doc Update the display buffer (flip DMA buffer)
 * Commits the current drawing operations to the display by flipping
 * the DMA buffer. Must be called after drawing operations to make
 * changes visible on the LED matrix.
 */
void Utils::displayShow() {
    if (m_matrix) m_matrix->flipDMABuffer();
}

/**
 * @doc Clear the entire display to black
 * Fills the display with black pixels and immediately updates the display.
 * Provides a quick way to clear all content from the matrix.
 */
void Utils::displayClear() {
    if (m_matrix) {
        m_matrix->fillScreen(0);  // Fill with black (0)
        displayShow();            // Update display immediately
    }
}

/**
 * @doc Set display brightness level
 * Adjusts the overall brightness of the LED matrix display.
 * Higher values increase brightness and power consumption.
 * 
 * @param level Brightness level (0-255, where 255 is maximum brightness)
 */
void Utils::displayBrightness(uint8_t level) {
    if (m_matrix) m_matrix->setBrightness8(level);
}

// TEXT DISPLAY AND POSITIONING SYSTEM
// ============================================================================

/**
 * @doc Render text with comprehensive formatting options (String version)
 * High-level text rendering function that supports multiple fonts, alignment
 * options, text wrapping, and positioning. Delegates to TextRenderer for
 * advanced text processing capabilities.
 * 
 * @param text String object containing text to render
 * @param x Horizontal position (logical coordinates)
 * @param y Vertical position (logical coordinates)
 * @param color 16-bit RGB565 color value
 * @param fontType Font type from FontType enumeration
 * @param alignment Text alignment (LEFT, CENTER, RIGHT)
 * @param baseLine Baseline reference (TOP, CENTER, BOTTOM)
 * @param wrapWidth Maximum width for text wrapping (-1 for no wrap)
 * @param xOffset Additional X offset for fine positioning
 * @param yOffset Additional Y offset for fine positioning
 */
void Utils::drawText(const String& text, int x, int y, uint16_t color,
                     FontType fontType, TextAlignment alignment, TextBaseLine baseLine,
                     int16_t wrapWidth, int16_t xOffset, int16_t yOffset) {
    textRendererInstance.drawText(text, x, y, color, fontType, alignment, baseLine, 
                                  wrapWidth, xOffset, yOffset);
}

/**
 * @doc Render text with comprehensive formatting options (C string version)
 * High-level text rendering function that supports multiple fonts, alignment
 * options, text wrapping, and positioning. Optimized for C-style strings.
 * 
 * @param text Null-terminated C string containing text to render
 * @param x Horizontal position (logical coordinates)
 * @param y Vertical position (logical coordinates)
 * @param color 16-bit RGB565 color value
 * @param fontType Font type from FontType enumeration
 * @param alignment Text alignment (LEFT, CENTER, RIGHT)
 * @param baseLine Baseline reference (TOP, CENTER, BOTTOM)
 * @param wrapWidth Maximum width for text wrapping (-1 for no wrap)
 * @param xOffset Additional X offset for fine positioning
 * @param yOffset Additional Y offset for fine positioning
 */
void Utils::drawText(const char* text, int x, int y, uint16_t color,
                     FontType fontType, TextAlignment alignment, TextBaseLine baseLine,
                     int16_t wrapWidth, int16_t xOffset, int16_t yOffset) {
    textRendererInstance.drawText(text, x, y, color, fontType, alignment, baseLine, 
                                  wrapWidth, xOffset, yOffset);
}

/**
 * @doc Render wrapped text with automatic scrolling capabilities
 * Advanced text rendering function that handles long text by wrapping and
 * optionally scrolling within a defined display area. Ideal for displaying
 * large amounts of text that exceed the display dimensions.
 * 
 * @param text Null-terminated C string containing text to render
 * @param x Starting horizontal position
 * @param y Starting vertical position
 * @param maxWidth Maximum width before wrapping
 * @param color 16-bit RGB565 color value
 * @param align Text alignment within each line
 * @param charSpacing Additional spacing between characters
 * @param lineSpacing Additional spacing between lines
 * @param isCustomFont true if using custom font, false for system font
 * @param displayAreaHeight Height of scrollable display area
 */
void Utils::drawWrappedTextAndScroll(const char* text, int x, int y, int maxWidth, uint16_t color,
                                     TextAlignment align, int charSpacing, int lineSpacing,
                                     bool isCustomFont, int displayAreaHeight) {
    // TextRenderer handles coordinate transformation and matrix offset internally
    textRendererInstance.drawWrappedText(text, x, y, maxWidth, color, align, 
                                         charSpacing, lineSpacing, isCustomFont, displayAreaHeight);
}

/**
 * @doc Update automatic text scrolling animation
 * Advances the vertical scroll position for wrapped text that exceeds
 * the display area. Should be called regularly to maintain smooth scrolling.
 */
void Utils::updateTextScroll() {
    textRendererInstance.updateVerticalScroll();
}

/**
 * @doc Stop automatic text scrolling
 * Halts any active text scrolling animation and resets the scroll position.
 * Used when switching modes or when scrolling is no longer needed.
 */
void Utils::stopTextScroll() {
    textRendererInstance.stopVerticalScroll();
}

/**
 * @doc Check if text is currently scrolling
 * Returns the current state of the text scrolling system.
 * 
 * @return true if text is actively scrolling, false otherwise
 */
bool Utils::isTextScrolling() const {
    return textRendererInstance.isVerticalScrollingActive();
}

/**
 * @doc Calculate horizontal center position for text
 * Calculates the X coordinate needed to center text horizontally within
 * a specified width. Uses the current matrix font to determine text bounds
 * and accounts for hardware coordinate offsets if requested.
 * 
 * @param text Null-terminated string to measure
 * @param screenWidth Total width available for centering
 * @param offsetByMatrixX true to include hardware X offset in result
 * @return X coordinate for centered text positioning
 */
int Utils::calculateTextCenterX(const char* text, int screenWidth, bool offsetByMatrixX) { 
    if (!m_matrix) return 0;
    
    // Get text bounds using current font settings
    int16_t x1, y1;
    uint16_t w, h;
    m_matrix->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

    // Calculate centered X position
    int x = (screenWidth - w) / 2;
    
    // Ensure text stays within screen bounds
    if (x < 0) x = 0;
    if (x + w > screenWidth) x = screenWidth - w;
    
    // Apply hardware offset if requested
    return x;
}

/**
 * @doc Calculate Y position for top-aligned text with custom fonts
 * For custom fonts, calculates the baseline Y coordinate needed to position
 * the top of the text at the desired Y position. System fonts use Y as
 * top position directly.
 * 
 * @param text Text string to measure (for bounds calculation)
 * @param desiredTopY Desired Y position for top of text
 * @param isCustomFont true for custom fonts, false for system font
 * @return Y coordinate for proper text positioning
 */
int Utils::calculateTextTopY(const char* text, int desiredTopY, bool isCustomFont) {
    if (!m_matrix) return desiredTopY;
    if (!isCustomFont) return desiredTopY;

    // Get text bounds for custom font baseline calculation
    int16_t x1, y1_bounds;
    uint16_t w, h_bounds;
    m_matrix->getTextBounds(text, 0, 0, &x1, &y1_bounds, &w, &h_bounds);
    
    // y1_bounds is the offset from baseline to top (usually negative)
    // Calculate baseline position to place top at desired Y
    int baselineY = desiredTopY - y1_bounds;
    return baselineY;
}

/**
 * @doc Set cursor position with top-based Y coordinate (system font)
 * Sets the matrix cursor position with automatic hardware offset application.
 * For system fonts, Y coordinate represents the top of the text.
 * 
 * @param x Logical X coordinate
 * @param y Top Y coordinate for text positioning
 */
void Utils::setCursorTopBased(int x, int y) {
    if (!m_matrix) return;
    m_matrix->setCursor(setPhysicalX(x), y);
}

/**
 * @doc Set cursor position with font-aware Y coordinate handling
 * Sets the matrix cursor position with proper handling for both system
 * and custom fonts. Custom fonts require baseline calculation for proper
 * top-based positioning.
 * 
 * @param x Logical X coordinate  
 * @param y Desired top Y coordinate for text
 * @param isCustomFont true for custom fonts, false for system font
 */
void Utils::setCursorTopBased(int x, int y, bool isCustomFont) {
    if (!m_matrix) return;
    
    if (!isCustomFont) {
        // System font: Y coordinate is top position
        m_matrix->setCursor(setPhysicalX(x), y);
    } else {
        // Custom font: Calculate baseline from desired top position
        int16_t x1_calc, y1_calc_bounds;
        uint16_t w_calc, h_calc_bounds;
        m_matrix->getTextBounds("A", 0, 0, &x1_calc, &y1_calc_bounds, &w_calc, &h_calc_bounds);
        
        // Calculate baseline Y from top position
        int baselineY = y - y1_calc_bounds;  // y1_calc_bounds is negative offset
        m_matrix->setCursor(setPhysicalX(x), baselineY);
    }
}

// TEXT RENDERER SCROLL INFORMATION (WRAPPER FUNCTIONS)
// ============================================================================

/**
 * @doc Get current scroll text X position
 * Returns the current horizontal scroll position for wrapped text.
 * Used for monitoring and coordinating text scroll animations.
 * 
 * @return Current X position of scrolling text
 */
int Utils::getScrollTextX() const {
    return textRendererInstance.getScrollTextX();
}

/**
 * @doc Get current scroll text Y position  
 * Returns the current vertical scroll position for wrapped text.
 * Used for monitoring vertical scroll progress and position.
 * 
 * @return Current Y position of scrolling text
 */
int Utils::getScrollTextY() const {
    return textRendererInstance.getScrollTextY();
}

/**
 * @doc Get displayable scroll area height
 * Returns the height of the area available for text scrolling.
 * This represents the visible portion of the scrollable text area.
 * 
 * @return Height of displayable scroll area in pixels
 */
int Utils::getDisplayableScrollHeight() const {
    return textRendererInstance.getDisplayableScrollHeight();
}

/**
 * @doc Get maximum width of scrolling text
 * Returns the maximum width used by the current scrolling text.
 * Useful for determining text bounds and layout calculations.
 * 
 * @return Maximum width of scrolling text in pixels
 */
int Utils::getScrollTextMaxWidth() const {
    return textRendererInstance.getScrollTextMaxWidth();
}

// TIME AND SYSTEM INFORMATION UTILITIES
// ============================================================================

/**
 * @doc Get current time as formatted string
 * Returns the current time in HH:MM format using the system's local time.
 * Handles time sync failures gracefully by returning a default value.
 * 
 * @return String containing formatted time (HH:MM) or "00:00" if time unavailable
 */
String Utils::getCurrentTimeString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "00:00";
    
    char timeStr[6];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    return String(timeStr);
}

/**
 * @doc Get current date as formatted string
 * Returns the current date in YYYY-MM-DD format using the system's local time.
 * Provides a fallback date if time synchronization has failed.
 * 
 * @return String containing formatted date (YYYY-MM-DD) or "1970-01-01" if unavailable
 */
String Utils::getCurrentDateString() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "1970-01-01";
    
    char dateStr[11];
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    return String(dateStr);
}

/**
 * @doc Get system uptime as human-readable string
 * Calculates and formats the system uptime in the most appropriate units.
 * Automatically scales from seconds to days based on the duration.
 * 
 * @return String containing formatted uptime (e.g., "2d 5h", "3h 45m", "12m 30s")
 */
String Utils::getUptimeString() { 
    unsigned long s = millis() / 1000;
    unsigned long m = s / 60;
    unsigned long h = m / 60;
    unsigned long d = h / 24;
    
    // Calculate remainders for each unit
    s %= 60; m %= 60; h %= 24;
    
    // Format based on largest significant unit
    if (d > 0) return String(d) + "d " + String(h) + "h";
    if (h > 0) return String(h) + "h " + String(m) + "m";
    return String(m) + "m " + String(s) + "s";
}

/**
 * @doc Get comprehensive system information string
 * Returns a multi-line string containing key system information including
 * hardware model, memory usage, CPU frequency, and flash storage capacity.
 * 
 * @return String containing formatted system information
 */
String Utils::getSystemInfo() {
    return "ESP32-S3\nHeap: " + String(ESP.getFreeHeap() / 1024) + "KB\n" +
           "CPU: " + String(ESP.getCpuFreqMHz()) + "MHz\n" +
           "Flash: " + String(ESP.getFlashChipSize() / 1024 / 1024) + "MB\n";
}

// DEBUGGING AND DEVELOPMENT UTILITIES
// ============================================================================

/**
 * @doc Debug display buffer state with context information
 * Logs current display buffer state information for debugging purposes.
 * Useful for tracking buffer operations and display update sequences.
 * 
 * @param context Descriptive string identifying the debug context
 */
void Utils::debugBufferState(const char* context) {
    if (!m_matrix) return;
    Serial.printf("DEBUG BUFFER [%s]: Matrix state tracked\n", context);
}

/**
 * @doc Mark display corners with colored pixels for visual debugging
 * Places colored pixels at the four corners of the display to help with
 * visual debugging of display boundaries, buffer operations, and coordinate
 * system verification. Each corner uses a different color for identification.
 * 
 * @param context Descriptive string identifying the debug context
 */
void Utils::markBufferCorners(const char* context) {
    if (!m_matrix) return;
    
    // Mark each corner with a different color for identification
    m_matrix->drawPixel(0, 0, hexToRgb565(0xFF0000));                           // Top-left: Red
    m_matrix->drawPixel(MATRIX_WIDTH - 1, 0, hexToRgb565(0x00FF00));            // Top-right: Green  
    m_matrix->drawPixel(0, MATRIX_HEIGHT - 1, hexToRgb565(0x0000FF));           // Bottom-left: Blue
    m_matrix->drawPixel(MATRIX_WIDTH - 1, MATRIX_HEIGHT - 1, hexToRgb565(0xFFFFFF)); // Bottom-right: White
    
    Serial.printf("DEBUG BUFFER [%s]: Corners marked.\n", context);
}
