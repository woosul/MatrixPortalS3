/**
 * @file mode_clock.cpp
 * @brief Implementation of digital clock display mode
 * 
 * Provides comprehensive digital clock functionality with time-based color themes,
 * animated progress indicators, and optimized rendering for LED matrix displays.
 */

#include "mode_clock.h"
#include "font_manager.h"
#include "common.h"
#include "utils.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// Static color constants for time period themes
const uint16_t ModeClock::COLOR_ERROR = Utils::hexToRgb565(0xFF0000);      // Red for errors
const uint16_t ModeClock::COLOR_MORNING = Utils::hexToRgb565(0x00FFFF);    // Cyan for morning
const uint16_t ModeClock::COLOR_AFTERNOON = Utils::hexToRgb565(0xFFFFFF);  // White for afternoon
const uint16_t ModeClock::COLOR_EVENING = Utils::hexToRgb565(0xFFC000);    // Amber for evening
const uint16_t ModeClock::COLOR_NIGHT = Utils::hexToRgb565(0x808080);      // Grey for night

/**
 * @doc Initializes clock display mode with required dependencies.
 */
void ModeClock::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) {
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;
    lastUpdate = 0;
    lastNTPSync = 0;
    
    // Check current NTP synchronization status
    struct tm timeinfo;
    timeConfigured = getLocalTime(&timeinfo);
    isInitialDisplayDone = false;
    
    Serial.printf("Clock mode initialized - NTP status: %s\n", 
                  timeConfigured ? "synchronized" : "pending");
}
/**
 * @doc Cleans up clock mode resources and resets display.
 */
void ModeClock::cleanup() {
    // Clear matrix display
    if (m_matrix && m_utils) {
        m_matrix->fillScreen(0);
        m_utils->displayShow();
    }

    // Reset display state flag (preserve NTP sync status)
    isInitialDisplayDone = false;
}

/**
 * @doc Attempts NTP time synchronization with retry logic.
 */
void ModeClock::syncNTPTime() {
    int attempts = 0;
    while (attempts < 10) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            timeConfigured = true;
            lastNTPSync = millis();
            
            char timeStr[64];
            strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
            return;
        }
        
        delay(500);
        attempts++;
    }
    
    timeConfigured = false;
}

/**
 * @doc Renders the main digital time display with blinking colon.
 */
void ModeClock::drawDigitalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        timeConfigured = false;
        return;
    }

    if (!m_matrix || !m_utils) return;
    
    // Configure display font for time rendering
    m_matrix->setFont(FontManager::getFont(FontType::FONT_DIGITAL_DISMAY_R_12));
    m_matrix->setTextSize(1);
    m_matrix->setTextWrap(false);
    
    // Apply time-based color theme
    uint16_t timeColor = getTimeBasedColor();
    
    // Log color changes when hour transitions occur
    static int lastHour = -1;
    if (timeinfo.tm_hour != lastHour) {
        logColorChange(timeinfo.tm_hour, timeColor);
        lastHour = timeinfo.tm_hour;
    }
    
    m_matrix->setTextColor(timeColor);
    
    // Create time string with animated blinking colon
    char timeStr[9];
    bool showColon = (timeinfo.tm_sec % 2) == 0;
    char colonChar = showColon ? ':' : ' ';
    sprintf(timeStr, "%02d%c%02d", timeinfo.tm_hour, colonChar, timeinfo.tm_min);
    
    // Center the time display using utility functions
    int time_x = m_utils->calculateTextCenterX(timeStr, 64);
    m_utils->setCursorTopBased(time_x, 15, true);
    m_matrix->print(timeStr);
    
    // Reset to default font for subsequent operations
    m_matrix->setFont();
}

/**
 * @doc Renders the date information below the time display.
 */
void ModeClock::drawDate() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    if (!m_matrix || !m_utils) return;
    
    // Day and month abbreviation arrays for display formatting
    const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                            "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    
    // Configure text properties for date display
    m_matrix->setTextSize(1);
    m_matrix->setTextWrap(false);
    m_matrix->setTextColor(m_utils->hexToRgb565(0x565656));  // Deep grey color
    
    // Format date string: "DAY DD MON" (e.g., "MON 15 JUN")
    char dateStr[12];
    sprintf(dateStr, "%s %02d%s", days[timeinfo.tm_wday], timeinfo.tm_mday, months[timeinfo.tm_mon]);
    
    // Center the date display below the time
    int x = m_utils->calculateTextCenterX(dateStr, 64);
    m_utils->setCursorTopBased(x, 51, false);
    m_matrix->print(dateStr);
}

/**
 * @doc Renders animated seconds progress bar with color gradient.
 */
void ModeClock::drawSecondBar() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    if (!m_matrix || !m_utils) return;
    
    // Progress bar positioning and dimensions (60 pixels wide, centered)
    int startX = 2;
    int endX = 61;
    int barPixels = 60;
    
    // Calculate progress bar width based on current seconds (0-59)
    int barWidth = (timeinfo.tm_sec * barPixels) / 60;
    
    // Clear the entire progress bar area
    for (int x = startX; x <= endX; x++) {
        m_matrix->drawPixel(setPhysicalX(x), 62, m_utils->hexToRgb565(0x000000));
    }
    
    // Draw gradient-colored progress bar
    for (int x = 0; x < barWidth; x++) {
        uint16_t gradientColor = getGradientColor(x, barPixels);
        m_matrix->drawPixel(setPhysicalX(startX + x), 62, gradientColor);
    }
}

/**
 * @doc Determines color theme based on specific hour.
 */
uint16_t ModeClock::getCurrentTimeColor(int hour) {
    if (hour >= 6 && hour <= 11) {
        return COLOR_MORNING;    // Morning: 06:00-11:59 - Cyan
    } else if (hour >= 12 && hour <= 17) {
        return COLOR_AFTERNOON;  // Afternoon: 12:00-17:59 - White
    } else if (hour >= 18 && hour <= 23) {
        return COLOR_EVENING;    // Evening: 18:00-23:59 - Amber
    } else {
        return COLOR_NIGHT;      // Night: 00:00-05:59 - Dark grey
    }
}

/**
 * @doc Gets current time-appropriate color based on hour of day.
 */
uint16_t ModeClock::getTimeBasedColor() {
    if (!timeConfigured) {
        return COLOR_ERROR;      // NTP sync failed - Red
    }
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return COLOR_ERROR;      // Failed to get time - Red
    }
    
    return getCurrentTimeColor(timeinfo.tm_hour);
}

/**
 * @doc Provides smooth color transitions near hour boundaries.
 */
uint16_t ModeClock::getTimeBasedColorSmooth(int hour, int minute) {
    // Apply 15-minute early transition periods for smoother color changes
    if (hour == 5 && minute >= 45) {
        return COLOR_MORNING;    // Early morning transition at 05:45
    } else if (hour == 11 && minute >= 45) {
        return COLOR_AFTERNOON;  // Late morning transition at 11:45
    } else if (hour == 17 && minute >= 45) {
        return COLOR_EVENING;    // Late afternoon transition at 17:45
    } else if (hour == 23 && minute >= 45) {
        return COLOR_NIGHT;      // Late evening transition at 23:45
    }
    
    // Use standard time-based color for other periods
    return getCurrentTimeColor(hour);
}

/**
 * @doc Logs color changes for debugging and monitoring.
 */
void ModeClock::logColorChange(int hour, uint16_t color) {
    const char* colorName = "Unknown";
    
    if (color == COLOR_ERROR) colorName = "Red (Error)";
    else if (color == COLOR_MORNING) colorName = "Cyan (Morning)";
    else if (color == COLOR_AFTERNOON) colorName = "White (Afternoon)";
    else if (color == COLOR_EVENING) colorName = "Amber (Evening)";
    else if (color == COLOR_NIGHT) colorName = "Grey (Night)";
    
    Serial.printf("Clock: %02d:xx - Color changed to %s (0x%04X)\n", hour, colorName, color);
}

/**
 * @doc Converts 24-bit hex color to 16-bit RGB565 format.
 */
uint16_t ModeClock::hexToRgb565(uint32_t hexColor) {
    // Delegate to utilities object for color conversion
    if (m_utils) return m_utils->hexToRgb565(hexColor);
    return Utils::hexToRgb565(hexColor); // Fallback to static method
}

/**
 * @doc Gets custom time-based color using specified hex values.
 */
uint16_t ModeClock::getCustomTimeColor(int hour, uint32_t morningHex, 
                                       uint32_t afternoonHex,
                                       uint32_t eveningHex,
                                       uint32_t nightHex) {
    if (!m_utils) return COLOR_ERROR;

    if (hour >= 6 && hour <= 11) {
        return m_utils->hexToRgb565(morningHex);
    } else if (hour >= 12 && hour <= 17) {
        return m_utils->hexToRgb565(afternoonHex);
    } else if (hour >= 18 && hour <= 23) {
        return m_utils->hexToRgb565(eveningHex);
    } else {
        return m_utils->hexToRgb565(nightHex);
    }
}

/**
 * @doc Main execution loop for clock display updates.
 */
void ModeClock::run() {
    unsigned long currentTime = millis();
    if (!m_matrix || !m_utils) return;

    // Periodic NTP synchronization check for unconfigured time
    // Note: Main ensureNetworkAndTime() handles primary NTP sync
    if (!timeConfigured && (currentTime - lastNTPSync >= 60000)) {
        syncNTPTime();
        lastNTPSync = currentTime;
    }
    
    static int lastMinute = -1;
    struct tm timeinfo;
    
    // Update display every second if time is available
    if (currentTime - lastUpdate >= 1000 && getLocalTime(&timeinfo)) {
        if (timeConfigured) {
            // Full redraw on minute changes or initial display
            if (timeinfo.tm_min != lastMinute || !isInitialDisplayDone) {
                m_matrix->fillScreen(0);
                drawDigitalTime();
                drawDate();
                drawSecondBar();
                m_utils->displayShow();
                lastMinute = timeinfo.tm_min;
                isInitialDisplayDone = true;
            } else {
                // Optimized update: only redraw time (with blinking colon) and seconds bar
                m_matrix->fillScreen(0);
                drawDigitalTime();
                drawDate();
                drawSecondBar();
                m_utils->displayShow();
                isInitialDisplayDone = true;
            }
        } else {
            // Display placeholder when time sync is unavailable
            m_matrix->fillScreen(0);
            m_matrix->setFont();
            m_matrix->setTextSize(1);
            m_matrix->setTextColor(COLOR_ERROR);
            int syncMsgX = m_utils->calculateTextCenterX("--:--", MATRIX_WIDTH);
            m_utils->setCursorTopBased(syncMsgX, 14, false);
            m_matrix->print("--:--");
            m_utils->displayShow();
            isInitialDisplayDone = true;
        }
        lastUpdate = currentTime;
    }
}/**
 * @doc Optimized colon-only update for smooth blinking animation.
 */
void ModeClock::drawColonOnly() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;

    if (!m_matrix || !m_utils) return;
    
    // Configure font to match main time display
    m_matrix->setFont(FontManager::getFont(FontType::FONT_AGENCY_R_12));
    
    // Calculate colon position in time string
    char hourStr[3];
    sprintf(hourStr, "%02d", timeinfo.tm_hour);
    
    int16_t x1, y1;
    uint16_t w, h;
    m_matrix->getTextBounds(hourStr, 0, 0, &x1, &y1, &w, &h);
    
    // Position colon after hour digits
    int colonX = (64 - w) / 2 + w;
    
    // Apply blinking logic and time-based color
    bool showColon = (timeinfo.tm_sec % 2) == 0;
    uint16_t timeColor = getTimeBasedColor();
    
    m_matrix->setTextColor(showColon ? timeColor : 0x0000);
    m_matrix->setCursor(setPhysicalX(colonX), 33);
    m_matrix->print(':');
    
    // Reset to default font
    m_matrix->setFont();
}

/**
 * @doc Calculates gradient color for seconds progress bar.
 */
uint16_t ModeClock::getGradientColor(int position, int totalWidth) {
    // Five-color gradient: Blue → Light Green → Yellow → Orange → Red
    // Each transition segment occupies 25% of the total width
    float ratio = (float)position / (float)totalWidth;

    if (!m_utils) return 0;
    
    // Define gradient color sequence
    uint16_t blue = m_utils->hexToRgb565(0x0000FF);        // Blue
    uint16_t lightGreen = m_utils->hexToRgb565(0x00FF80);  // Light Green
    uint16_t yellow = m_utils->hexToRgb565(0xFFFF00);      // Yellow
    uint16_t orange = m_utils->hexToRgb565(0xFF8000);      // Orange
    uint16_t red = m_utils->hexToRgb565(0xFF0000);         // Red
    
    // Determine gradient segment and interpolate within it
    if (ratio <= 0.25) {
        // Blue to Light Green (0-25%)
        float segmentRatio = ratio / 0.25;
        return interpolateColor(blue, lightGreen, segmentRatio);
    } else if (ratio <= 0.5) {
        // Light Green to Yellow (25-50%)
        float segmentRatio = (ratio - 0.25) / 0.25;
        return interpolateColor(lightGreen, yellow, segmentRatio);
    } else if (ratio <= 0.75) {
        // Yellow to Orange (50-75%)
        float segmentRatio = (ratio - 0.5) / 0.25;
        return interpolateColor(yellow, orange, segmentRatio);
    } else {
        // Orange to Red (75-100%)
        float segmentRatio = (ratio - 0.75) / 0.25;
        return interpolateColor(orange, red, segmentRatio);
    }
}

/**
 * @doc Interpolates between two RGB565 colors.
 */
uint16_t ModeClock::interpolateColor(uint16_t color1, uint16_t color2, float ratio) {
    // Clamp ratio to valid range
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    
    // Extract RGB components from RGB565 format
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    // Interpolate each RGB component
    uint8_t r = r1 + (uint8_t)((r2 - r1) * ratio);
    uint8_t g = g1 + (uint8_t)((g2 - g1) * ratio);
    uint8_t b = b1 + (uint8_t)((b2 - b1) * ratio);
    
    // Combine components back to RGB565 format
    return (r << 11) | (g << 5) | b;
}