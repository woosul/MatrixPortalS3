/**
 * @file utils.h
 * @brief Core utility class providing system-wide functionality for LED matrix applications
 * 
 * The Utils class serves as the central hub for common functionality used across all display modes
 * and system components. It provides hardware abstraction, display management, user input handling,
 * audio feedback, text rendering, and various utility functions essential for the ESP32 Matrix Portal S3.
 * 
 * Key responsibilities:
 * - Hardware initialization and GPIO management
 * - LED matrix display control and visual effects
 * - Button input processing with debouncing
 * - Audio feedback system with volume control
 * - Text rendering and positioning utilities
 * - Color conversion and management
 * - System information and timing functions
 * - Debug and diagnostic utilities
 * 
 * This class is designed to be instantiated once and shared across all display modes,
 * providing a consistent interface for common operations while maintaining hardware
 * abstraction and optimized performance.
 */

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config.h"
#include <Wire.h>
#include "font_manager.h"
#include "text_renderer.h"

/**
 * @brief Button action enumeration for user input events
 * 
 * Defines the types of button actions that can be triggered by physical
 * GPIO buttons. Used for mode switching and navigation throughout the system.
 */
enum class ButtonAction { 
    NONE,           // No button action pending
    NEXT_MODE,      // Navigate to next display mode  
    PREV_MODE       // Navigate to previous display mode
};

/**
 * @brief Core utility class providing system-wide functionality
 * 
 * The Utils class encapsulates all common functionality needed across display modes
 * and system components. It manages hardware interfaces, provides display utilities,
 * handles user input, and offers various helper functions for text rendering,
 * color management, and system operations.
 * 
 * Design Philosophy:
 * - Single point of access for common functionality
 * - Hardware abstraction for easy portability
 * - Optimized performance for real-time display operations
 * - Comprehensive error handling and validation
 * - Modular design with clear separation of concerns
 */
class Utils {
private:
    // Core system references
    MatrixPanel_I2S_DMA* m_matrix;         // LED matrix display driver reference
    
    // Button input state management
    unsigned long lastButtonCheck;         // Timestamp of last button state check
    bool lastButtonUpState;                // Previous state of UP button
    bool lastButtonDownState;              // Previous state of DOWN button
    ButtonAction pendingButtonAction;      // Queued button action for processing
    
    // Audio system state
    bool buzzerEnabled;                    // Global audio feedback enable flag
    uint8_t buzzerVolume;                  // Audio volume level (1-100)
    
    // Integrated helper subsystems
    FontManager fontManagerInstance;       // Font management and selection
    TextRenderer textRendererInstance;     // Advanced text rendering with scrolling
    
    // Private hardware initialization methods
    /**
     * @doc Initializes LED matrix display hardware with configured parameters.
     * 
     * Internal method for setting up the HUB75 interface with proper pin assignments,
     * timing parameters, and buffer configuration. Called during main setup process.
     */
    void setupDisplay();

public:
    // Construction and initialization
    /**
     * @doc Constructs Utils instance with default configuration.
     * 
     * Initializes all member variables to safe default states. Hardware
     * initialization is deferred to the setup() method which requires
     * the display driver reference.
     */
    Utils();
    
    /**
     * @doc Initializes all hardware interfaces and subsystems.
     * 
     * Performs complete system initialization including GPIO configuration,
     * I2C setup, display testing, and subsystem preparation. This must be
     * called before using any other Utils functionality.
     * 
     * @param matrix_ptr Pointer to initialized LED matrix display driver
     */
    void setup(MatrixPanel_I2S_DMA* matrix_ptr);

    // --- DISPLAY MANAGEMENT AND VISUAL EFFECTS ---
    
    /**
     * @doc Executes basic RGB color sequence test on LED matrix.
     * 
     * Displays primary colors (red, green, blue) in sequence to verify
     * display functionality and color accuracy. Used for initial hardware
     * validation and troubleshooting display issues.
     */
    void displayColorTest();
    
    /**
     * @doc Executes comprehensive color gradient test with smooth transitions.
     * 
     * Performs advanced color testing with smooth gradients through the full
     * color spectrum. Includes fade effects and timing validation for display
     * performance testing and calibration.
     * This function is more complex than displayColorTest and provides, 
     * but needs more time and power resources. (e.g., over 20W, DC 5V 4A)
     */
    void displayColorTest2();
    
    /**
     * @doc Retrieves short display name for specified mode.
     * 
     * Returns abbreviated mode names suitable for display on small screens.
     * Used for mode preview displays and status indicators.
     * 
     * @param mode Display mode to get name for
     * @return Pointer to null-terminated mode name string
     */
    const char* getShortModeName(DisplayMode mode);
    
    /**
     * @doc Displays mode preview screen with mode number and name.
     * 
     * Shows a brief preview screen when switching between display modes,
     * providing visual feedback about the new mode selection. Display
     * duration is controlled by MODE_PREVIEW_TIME configuration.
     * 
     * @param mode Display mode to preview
     * @param modeName Human-readable mode name
     */
    void showModePreview(DisplayMode mode, const char* modeName);

    // --- COLOR CONVERSION AND MANAGEMENT ---
    
    /**
     * @doc Converts 24-bit hex color to 16-bit RGB565 format.
     * 
     * Transforms standard hex color values (0xRRGGBB) to the RGB565 format
     * required by the LED matrix display. Handles color depth reduction
     * with proper bit shifting and masking.
     * 
     * @param hexColor 24-bit hex color value (e.g., 0xFF0000 for red)
     * @return 16-bit RGB565 color value for display hardware
     */
    static uint16_t hexToRgb565(uint32_t hexColor);
    
    /**
     * @doc Converts 24-bit RGB888 color to 16-bit RGB565 format.
     * 
     * Transforms full 24-bit RGB color values to the compressed RGB565
     * format used by the display hardware. Provides color conversion
     * with optimized bit manipulation.
     * 
     * @param rgb888 24-bit RGB color value
     * @return 16-bit RGB565 color value
     */
    static uint16_t rgb888to565(uint32_t rgb888);
    
    /**
     * @doc Converts individual RGB components to RGB565 format.
     * 
     * Combines separate red, green, and blue color components into
     * the RGB565 format required by the display. Useful for dynamic
     * color generation and mathematical color operations.
     * 
     * @param r Red component (0-255)
     * @param g Green component (0-255)  
     * @param b Blue component (0-255)
     * @return 16-bit RGB565 color value
     */
    static uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b);

    // --- USER INPUT AND BUTTON MANAGEMENT ---
    
    /**
     * @doc Retrieves and clears any pending button action.
     * 
     * Returns the next queued button action and clears it from the internal
     * queue. This implements a non-blocking button event system that prevents
     * action loss and provides clean event handling.
     * 
     * @return ButtonAction enum value (NONE if no action pending)
     */
    ButtonAction getButtonAction();
    
    /**
     * @doc Processes button state changes with debouncing logic.
     * 
     * Monitors GPIO button states, applies debouncing to prevent false
     * triggers, and queues appropriate actions for consumption. Should be
     * called regularly from the main loop for responsive input handling.
     */
    void handleButtons();
    
    /**
     * @doc Checks current state of UP navigation button.
     * 
     * @return true if UP button is currently pressed (active low)
     */
    bool isButtonUpPressed();
    
    /**
     * @doc Checks current state of DOWN navigation button.
     * 
     * @return true if DOWN button is currently pressed (active low)
     */
    bool isButtonDownPressed();

    // --- AUDIO FEEDBACK SYSTEM ---
    
    /**
     * @doc Checks if audio feedback is currently enabled.
     * 
     * @return true if buzzer audio feedback is enabled
     */
    bool isSoundFeedbackEnabled() const { return buzzerEnabled; }
    
    /**
     * @doc Sets audio feedback volume level with validation.
     * 
     * Adjusts the buzzer volume with automatic range clamping.
     * Volume affects the PWM duty cycle for hardware volume control.
     * 
     * @param volume Volume level (1-100, automatically clamped)
     */
    void setBuzzerVolume(uint8_t volume);
    
    /**
     * @doc Enables or disables audio feedback system.
     * 
     * Controls global audio feedback state with confirmation tone.
     * When enabled, plays high tone; when disabled, plays low tone.
     * 
     * @param enable true to enable audio feedback, false to disable
     */
    void enableBuzzer(bool enable);
    
    /**
     * @doc Generates tone with specified frequency and duration.
     * 
     * Core audio generation function using software PWM with volume control.
     * Respects global buzzer enable state and volume settings.
     * 
     * @param frequency Tone frequency in Hz (higher = more shrill)
     * @param duration Tone duration in milliseconds
     */
    void playBuzzer(int frequency, int duration);
    
    /**
     * @doc Plays musical sequence with specified notes and timing.
     * 
     * Executes a sequence of musical notes with individual durations.
     * Supports rests (frequency = 0) and automatic inter-note spacing.
     * 
     * @param notes Array of frequencies in Hz (0 = rest)
     * @param durations Array of note durations in milliseconds
     * @param length Number of notes in the sequence
     */
    void playMelody(int* notes, int* durations, int length);
    
    /**
     * @doc Plays ascending musical scale for system startup.
     * 
     * Generates a pleasant ascending tone sequence (C5-C6) typically
     * used during system initialization to indicate successful startup.
     */
    void playScaleTone();
    
    /**
     * @doc Plays attention-getting notification sequence.
     * 
     * Generates a distinctive three-tone sequence designed to draw
     * attention for important notifications or alerts.
     */
    void playNotiTone();
    
    /**
     * @doc Plays brief confirmation tone for user actions.
     */
    void playSingleTone();
    
    /**
     * @doc Plays double confirmation tone for special events.
     */
    void playDoubleTone();
    
    /**
     * @doc Plays low-frequency error indication sequence.
     */
    void playErrorTone();

    // --- STATUS INDICATION ---
    
    /**
     * @doc Controls status LED for visual system feedback.
     * 
     * Manages the onboard status LED for indicating system state,
     * network connectivity, or other status information.
     * 
     * @param state true to turn LED on, false to turn off
     */
    void setStatusLED(bool state);

    // --- BASIC DISPLAY OPERATIONS ---
    
    /**
     * @doc Commits display buffer to LED matrix hardware.
     * 
     * Triggers the DMA buffer flip to make all pending drawing operations
     * visible on the physical display. Essential for double-buffered operation.
     */
    void displayShow();
    
    /**
     * @doc Clears entire display to black and updates hardware.
     * 
     * Convenience method that fills the display buffer with black pixels
     * and immediately commits the changes to the hardware.
     */
    void displayClear();
    
    /**
     * @doc Adjusts overall display brightness level.
     * 
     * Controls the global brightness of the LED matrix display.
     * Higher values increase power consumption and visual intensity.
     * 
     * @param level Brightness level (0-255, where 255 is maximum)
     */
    void displayBrightness(uint8_t level);

    // --- ADVANCED TEXT RENDERING SYSTEM ---
    
    /**
     * @doc Renders text with comprehensive formatting options (String version).
     * 
     * Advanced text rendering function supporting multiple fonts, alignment
     * options, baseline control, and optional text wrapping. Provides complete
     * typography control for professional display presentation.
     * 
     * @param text String object containing text to render
     * @param x X coordinate for text positioning
     * @param y Y coordinate for text positioning  
     * @param color RGB565 color for text rendering
     * @param fontType Font selection from FontType enumeration
     * @param alignment Text alignment (LEFT, CENTER, RIGHT)
     * @param baseLine Baseline positioning mode
     * @param wrapWidth Maximum width for text wrapping (0 = no wrap)
     * @param xOffset Additional X coordinate offset
     * @param yOffset Additional Y coordinate offset
     */
    void drawText(const String& text, int x, int y, uint16_t color,
                  FontType fontType = FONT_PRIMARY,
                  TextAlignment alignment = ALIGN_LEFT,
                  TextBaseLine baseLine = BASELINE_GFX_DEFAULT,
                  int16_t wrapWidth = 0,
                  int16_t xOffset = 0, int16_t yOffset = 0);
    
    /**
     * @doc Renders text with comprehensive formatting options (C-string version).
     * 
     * C-string version of the advanced text rendering function. See String
     * version above for detailed parameter descriptions.
     */
    void drawText(const char* text, int x, int y, uint16_t color,
                  FontType fontType = FONT_PRIMARY,
                  TextAlignment alignment = ALIGN_LEFT,
                  TextBaseLine baseLine = BASELINE_GFX_DEFAULT,
                  int16_t wrapWidth = 0,
                  int16_t xOffset = 0, int16_t yOffset = 0);
    
    /**
     * @doc Renders wrapped text with automatic vertical scrolling capability.
     * 
     * Advanced text display function for long content that exceeds the display
     * area. Automatically wraps text and provides smooth vertical scrolling
     * animation for content that doesn't fit in the available space.
     * 
     * @param text Null-terminated string to display
     * @param x Starting X coordinate  
     * @param y Starting Y coordinate
     * @param maxWidth Maximum width before wrapping
     * @param color RGB565 text color
     * @param align Text alignment within the width
     * @param charSpacing Additional spacing between characters
     * @param lineSpacing Additional spacing between lines
     * @param isCustomFont true if using bitmap font, false for system font
     * @param displayAreaHeight Height of scrolling area (-1 for full display)
     */
    void drawWrappedTextAndScroll(const char* text, int x, int y, int maxWidth, uint16_t color,
                                  TextAlignment align = ALIGN_LEFT, int charSpacing = 1, int lineSpacing = 0,
                                  bool isCustomFont = false, int displayAreaHeight = -1);
    
    /**
     * @doc Updates vertical scrolling animation for wrapped text.
     * 
     * Advances the vertical scrolling animation by one frame. Should be
     * called regularly to maintain smooth scrolling motion for long text content.
     */
    void updateTextScroll();
    
    /**
     * @doc Stops any active text scrolling animation.
     * 
     * Immediately halts vertical scrolling and resets to the beginning
     * of the text content.
     */
    void stopTextScroll();
    
    /**
     * @doc Checks if text scrolling animation is currently active.
     * 
     * @return true if vertical scrolling is in progress
     */
    bool isTextScrolling() const;

    // --- TEXT MEASUREMENT AND POSITIONING UTILITIES ---
    
    /**
     * @doc Calculates X coordinate for horizontally centered text.
     * 
     * Computes the appropriate X coordinate to center text within a given
     * screen width. Handles text that exceeds the available width by
     * constraining to valid display boundaries.
     * 
     * @param text Null-terminated string to measure
     * @param screenWidth Available width for centering
     * @param offsetByMatrixX true to apply hardware X offset correction
     * @return X coordinate for centered text positioning
     */
    int calculateTextCenterX(const char* text, int screenWidth, bool offsetByMatrixX = true);
    
    /**
     * @doc Calculates baseline Y coordinate for top-aligned text positioning.
     * 
     * Converts a desired top edge position to the appropriate baseline
     * coordinate for font rendering. Essential for precise text positioning
     * with custom fonts that have varying baseline offsets.
     * 
     * @param text Sample text for font metrics calculation
     * @param desiredTopY Desired Y coordinate for text top edge
     * @param isCustomFont true if using bitmap font, false for system font
     * @return Calculated baseline Y coordinate
     */
    int calculateTextTopY(const char* text, int desiredTopY, bool isCustomFont = false);
    
    /**
     * @doc Sets cursor position with automatic hardware offset (system font).
     * 
     * Convenience method for cursor positioning with automatic application
     * of hardware coordinate corrections. Assumes system font positioning.
     * 
     * @param x Logical X coordinate
     * @param y Logical Y coordinate (top-based for system font)
     */
    void setCursorTopBased(int x, int y);
    
    /**
     * @doc Sets cursor position with font-aware coordinate calculation.
     * 
     * Advanced cursor positioning that handles both system and custom fonts
     * with appropriate coordinate transformations for each font type.
     * 
     * @param x Logical X coordinate
     * @param y Logical Y coordinate (top-based)
     * @param isCustomFont true for bitmap fonts, false for system font
     */
    void setCursorTopBased(int x, int y, bool isCustomFont);

    // --- TEXT SCROLLING STATUS ACCESSORS ---
    
    /**
     * @doc Gets current X position of scrolling text.
     * @return Current X coordinate of scrolling text content
     */
    int getScrollTextX() const;
    
    /**
     * @doc Gets current Y position of scrolling text.
     * @return Current Y coordinate of scrolling text content
     */
    int getScrollTextY() const;
    
    /**
     * @doc Gets height of displayable scrolling area.
     * @return Height in pixels of the scrolling display area
     */
    int getDisplayableScrollHeight() const;
    
    /**
     * @doc Gets maximum width of scrolling text content.
     * @return Maximum width in pixels of scrolling text
     */
    int getScrollTextMaxWidth() const;

    // --- SYSTEM INFORMATION AND TIMING ---
    
    /**
     * @doc Gets current time as formatted string (HH:MM).
     * @return Time string in 24-hour format, or "00:00" if NTP unavailable
     */
    String getCurrentTimeString();
    
    /**
     * @doc Gets current date as formatted string (YYYY-MM-DD).
     * @return ISO date string, or "1970-01-01" if NTP unavailable  
     */
    String getCurrentDateString();
    
    /**
     * @doc Gets system uptime as human-readable string.
     * @return Formatted uptime (e.g., "2d 3h", "45m 32s")
     */
    String getUptimeString();
    
    /**
     * @doc Gets comprehensive system information summary.
     * @return Multi-line string with hardware and memory details
     */
    String getSystemInfo();

    // --- DEBUG AND DIAGNOSTIC UTILITIES ---
    
    /**
     * @doc Logs display buffer state for debugging purposes.
     * 
     * Outputs debug information about the current display buffer state
     * for troubleshooting display synchronization and rendering issues.
     * 
     * @param context Descriptive context string for debug log identification
     */
    void debugBufferState(const char* context);
    
    /**
     * @doc Marks display corners with colored pixels for visual debugging.
     * 
     * Places distinctive colored pixels at each corner of the display
     * for visual verification of coordinate systems, buffer flipping,
     * and display hardware functionality.
     * 
     * @param context Descriptive context string for debug log identification
     */
    void markBufferCorners(const char* context);
};

#endif // UTILS_H
