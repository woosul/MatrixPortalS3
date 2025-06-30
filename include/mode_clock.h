/**
 * @file mode_clock.h
 * @brief Digital clock display mode with time-based color transitions
 * 
 * Provides a digital clock display with seconds progress bar, date information,
 * and dynamic color schemes that change based on time of day. Features NTP
 * synchronization, smooth color transitions, and optimized rendering performance.
 */

#ifndef MODE_CLOCK_H
#define MODE_CLOCK_H

#include <time.h>
#include <Arduino.h>
#include "config.h"

// Forward declarations to avoid circular dependencies
class Utils;
class MatrixPanel_I2S_DMA;

/**
 * @brief Digital clock display mode implementation
 * 
 * This class handles the display of digital time with the following features:
 * - Large digital time display with blinking colon separator
 * - Date display showing day of week and formatted date
 * - Animated seconds progress bar with color gradient
 * - Time-based color themes (morning, afternoon, evening, night)
 * - NTP synchronization with fallback display for sync failures
 * - Optimized rendering to minimize flicker and improve performance
 */
class ModeClock {
public:
    /**
     * @doc Initializes clock display mode with required dependencies.
     * 
     * Sets up the clock display mode with references to utilities and display
     * hardware. Performs initial time configuration check and prepares internal
     * state for time display operations.
     * 
     * @param utils_ptr Pointer to utilities object for display helpers
     * @param matrix_ptr Pointer to LED matrix display driver
     */
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr);
    
    /**
     * @doc Main execution loop for clock display updates.
     * 
     * Handles periodic time updates, NTP synchronization checks, and display
     * refresh operations. Updates display elements based on time changes and
     * manages rendering optimization to reduce unnecessary redraws.
     */
    void run();
    
    /**
     * @doc Cleans up clock mode resources and resets display.
     * 
     * Clears the display and resets internal state flags when exiting
     * clock mode. Prepares the mode for potential re-entry.
     */
    void cleanup();
    
private:
    // Core member variables
    Utils* m_utils;                         // Utilities object reference  
    MatrixPanel_I2S_DMA* m_matrix;          // Display driver reference
    
    // Timing and synchronization state
    unsigned long lastUpdate;               // Last display update timestamp
    unsigned long lastNTPSync;              // Last NTP synchronization attempt
    bool timeConfigured;                    // NTP time synchronization status
    bool isInitialDisplayDone;              // Initial display setup completion flag
    struct tm cachedTime;                   // Cached time structure for optimization
    
    // Display rendering methods
    /**
     * @doc Renders the main digital time display with blinking colon.
     */
    void drawDigitalTime();
    
    /**
     * @doc Renders the date information below the time display.
     */
    void drawDate();
    
    /**
     * @doc Renders animated seconds progress bar with color gradient.
     */
    void drawSecondBar();
    
    /**
     * @doc Optimized colon-only update for smooth blinking animation.
     */
    void drawColonOnly();
    
    /**
     * @doc Attempts NTP time synchronization with retry logic.
     */
    void syncNTPTime();
    
    // Time-based color management methods
    /**
     * @doc Gets current time-appropriate color based on hour of day.
     * 
     * @return RGB565 color value for current time period
     */
    uint16_t getTimeBasedColor();
    
    /**
     * @doc Determines color theme based on specific hour.
     * 
     * @param hour Hour value (0-23) to determine color theme
     * @return RGB565 color value for specified hour
     */
    uint16_t getCurrentTimeColor(int hour);
    
    /**
     * @doc Provides smooth color transitions near hour boundaries.
     * 
     * @param hour Current hour (0-23)
     * @param minute Current minute (0-59)
     * @return RGB565 color with smooth transition applied
     */
    uint16_t getTimeBasedColorSmooth(int hour, int minute);
    
    /**
     * @doc Logs color changes for debugging and monitoring.
     * 
     * @param hour Hour when color change occurred
     * @param color New RGB565 color value
     */
    void logColorChange(int hour, uint16_t color);
    
    // Color utility and conversion methods
    /**
     * @doc Converts 24-bit hex color to 16-bit RGB565 format.
     * 
     * @param hexColor 24-bit hex color value (0xRRGGBB)
     * @return 16-bit RGB565 color value
     */
    uint16_t hexToRgb565(uint32_t hexColor);
    
    /**
     * @doc Gets custom time-based color using specified hex values.
     * 
     * @param hour Current hour for time period determination
     * @param morningHex Hex color for morning period (06:00-11:59)
     * @param afternoonHex Hex color for afternoon period (12:00-17:59)
     * @param eveningHex Hex color for evening period (18:00-23:59)
     * @param nightHex Hex color for night period (00:00-05:59)
     * @return RGB565 color value for current time period
     */
    uint16_t getCustomTimeColor(int hour, uint32_t morningHex, 
                                uint32_t afternoonHex,
                                uint32_t eveningHex,
                                uint32_t nightHex);
    
    // Gradient and animation helper methods  
    /**
     * @doc Calculates gradient color for seconds progress bar.
     * 
     * @param position Current position in gradient (0 to totalWidth)
     * @param totalWidth Total width of gradient bar
     * @return RGB565 color value for specified position
     */
    uint16_t getGradientColor(int position, int totalWidth);
    
    /**
     * @doc Interpolates between two RGB565 colors.
     * 
     * @param color1 Starting RGB565 color
     * @param color2 Ending RGB565 color
     * @param ratio Interpolation ratio (0.0 to 1.0)
     * @return Interpolated RGB565 color value
     */
    uint16_t interpolateColor(uint16_t color1, uint16_t color2, float ratio);
    
    // Time period color constants
    static const uint16_t COLOR_ERROR;      // NTP sync failed - Red (#FF0000)
    static const uint16_t COLOR_MORNING;    // Morning period - Cyan (#00FFFF)
    static const uint16_t COLOR_AFTERNOON;  // Afternoon period - White (#FFFFFF)  
    static const uint16_t COLOR_EVENING;    // Evening period - Amber (#FFC000)
    static const uint16_t COLOR_NIGHT;      // Night period - Dark grey (#808080)
};

#endif