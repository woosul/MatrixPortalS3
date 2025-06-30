/**
 * @file config.h
 * @brief Hardware and application configuration definitions
 * 
 * Contains all hardware pin assignments, network settings, display parameters,
 * IR remote codes, and application behavior configurations for the ESP32 Matrix Portal S3.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <Arduino.h>

// --- DEVICE AND NETWORK SETTINGS ---

// Basic device identification
#define DEVICE_NAME "ESP32S3 MatrixPortal"  // Device name for logging and identification
#define DEVICE_VERSION "1.0.0"              // Current firmware version


// WiFi connection behavior
#define WIFI_USE_BEST_SIGNAL true           // Enable automatic selection of strongest signal AP

// Network Time Protocol servers for time synchronization
#define NTP_SERVER_1 "pool.ntp.org"         // Primary NTP server
#define NTP_SERVER_2 "time.google.com"      // Secondary NTP server
#define NTP_SERVER_3 "time.windows.com"     // Tertiary NTP server
#define GMT_OFFSET_SEC 32400                // GMT+9 for Korea Standard Time
#define DAYLIGHT_OFFSET_SEC 0               // No daylight saving in Korea

// --- DISPLAY HARDWARE CONFIGURATION ---

// LED matrix panel specifications
#define MATRIX_WIDTH 64                     // Panel width in pixels
#define MATRIX_HEIGHT 64                    // Panel height in pixels
#define MATRIX_CHAIN 1                      // Number of chained panels
#define MATRIX_COLOR_DEPTH 6                // Color depth in bits (affects color quality vs performance)
#define MATRIX_X_OFFSET 1                   // Global X-axis coordinate correction offset

// Global X-axis coordinate correction offset by MATRIX_X_OFFSET
inline int setPhysicalX(int logicalX) { int physicalX = (logicalX + MATRIX_X_OFFSET) % MATRIX_WIDTH;  return physicalX; }

// ESP32-S3 MatrixPortal HUB75 interface pin assignments
#define R1_PIN 42                           // Red channel 1 data pin
#define G1_PIN 41                           // Green channel 1 data pin
#define B1_PIN 40                           // Blue channel 1 data pin
#define R2_PIN 38                           // Red channel 2 data pin
#define G2_PIN 39                           // Green channel 2 data pin
#define B2_PIN 37                           // Blue channel 2 data pin
#define A_PIN 45                            // Address line A
#define B_PIN 36                            // Address line B
#define C_PIN 48                            // Address line C
#define D_PIN 35                            // Address line D
#define E_PIN 21                            // Address line E (for 64x64 panels)
#define LAT_PIN 47                          // Latch signal pin
#define OE_PIN 14                           // Output enable pin
#define CLK_PIN 2                           // Clock signal pin

// --- ANIMATION AND PERFORMANCE SETTINGS ---

// Animation frame rate configuration
#ifndef ANIMATION_FPS
#define ANIMATION_FPS 45                    // Default animation frames per second
#endif

// Display buffer configuration
#define FORCE_SINGLE_BUFFER false           // Use double buffering for smoother display

// Sandbox simulation settings
#define INDEPENDENT_SANDBOX_ENABLE false    // Enable independent physics simulation mode

// Virtual panel dimensions for complex animations
#ifndef VPANEL_W
#define VPANEL_W MATRIX_WIDTH               // Virtual panel width
#endif
#ifndef VPANEL_H
#define VPANEL_H MATRIX_HEIGHT              // Virtual panel height
#endif

// --- IR RECEIVER AND GPIO SETTINGS ---

// IR receiver operating mode selection (uncomment one)
#define USE_YAHBOOM_IR_BLOCKING_MODE        // Blocking mode for non-standard NEC protocol remotes
// #define USE_NEC_IR_NON_BLOCKING_MODE     // Non-blocking mode for standard NEC protocol

// GPIO pin assignments for Matrix Portal S3 board
#define BUTTON_UP_PIN 6                     // Physical UP button pin
#define BUTTON_DOWN_PIN 7                   // Physical DOWN button pin
#define STATUS_LED_PIN 3                    // Status indicator LED pin
#define BUZZER_PIN 10                       // Audio feedback buzzer pin
#define IR_RECEIVER_PIN 11                  // Infrared signal receiver pin

// Audio feedback settings
#define DEFAULT_BUZZER_ENABLED true         // Enable audio feedback by default

/**
 * @brief IR remote control code mapping structure
 * 
 * Maps raw IR command codes to human-readable button names for easier
 * identification and debugging of remote control inputs.
 */
struct IRCodeMapping {
    uint8_t code;                           // Raw IR command code
    const char* name;                       // Human-readable button name
};

// IR remote control button code definitions
const IRCodeMapping IR_CODE_MAP[] = {
    {0x00, "POWER"},                        // Power/standby button
    {0x01, "UP"},                           // Up navigation
    {0x02, "LIGHT"},                        // Light/brightness control
    {0x04, "LEFT"},                         // Left navigation
    {0x05, "SOUND"},                        // Audio toggle
    {0x06, "RIGHT"},                        // Right navigation
    {0x08, "UNDO"},                         // Undo operation
    {0x09, "DOWN"},                         // Down navigation
    {0x0a, "REDO"},                         // Redo operation
    {0x0c, "PLUS"},                         // Increment/volume up
    {0x0d, "0"},                            // Number 0
    {0x0e, "MINUS"},                        // Decrement/volume down
    {0x10, "1"},                            // Number 1
    {0x11, "2"},                            // Number 2
    {0x12, "3"},                            // Number 3
    {0x14, "4"},                            // Number 4
    {0x15, "5"},                            // Number 5
    {0x16, "6"},                            // Number 6
    {0x18, "7"},                            // Number 7
    {0x19, "8"},                            // Number 8
    {0x1a, "9"}                             // Number 9
};

// Total number of defined IR codes
const size_t IR_CODE_MAP_SIZE = sizeof(IR_CODE_MAP) / sizeof(IRCodeMapping);

// IR command code definitions for programmatic access
#define IR_POWER    0x00                    // Power button code
#define IR_UP       0x01                    // Up button code
#define IR_LIGHT    0x02                    // Light button code
#define IR_LEFT     0x04                    // Left button code
#define IR_SOUND    0x05                    // Sound button code
#define IR_RIGHT    0x06                    // Right button code
#define IR_UNDO     0x08                    // Undo button code
#define IR_DOWN     0x09                    // Down button code
#define IR_REDO     0x0a                    // Redo button code
#define IR_PLUS     0x0c                    // Plus button code
#define IR_0        0x0d                    // Number 0 button code
#define IR_MINUS    0x0e                    // Minus button code
#define IR_1        0x10                    // Number 1 button code
#define IR_2        0x11                    // Number 2 button code
#define IR_3        0x12                    // Number 3 button code
#define IR_4        0x14                    // Number 4 button code
#define IR_5        0x15                    // Number 5 button code
#define IR_6        0x16                    // Number 6 button code
#define IR_7        0x18                    // Number 7 button code
#define IR_8        0x19                    // Number 8 button code
#define IR_9        0x1a                    // Number 9 button code

// --- MODE AND APPLICATION SETTINGS ---

// User interface behavior settings
#define SHOW_MODE_PREVIEW true              // Display preview screen when switching modes
#define MODE_PREVIEW_TIME (2 * 1000UL)      // Mode preview duration in milliseconds

// GIF playback behavior settings
#define GIF_LOAD_AUTO_NEXT_INDEX false      // Automatically advance to next GIF
#define GIF_ENABLE_SINGLE_LOOP true         // Enable single loop playback mode

/**
 * @brief Application display mode enumeration
 * 
 * Defines all available display modes that the application can switch between.
 * Each mode provides different functionality and visual content.
 */
enum DisplayMode : int {
    MODE_CLOCK = 1,                         // Digital clock display
    MODE_MQTT = 2,                          // MQTT message display
    MODE_COUNTDOWN = 3,                     // Countdown timer with sand simulation
    MODE_PATTERN = 4,                       // Pattern and animation display
    MODE_IMAGE = 5,                         // PNG image slideshow
    MODE_GIF = 6,                           // GIF animation playback
    MODE_FONT = 7,                          // Font preview and testing
    MODE_SYSINFO = 8,                       // System information display
    MODE_IR_SCAN = 9,                       // IR remote control scanner
    MODE_ENUM_COUNT                         // Total number of modes (internal use)
};

// Maximum valid mode number for range checking
#define MODE_MAX ((int)MODE_ENUM_COUNT - 1)

/**
 * @brief MQTT stage keyword mapping structure
 * 
 * Maps MQTT message stage keywords to single character indicators
 * for visual status representation on the display.
 */
struct StageMarkMapping {
    char mark;                              // Single character status indicator
    const char* keyword;                    // Lowercase stage keyword to match
};

// Stage indicator mappings for MQTT message processing
const StageMarkMapping STAGE_MARK_MAP[] = {
    // Good/success status indicators
    {'G', "good"},
    {'G', "ok"},
    {'G', "normal"},
    {'G', "pass"},
    
    // Error/failure status indicators
    {'E', "danger"},
    {'E', "error"},
    {'E', "fail"},
    {'E', "ng"},
    
    // Warning status indicators
    {'W', "warning"},
    {'W', "caution"},

    // Information/message status indicators
    {'M', "msg"},
    {'M', "message"},
    {'M', "info"},
    {'M', "information"},
    {'M', "notice"},
    {'M', "alert"}
};

// Total number of stage mark mappings
const size_t STAGE_MARK_MAP_SIZE = sizeof(STAGE_MARK_MAP) / sizeof(StageMarkMapping);

// --- FONT MODE SETTINGS ---

// Font display behavior configuration
#define FONT_PRIMARY FONT_DEFAULT           // Default primary font selection
#define FONT_NAME_SCROLL_ENABLED true       // Enable scrolling for long font names
#define FONT_SAMPLE_TEXT_SCROLL_ENABLED true // Enable scrolling for long sample text
#define SCROLL_CYCLE_END_WAIT_MS (2000UL)   // Wait time between scroll cycles

// --- RUNTIME CONFIGURATION SETTINGS ---

// Runtime configuration variables (defined in main.cpp)
extern char g_deviceId[64];                 // Unique device identifier
extern char g_deviceNo[16];                 // Device number for grouping
extern char g_wifiSsid[64];                 // WiFi network SSID
extern char g_wifiPass[64];                 // WiFi network password
extern char g_mqttServer[64];               // MQTT broker server address
extern int  g_mqttPort;                     // MQTT broker port number
extern char g_mqttTopic[128];               // MQTT subscription topic

// Display and audio settings with defaults
#define DEFAULT_BRIGHTNESS 128              // Default LED panel brightness (0-255)
extern int g_panelBrightness;               // Runtime brightness setting

#define DEFAULT_BUZZER_VOLUME 50            // Default buzzer volume (1-100)
extern int g_buzzerVolume;                  // Runtime buzzer volume setting

// Timeout settings with defaults
#define DEFAULT_IDLE_TIMEOUT_MQTT (60 * 1000UL)     // MQTT mode idle timeout before switching to standby
#define DEFAULT_GLOBAL_IDLE_TIMEOUT_MS (5 * 60 * 1000UL) // Global idle timeout to return to clock mode
extern unsigned long g_idleTimeoutMqtt;
extern unsigned long g_globalIdleTimeoutMs;

// Default display mode settings
#define DEFAULT_INITIAL_DISPLAY_MODE "1.0"  // Default display mode on startup (e.g., "1.0" for Clock)
extern char g_defaultInitialDisplayMode[16]; // Runtime setting for initial display mode

#define DEFAULT_PENDING_MODE "1.0"          // Default mode to switch to on idle timeout
extern char g_defaultPendingMode[16];       // Runtime setting for pending display mode

// Configuration file management function declarations
void loadConfiguration();                   // Load settings from persistent storage
void saveConfiguration();                   // Save current settings to persistent storage

#endif
