/**
 * @file main.cpp
 * @brief Main application entry point for ESP32 Matrix Portal S3
 *
 * Multi-mode LED matrix display system supporting:
 * - mode Clock : Digital clock with real-time updates
 * - mode MQTT : MQTT message display with status indicators
 * - mode Countdown : Countdown timer with physics simulation
 * - mode Pattern : Pattern animations and visual effects
 * - mode Image : PNG image slideshow display
 * - mode GIF : GIF animation playback
 * - mode Font : Font preview and testing
 * - mode SysInfo : System information monitoring
 * - mode IRScan : IR remote control scanner
 * 
 * Hardware: ESP32-S3 Adafruit Matrix Portal with 64x64 P2 LED panel
 * Control: IR remote (NEC protocol), GPIO buttons, MQTT messages
 * 
 * @author Denny Kim <woosul@gmail.com>
 * @date 2025-06-24
 * @license MIT License
 */

// System configuration and utility headers
#include "config.h"
#include "utils.h"
#include "ir_manager.h"

// Display mode class headers
#include "mode_clock.h"
#include "mode_mqtt.h"
#include "mode_countdown.h"
#include "mode_pattern.h"     // Pattern animation display mode
#include "mode_gif.h"         // GIF mode - included before mode_image to avoid macro conflicts
#include "mode_image.h"       // PNG image display mode
#include "mode_font.h"
#include "mode_sysinfo.h"
#include "mode_ir_scan.h"

// Network (WIFI, MQTT and NTP), file system (LittleFS) and system libraries
#include <WiFi.h>
#include <time.h>
#include <FS.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// LED matrix display driver
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h> 

// --- GLOBAL SYSTEM VARIABLES ---

// Core system state
DisplayMode currentMode;                     // Currently active display mode
Utils utils;                                 // Hardware utility and display management
IRManager irManager;                         // Infrared remote control handler

// Hardware interface objects
MatrixPanel_I2S_DMA *dma_display = nullptr; // LED matrix display controller
WiFiClient wifiClient;                       // WiFi network client
PubSubClient mqttClient(wifiClient);         // MQTT communication client

// Communication and activity tracking
char mqtt_message[256] = "";                 // MQTT message processing buffer
unsigned long lastMqttActivity = 0;         // Timestamp of last MQTT activity
unsigned long lastUserActivityTime = 0;     // Timestamp of last user interaction

// MQTT connection and message handling
unsigned long g_lastMqttConnectTime = 0;     // Timestamp of last successful MQTT connection
const unsigned long RETAINED_MESSAGE_GRACE_PERIOD_MS = 5000; // Grace period for retained messages
bool g_systemInitializing = true;           // System initialization state flag
unsigned long g_setupCompleteTime = 0;      // Timestamp when setup() completes

// Runtime configuration variables with default values
char g_deviceId[64] = "D101";                // Unique device identifier
char g_deviceNo[16] = "101";                 // Device number for grouping
char g_wifiSsid[64] = "Ring2";               // WiFi network SSID
char g_wifiPass[64] = "13695271";            // WiFi network password
char g_mqttServer[64] = "mqtt.unoware.com";  // MQTT broker server address
int  g_mqttPort = 2883;                      // MQTT broker port number
char g_mqttTopic[128] = "rfid/d101";         // MQTT subscription topic
int g_panelBrightness = DEFAULT_BRIGHTNESS;  // LED panel brightness level
int g_buzzerVolume = DEFAULT_BUZZER_VOLUME;  // Audio feedback volume
char g_defaultInitialDisplayMode[16] = DEFAULT_INITIAL_DISPLAY_MODE;
char g_defaultPendingMode[16] = DEFAULT_PENDING_MODE;
unsigned long g_idleTimeoutMqtt = DEFAULT_IDLE_TIMEOUT_MQTT;
unsigned long g_globalIdleTimeoutMs = DEFAULT_GLOBAL_IDLE_TIMEOUT_MS;

// Configuration file path
const char* CONFIG_FILE_PATH = "/config.json";

// Display mode instances
ModeClock modeClock;                         // Digital clock display mode
ModeMqtt modeMqtt;                           // MQTT message display mode
ModeCountdown modeCountdown;                 // Countdown timer mode
ModePattern modePattern;                     // Pattern animation mode
ModeImage modeImage;                         // PNG image display mode
ModeGIF modeGif;                             // GIF animation mode
ModeFont modeFont;                           // Font preview mode
ModeSysinfo modeSysinfo;                     // System information mode
ModeIRScan modeIRScan;                       // IR remote scanner mode

// Mode switching and preview control
bool showingModePreview = false;             // Mode preview display state
unsigned long modePreviewStartTime = 0;      // Mode preview start timestamp
DisplayMode pendingMode = MODE_CLOCK;        // Pending mode change target
bool modeChangeByIR = false;                 // IR-triggered mode change flag


// --- FUNCTION DECLARATIONS ---

void handleModeChangeRequest(const char* modeString);
void setupMatrixDisplay();
void connectWiFi();
void ensureNetworkAndTime();
bool connectMQTT();
void processJsonMessage(const char* jsonString);
void handleButtonActions();
void mqttCallback(char* topic, byte* payload, unsigned int length); // NOLINT(bugprone-easily-swappable-parameters)
void maintainMqttConnections();
void setupTime();
void handleIRCommand(uint32_t command);
void switchMode(DisplayMode newMode, bool activateMode = true);
void updateCurrentMode();
void saveConfiguration();

// Color interpolation helper functions for progress bar display
/**
 * @brief Interpolates between two RGB565 colors based on ratio.
 * 
 * Performs linear interpolation between two 16-bit RGB565 colors.
 * Used for smooth color transitions in progress bar animation.
 * 
 * @param color1 Starting RGB565 color
 * @param color2 Ending RGB565 color  
 * @param ratio Interpolation ratio (0.0 to 1.0)
 * @return Interpolated RGB565 color value
 */
static uint16_t interpolateColor(uint16_t color1, uint16_t color2, float ratio) {
    // Clamp ratio to valid range
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;

    // Extract RGB components from RGB565 colors
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5)  & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5)  & 0x3F;
    uint8_t b2 = color2 & 0x1F;

    // Interpolate each component
    uint8_t r = r1 + (uint8_t)((float)(r2 - r1) * ratio);
    uint8_t g = g1 + (uint8_t)((float)(g2 - g1) * ratio);
    uint8_t b = b1 + (uint8_t)((float)(b2 - b1) * ratio);

    // Combine back to RGB565
    return (r << 11) | (g << 5) | b;
}

/**
 * @brief Draws an animated progress bar during system initialization.
 *
 * This function displays a visual progress indicator with elapsed time during
 * the system setup phase. The progress bar color transitions from white to cyan
 * to blue as setup progresses. Also displays "SYSTEM READY" text and real-time
 * elapsed time counter.
 * 
 * @param progress Current progress percentage (0-100)
 * @param startTime System startup timestamp in milliseconds
 * @param display Pointer to the matrix display driver
 * @param util_obj Pointer to utilities object for color conversion and text positioning
 */
static void drawSetupProgressBar(uint8_t progress, unsigned long startTime, MatrixPanel_I2S_DMA* display, Utils* util_obj) {
    if (!display || !util_obj) return;

    // Clear display before drawing progress update
    display->fillScreen(0);

    // Define progress bar colors
    uint16_t colorWhite = util_obj->hexToRgb565(0xFFFFFF);  // Static white
    uint16_t colorCyan  = util_obj->hexToRgb565(0x00FFFF);  // Cyan for time display
    uint16_t colorBlue  = util_obj->hexToRgb565(0x0000FF);  // Blue for progress bar

    // Progress bar dimensions and positioning
    int barBaseX = 2;
    int barY = 60;
    int barWidthMax = MATRIX_WIDTH - (2 * barBaseX);
    int barHeight = 2;

    // Calculate current progress bar width
    int currentBarWidth = (barWidthMax * progress) / 100;
    if (currentBarWidth < 0) currentBarWidth = 0;
    if (currentBarWidth > barWidthMax) currentBarWidth = barWidthMax;

    // Determine progress bar color based on completion percentage
    uint16_t barColor;
    if (progress <= 50) {
        barColor = interpolateColor(colorWhite, colorCyan, progress / 50.0f);
    } else {
        barColor = interpolateColor(colorCyan, colorBlue, (progress - 50) / 50.0f);
    }

    // Display "SYSTEM READY" text
    display->setFont();
    display->setTextSize(1);
    display->setTextColor(colorWhite);
    
    const char* systemText = "SYSTEM";
    int system_x = util_obj->calculateTextCenterX(systemText, MATRIX_WIDTH);
    display->setCursor(system_x, 18);
    display->print(systemText);

    const char* readyText = "READY";
    int ready_x = util_obj->calculateTextCenterX(readyText, MATRIX_WIDTH);
    display->setCursor(ready_x, 30);
    display->print(readyText);

    // Calculate and display elapsed time
    float elapsedTimeSec = (millis() - startTime) / 1000.0f;
    char timeText[10];
    sprintf(timeText, "%.1f", elapsedTimeSec);

    // Configure time display font and position
    display->setFont(FontManager::getFont(FontType::FONT_ORG_R_6));
    display->setTextSize(1);
    int16_t x1, y1; 
    uint16_t w, h;
    display->getTextBounds(timeText, 0, 0, &x1, &y1, &w, &h);
    
    // Right-align time text with margin
    int textX_logical = MATRIX_WIDTH - w - 2;
    int textY_top = 53;

    // Draw progress bar if there's any progress
    if (currentBarWidth > 0) {
        display->fillRect(setPhysicalX(barBaseX), barY, currentBarWidth, barHeight, barColor);
    }
    
    // Draw elapsed time display
    display->setTextColor(colorCyan);
    utils.setCursorTopBased(textX_logical, textY_top, true);
    display->print(timeText);

    display->flipDMABuffer();
}

/**
 * @brief Loads system configuration from JSON file in LittleFS.
 * 
 * Reads configuration parameters from /data/config.json including WiFi credentials,
 * MQTT settings, device ID, and other system parameters. Falls back to default
 * values if configuration file is missing or invalid.
 */
void loadConfiguration() {
    // Serial.println("Attempting to load configuration from LittleFS..."); // DEBUG
    if (!LittleFS.begin(true)) { // true to format LittleFS if mount fails
        Serial.println("LittleFS Mount Failed. Using default configuration.");
        Serial.println("--- DEBUG: loadConfiguration() exiting due to LittleFS mount failure ---");
        // Stick with hardcoded defaults if LittleFS fails
        return;
    }
    Serial.println("LittleFS mounted successfully.");

    if (LittleFS.exists(CONFIG_FILE_PATH)) {
        // Serial.printf("--- DEBUG: Config file '%s' exists on LittleFS.---\n", CONFIG_FILE_PATH);
        File configFile = LittleFS.open(CONFIG_FILE_PATH, "r");
        if (!configFile) {
            Serial.println("Failed to open config file for reading. Using default configuration.");
            // Serial.println("--- DEBUG: loadConfiguration() exiting due to config file open failure ---");
            return;
        }

        StaticJsonDocument<512> docConfig; // Increased size to be safe
        DeserializationError error = deserializeJson(docConfig, configFile);
        configFile.close();

        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            Serial.println("Using default configuration and attempting to save defaults.");
            Serial.println("--- DEBUG: loadConfiguration() will call saveConfiguration() due to JSON parse error ---");
            saveConfiguration(); // Create a default config file if parsing fails
            return;
        }

        strlcpy(g_deviceId, docConfig["deviceId"] | g_deviceId, sizeof(g_deviceId));
        strlcpy(g_deviceNo, docConfig["deviceNo"] | g_deviceNo, sizeof(g_deviceNo));
        strlcpy(g_wifiSsid, docConfig["wifiSsid"] | g_wifiSsid, sizeof(g_wifiSsid));
        strlcpy(g_wifiPass, docConfig["wifiPass"] | g_wifiPass, sizeof(g_wifiPass));
        strlcpy(g_mqttServer, docConfig["mqttServer"] | g_mqttServer, sizeof(g_mqttServer));
        g_mqttPort = docConfig["mqttPort"] | g_mqttPort;
        strlcpy(g_mqttTopic, docConfig["mqttTopic"] | g_mqttTopic, sizeof(g_mqttTopic));
        g_panelBrightness = docConfig["panelBrightness"] | g_panelBrightness;
        // Clamp panel brightness to 0-255 range
        if (g_panelBrightness < 0) g_panelBrightness = 0;
        if (g_panelBrightness > 255) g_panelBrightness = 255;
        g_buzzerVolume = docConfig["buzzerVolume"] | g_buzzerVolume;
        // Clamp buzzer volume to 1-100 range
        if (g_buzzerVolume < 1) g_buzzerVolume = 1;
        if (g_buzzerVolume > 100) g_buzzerVolume = 100;        
        g_idleTimeoutMqtt = docConfig["idleTimeoutMqtt"] | g_idleTimeoutMqtt;
        g_globalIdleTimeoutMs = docConfig["globalIdleTimeoutMs"] | g_globalIdleTimeoutMs;
        strlcpy(g_defaultInitialDisplayMode, docConfig["defaultInitialDisplayMode"] | g_defaultInitialDisplayMode, sizeof(g_defaultInitialDisplayMode));
        strlcpy(g_defaultPendingMode, docConfig["defaultPendingMode"] | g_defaultPendingMode, sizeof(g_defaultPendingMode));

        Serial.printf("Configuration loaded from LittleFS: (%s)\n", CONFIG_FILE_PATH);
        // Serial.println("--- DEBUG: loadConfiguration() successfully loaded and parsed config.json ---");
    } else {
        Serial.printf("--- DEBUG: Config file '%s' not found. Saving default configuration to LittleFS.---\n", CONFIG_FILE_PATH);
        saveConfiguration(); // Create and save default config if not found
    }
    
    Serial.printf(" - Device ID: %s\n", g_deviceId);
    Serial.printf(" - Device No: %s\n", g_deviceNo);
    Serial.printf(" - WiFi SSID: %s\n", g_wifiSsid);
    // Do not print WiFi password to serial for security: Serial.printf(" - WiFi Pass: %s\n", g_wifiPass);
    Serial.printf(" - MQTT Server: %s\n", g_mqttServer);
    Serial.printf(" - MQTT Port: %d\n", g_mqttPort);
    Serial.printf(" - MQTT Topic: %s\n", g_mqttTopic);
    Serial.printf(" - Panel Brightness: %d\n", g_panelBrightness);
    Serial.printf(" - Buzzer Volume: %d\n", g_buzzerVolume);
    Serial.printf(" - Default Initial Display Mode: %s\n", g_defaultInitialDisplayMode);
    Serial.printf(" - Default Pending Mode: %s\n", g_defaultPendingMode);
    Serial.printf(" - MQTT Idle Timeout: %lu ms\n", g_idleTimeoutMqtt);
    Serial.printf(" - Global Idle Timeout: %lu ms\n", g_globalIdleTimeoutMs);
}

/**
 * @brief Saves current system configuration to JSON file in LittleFS.
 * 
 * Writes all runtime configuration parameters to /data/config.json including
 * device settings, network credentials, MQTT parameters, and user preferences.
 * Creates the file if it doesn't exist or overwrites existing configuration.
 */
void saveConfiguration() {
    File configFile = LittleFS.open(CONFIG_FILE_PATH, "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing.");
        return;
    }
    StaticJsonDocument<1024> doc; // Increased size to match loading
    doc["deviceId"] = g_deviceId;
    doc["deviceNo"] = g_deviceNo;
    doc["wifiSsid"] = g_wifiSsid;
    doc["wifiPass"] = g_wifiPass;
    doc["mqttServer"] = g_mqttServer;
    doc["mqttPort"] = g_mqttPort;
    doc["mqttTopic"] = g_mqttTopic;
    doc["panelBrightness"] = g_panelBrightness;
    doc["buzzerVolume"] = g_buzzerVolume;
    doc["idleTimeoutMqtt"] = g_idleTimeoutMqtt;
    doc["globalIdleTimeoutMs"] = g_globalIdleTimeoutMs;
    doc["defaultInitialDisplayMode"] = g_defaultInitialDisplayMode;
    doc["defaultPendingMode"] = g_defaultPendingMode;
    
    if (serializeJson(doc, configFile) == 0) {
        Serial.println(F("Failed to write to config file"));
    } else {
        Serial.println(F("Configuration saved to LittleFS."));
    }
    configFile.close();
}

/**
 * @brief Handles mode change requests from various sources (MQTT, initial setup, idle timeout).
 * 
 * Parses a mode string (e.g., "1.0", "4.2") to determine the target main mode and
 * optional sub-mode. It then calls the appropriate functions to switch to the
 * new mode and/or set the sub-mode.
 * 
 * @param modeString A C-string representing the target mode and optional sub-mode.
 */
void handleModeChangeRequest(const char* modeString) {
    if (!modeString || *modeString == '\0') {
        Serial.println("handleModeChangeRequest: Received empty or null mode string.");
        return;
    }

    Serial.printf("Handling mode change request: %s\n", modeString);

    char* decimal_point = strchr(const_cast<char*>(modeString), '.');
    int mainMode = atoi(modeString);

    if (mainMode < (int)MODE_CLOCK || mainMode > MODE_MAX) {
        Serial.printf("Invalid main mode number: %d\n", mainMode);
        return;
    }

    DisplayMode targetMode = (DisplayMode)mainMode;
    bool hasSubMode = (decimal_point != nullptr);

    // Switch mode. If a sub-mode is specified, prevent automatic activation of the default content.
    // The sub-mode will be handled explicitly below.
    switchMode(targetMode, !hasSubMode);

    if (hasSubMode) {
        // Decimal format detected (e.g., "6.1")
        int subMode = atoi(decimal_point + 1);

        switch(targetMode) {
            case MODE_PATTERN:
                if (subMode >= 1 && subMode <= MAX_PATTERNS) {
                    modePattern.setPattern(static_cast<PatternType>(subMode - 1));
                } // No default activation, setup is enough
                break;
            case MODE_IMAGE:
                if (subMode >= 1 && subMode <= (int)ImageContentType::IMAGE_CONTENT_COUNT) {
                    modeImage.setImage(static_cast<ImageContentType>(subMode - 1));
                } else {
                    modeImage.activate(); // Invalid sub-mode, activate default
                }
                break;
            case MODE_GIF:
                if (subMode >= 1 && subMode <= (int)GifContentType::GIF_CONTENT_COUNT) {
                    modeGif.setGifContent(static_cast<GifContentType>(subMode - 1));
                } else {
                    modeGif.activate(); // Invalid sub-mode, activate default
                }
                break;
            case MODE_FONT:
                if (subMode >= 1 && subMode <= MAX_FONT_MODES) {
                    modeFont.setFontMode(static_cast<FontModeType>(subMode - 1));
                } else {
                    modeFont.activate(); // Invalid sub-mode, activate default
                }
                break;
            case MODE_SYSINFO:
                if (subMode >= 1 && subMode <= ModeSysinfo::INFO_MODE_COUNT) {
                    modeSysinfo.setInfoMode(static_cast<ModeSysinfo::InfoModeType>(subMode - 1));
                } // No default activation needed
                break;
            default:
                Serial.printf("Sub-mode specified for mode %d, but it doesn't support sub-modes.\n", mainMode);
                break;
        }
    }
}


// --- Main Function Implementations ---

/**
 * @brief Initializes and configures the LED matrix display hardware.
 * 
 * Sets up the HUB75 LED matrix panel using ESP32-HUB75-MatrixPanel-DMA library.
 * Configures display parameters like dimensions, chain length, and DMA settings.
 * Enables the display and sets initial brightness level.
 */
void setupMatrixDisplay() {
    Serial.printf("Initializing %dx%d display...\n", MATRIX_WIDTH, MATRIX_HEIGHT);
    HUB75_I2S_CFG mxconfig(MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_CHAIN);
    // Use pin definitions from config.h
    mxconfig.gpio.r1 = R1_PIN; mxconfig.gpio.g1 = G1_PIN; mxconfig.gpio.b1 = B1_PIN;
    mxconfig.gpio.r2 = R2_PIN; mxconfig.gpio.g2 = G2_PIN; mxconfig.gpio.b2 = B2_PIN;
    mxconfig.gpio.a = A_PIN;  mxconfig.gpio.b = B_PIN;  mxconfig.gpio.c = C_PIN;
    mxconfig.gpio.d = D_PIN;  mxconfig.gpio.e = E_PIN;
    mxconfig.gpio.clk = CLK_PIN; mxconfig.gpio.lat = LAT_PIN; mxconfig.gpio.oe = OE_PIN;

    mxconfig.double_buff = !FORCE_SINGLE_BUFFER; // Use config.h setting
    mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;   // Or other configured speed

    dma_display = new MatrixPanel_I2S_DMA(mxconfig);

    if (dma_display && dma_display->begin()) {
        Serial.printf("Display: %dx%d initialized successfully\n", MATRIX_WIDTH, MATRIX_HEIGHT);
        dma_display->setBrightness8(g_panelBrightness); // Set default brightness (config.h)
        dma_display->clearScreen();
        dma_display->flipDMABuffer(); // Display initial buffer
    } else {
        Serial.println("Display: Initialization FAILED!");
        // Handle display initialization failure (e.g., infinite loop or retry)
        while(1) { delay(1000); }
    }
}

/**
 * @brief Main system initialization and setup function.
 * 
 * Initializes all hardware components, loads configuration, establishes network
 * connections, sets up time synchronization, and prepares the system for operation.
 * Displays animated progress bar during initialization. Sets up the default clock
 * mode after completion.
 */
void setup() {
    // Initialize serial communication for debugging
    Serial.begin(115200);
    delay(500); // Allow serial to initialize
    
    // Initialize LittleFS and load configuration
    loadConfiguration(); // Call this early, before WiFi/MQTT setup that might use these values
    utils.setBuzzerVolume(g_buzzerVolume); // Set buzzer volume after loading config
    
    // Log the time taken to set the module ------------------------------------------
    unsigned long setupStartTime = millis();
    unsigned long lastLogTime = setupStartTime;
    unsigned long currentTime;
    
    uint8_t totalProgress = 0;

    // Wait for serial port to stabilize, but log this time
    delay(100); // Reduced delay, 1000ms is usually not needed if Serial.begin is early.


    // Log the time taken to set the module ------------------------------------------
    currentTime = millis();
    lastLogTime = currentTime;

    // Initialize display hardware, MatrixPanel_I2S_DMA, and Utils
    Serial.println("########## Matrix Portal S3 Starting... ##########");
    // 1. setupMatrixDisplay() is done here. Progress: 10%
    setupMatrixDisplay();

    // 2. utils.setup() - This will show "SYSTEM READY". Progress: 5%
    utils.setup(dma_display);        // Initialize Utils by passing the display pointer and run displayColortest if defined. 
                                                //'dma_display' is a pointer to MatrixPanel_I2S_DMA object.
    Serial.println("Display, GPIO and Utils initialized");

    // After "SYSTEM READY" is displayed by utils.setup(),
    // remove the screen initialization code to keep "SYSTEM READY" displayed.

    // Initial progress bar display (0%) after "SYSTEM READY"
    drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);

    // Account for setupMatrixDisplay
    totalProgress += 10;
    drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);
    // Account for utils.setup
    totalProgress += 5;
    drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);

    // Log the time taken to set the module ------------------------------------------
    currentTime = millis();
    Serial.printf("--- Setup - Display and Utils Initialized: %lu ms, Total: %lu ms\n\n", currentTime - lastLogTime, currentTime - setupStartTime);
    lastLogTime = currentTime;

    // Initialize IRManager (pass Utils object address)
    irManager.setup(&utils);          // 'utils' is a just object and make pointer variable with '&' for Utils object.
    // 3. irManager.setup(). Progress: 5%
    totalProgress += 5;
    drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);
    Serial.println("IRManager initialized");

    // In main.cpp setup() function, after irManager.setup(&utils);
    #if defined(USE_YAHBOOM_IR_BLOCKING_MODE)
        irManager.setReceiveMode(IRManager::IRReceiveMode::YAHBOOM_BLOCKING);
        Serial.println("Main: IRManager set to YAHBOOM_BLOCKING mode from config.");
    #elif defined(USE_NEC_IR_NON_BLOCKING_MODE)
        irManager.setReceiveMode(IRManager::IRReceiveMode::NEC_NON_BLOCKING);
        Serial.println("Main: IRManager set to NEC_NON_BLOCKING mode from config.");
    #else
        // Default to NEC_NON_BLOCKING if no specific mode is defined in config.h
        irManager.setReceiveMode(IRManager::IRReceiveMode::NEC_NON_BLOCKING);
        Serial.println("Main: IRManager set to default NEC_NON_BLOCKING mode.");
    #endif

    // Log the time taken to set the module ------------------------------------------
    currentTime = millis();
    Serial.printf("--- Setup - IRManager Initialized & Mode Set: %lu ms, Total: %lu ms\n\n", currentTime - lastLogTime, currentTime - setupStartTime);
    lastLogTime = currentTime;

    // Connect to WiFi and ensure network and time synchronization
    // 4. ensureNetworkAndTime broken down for progress:
    // 4.a. WiFi: 20%
    connectWiFi();
    if (WiFi.status() == WL_CONNECTED) {
        totalProgress += 20;
        drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);
    } else {
        Serial.println("--- Setup: WiFi connection failed. Progress for MQTT/NTP might be affected.");
    }

    // 4.b. MQTT: 20%
    if (WiFi.status() == WL_CONNECTED) {
        if (connectMQTT()) {
            totalProgress += 20;
            drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);
        } else {
            Serial.println("--- Setup: MQTT connection failed.");
        }
    }

    // 4.c. NTP: 20%
    if (WiFi.status() == WL_CONNECTED) {
        setupTime();
        totalProgress += 20; // Credit NTP attempt
        if(totalProgress > 80) totalProgress = 80; // Cap if previous steps failed
        drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);
    }

    // Log the time taken to set the module ------------------------------------------
    currentTime = millis();
    Serial.printf("Setup - Network & Time Synced: %lu ms, Total: %lu ms\n\n", currentTime - lastLogTime, currentTime - setupStartTime);
    lastLogTime = currentTime;

    // Note: Individual mode setup() functions are now called when first entering each mode
    // via switchMode(), rather than during main setup() to optimize boot time.

    // 5. Mode setups. Progress: 10%
    totalProgress += 10;
    if(totalProgress > 90) totalProgress = 90;
    drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);

    // Log the time taken to set the module ------------------------------------------
    currentTime = millis();
    Serial.printf("--- Setup - Core Initializations Done (Modes will setup on entry): %lu ms, Total: %lu ms\n\n", currentTime - lastLogTime, currentTime - setupStartTime);
    lastLogTime = currentTime;

    // 6. utils.playScaleTone(). Progress: 5%
    // Play scale tone first, then switch to initial mode
    utils.playScaleTone();
    totalProgress += 5;
    if(totalProgress > 95) totalProgress = 95;
    drawSetupProgressBar(totalProgress, setupStartTime, dma_display, &utils);
    Serial.println("Setup and initialization complete!");

    // Log the time taken to set the module ------------------------------------------
    currentTime = millis();
    Serial.printf("--- Setup - Scale Tone Played: %lu ms, Total: %lu ms\n\n", currentTime - lastLogTime, currentTime - setupStartTime);
    lastLogTime = currentTime;

    // Display progress at 100% and wait briefly
    drawSetupProgressBar(100, setupStartTime, dma_display, &utils);
    delay(1000); // Display 100% for a second

    // Switch to initial mode - this call is moved to the end of setup()
    // switchMode(MODE_CLOCK);

    currentTime = millis(); // Measure time after switchMode completion
    // Serial.printf("--- Setup - Initial Mode Switched: %lu ms, Total: %lu ms\n\n", currentTime - lastLogTime, currentTime - setupStartTime);
    Serial.printf("--- Total Setup Time: %lu ms\n\n", currentTime - setupStartTime);

    g_setupCompleteTime = millis(); // Record the time setup completes
    g_systemInitializing = true;    // Ensure it's true as setup finishes
    lastUserActivityTime = millis(); // Initialize last activity time
    
    Serial.printf("Setting initial display mode to: %s\n", g_defaultInitialDisplayMode);
    handleModeChangeRequest(g_defaultInitialDisplayMode);
    
}

/**
 * @doc Main system loop executing mode operations and managing connections.
 * 
 * Continuously monitors system state, processes IR commands, handles button inputs,
 * executes current display mode operations, maintains network connections, and
 * implements idle timeout functionality to return to clock mode.
 */
void loop() {
    // Check if system initialization grace period has passed
    if (g_systemInitializing && (millis() - g_setupCompleteTime > RETAINED_MESSAGE_GRACE_PERIOD_MS)) {
        if (g_systemInitializing) { // Check again to prevent multiple prints if loop is fast
            Serial.println("System initialization grace period ended."); // System initialization grace period ended.
            g_systemInitializing = false;
        }
    }

    // Check if the IRManager has a new command
    if (irManager.getCurrentReceiveMode() == IRManager::IRReceiveMode::NEC_NON_BLOCKING) {
        // Update IR reception state machine (non-blocking) for NEC mode
        irManager.update();
    } else if (irManager.getCurrentReceiveMode() == IRManager::IRReceiveMode::YAHBOOM_BLOCKING) {
        // For RAW_BLOCKING mode, explicitly call readRawBlockingCode.
        // This call is blocking. Consider calling it less frequently or based on specific triggers.
        // Example: timeout of 100ms
        irManager.readRawCodeYahboom(100); 
    }

    // Check and handle network and time synchronization status
    ensureNetworkAndTime(); 

    // Let Utils handle its internal button debouncing and state regarding UP/DOWN buttons to change modes
    utils.handleButtons();
    // Call the new function to process button actions
    handleButtonActions(); 

    // Execute the main funtion for the current mode
    // This will call the run() function of the currently active mode
    // and pass handleIRCommand as the IR command handler.
    updateCurrentMode();

    // Maintain MQTT connection
    maintainMqttConnections(); 

    // Check for global idle timeout to switch to default mode
    int pendingMainMode = atoi(g_defaultPendingMode);
    if ((int)currentMode != pendingMainMode && (millis() - lastUserActivityTime > g_globalIdleTimeoutMs)) {
        Serial.printf("Global idle timeout reached. Switching to pending mode: %s\n", g_defaultPendingMode);
        // if (utils.isSoundFeedbackEnabled()) {
        //     utils.playSingleTone(); 
        // }
        handleModeChangeRequest(g_defaultPendingMode);
        // lastUserActivityTime will be updated by the next actual user interaction
    }
    // Short delay to allocate CPU time to other tasks (e.g., WiFi, background tasks)
    delay(5); // Short delay to yield CPU for other tasks (WiFi, background processes)
}
// Helper functions for main loop operations
/**
 * @brief Processes button actions received from the Utils module.
 *
 * Checks for button press events (NEXT_MODE/PREV_MODE) from hardware GPIO buttons,
 * calculates target display mode, and triggers mode switching. Updates user activity
 * timestamp for idle timeout management.
 */
void handleButtonActions() {
    // Check for button actions from Utils
    ButtonAction actionFromUtils = utils.getButtonAction(); // Changed: Utils::ButtonAction -> ButtonAction
    if (actionFromUtils != ButtonAction::NONE) { // Changed: Utils::ButtonAction::NONE -> ButtonAction::NONE
        DisplayMode targetMode = currentMode;
        lastUserActivityTime = millis(); // User pressed a button

        if (actionFromUtils == ButtonAction::NEXT_MODE) { // Changed
            targetMode = (DisplayMode)((((int)currentMode - 1 + 1) % (int)MODE_MAX) + 1);
            Serial.printf("Main: Processing NEXT_MODE to %d from Utils button press.\n", (int)targetMode);
        } else if (actionFromUtils == ButtonAction::PREV_MODE) { // Changed
            targetMode = (DisplayMode)((((int)currentMode - 1 - 1 + (int)MODE_MAX) % (int)MODE_MAX) + 1);
            Serial.printf("Main: Processing PREV_MODE to %d from Utils button press.\n", (int)targetMode);
        }
        if (targetMode != currentMode) { // Simplified condition: if target is different, switch.
            switchMode(targetMode, true); // Explicitly activate the new mode
        }
    }
}

/**
 * @brief Switches from current display mode to a new display mode.
 * 
 * Handles cleanup of the current mode, initializes the new mode, and updates
 * the global currentMode variable. Some modes require activation calls after
 * setup to load initial content (e.g., images, GIFs, fonts).
 * 
 * @param newMode Target display mode to switch to
 */
void switchMode(DisplayMode newMode, bool activateMode) {
    if (currentMode == newMode && !modeChangeByIR && activateMode) { // Also check activateMode to allow re-activation
        // Always perform changes due to IR.
        // If the same mode is requested (excluding IR), do nothing 
        // If the same mode is requested from IR, restart the current mode
        return;
    }

    // Cleanup previous mode
    switch (currentMode) {
        case MODE_CLOCK:        modeClock.cleanup(); break;
        case MODE_MQTT:         modeMqtt.cleanup(); break;
        case MODE_COUNTDOWN:    modeCountdown.cleanup(); break;
        case MODE_PATTERN:      modePattern.cleanup(); break; // Renamed from MODE_ANIMATION
        case MODE_IMAGE:       modeImage.cleanup(); break;
        case MODE_GIF:          modeGif.cleanup(); break;     // Cleanup for new GIF mode
        case MODE_FONT:         modeFont.cleanup(); break;
        case MODE_SYSINFO:      modeSysinfo.cleanup(); break;
        case MODE_IR_SCAN:      modeIRScan.cleanup(); break;
        default: break;
    }

    currentMode = newMode;

    // Setup new mode - call setup() or activate() depending on the mode entered
    switch (newMode) {
        case MODE_CLOCK:
                                modeClock.setup(&utils, dma_display);
                                break;
        case MODE_MQTT:
                                modeMqtt.setup(&utils, dma_display);
                                // ModeMqtt.setup() should handle resetting its internal flags
                                break;
        case MODE_COUNTDOWN:
                                modeCountdown.setup(&utils, dma_display);
                                // ModeCountdown.setup() should handle resetting its internal flags, timer and sand glass
                                break;
        case MODE_PATTERN:
                                modePattern.setup(&utils, dma_display);
                                // ModePattern.setup() should set the initial pattern
                                break;
        case MODE_IMAGE:
                                modeImage.setup(&utils, dma_display);
                                if (activateMode) {
                                    modeImage.activate();
                                }
                                break;
        case MODE_GIF:
                                modeGif.setup(&utils, dma_display);
                                if (activateMode) {
                                    modeGif.activate();
                                }
                                break;
        case MODE_FONT:
                                modeFont.setup(&utils, dma_display);
                                if (activateMode) {
                                    modeFont.activate();
                                }
                                break;
        case MODE_SYSINFO:
                                modeSysinfo.setup(&utils, dma_display);
                                // ModeSysinfo.setup() should set the initial info screen
                                break;
        case MODE_IR_SCAN:
                                modeIRScan.setup(&utils, dma_display, &irManager);
                                // ModeIRScan.setup() prepares for IR scanning,
                                // its run() method might contain the main blocking loop if USE_RUN_INTERNAL_LOOP is true
                                break;
        default: break;
    }

    Serial.printf("Mode switched to: %d (%s)\n", (int)currentMode, utils.getShortModeName(currentMode));
    modeChangeByIR = false; // Reset IR request flag
}

/**
 * @brief Processes IR remote control commands and triggers appropriate actions.
 * 
 * Handles mode switching (IR_1-9 for direct modes, IR_UP/DOWN for sequential),
 * sub-mode navigation (IR_LEFT/RIGHT for patterns, images, fonts, etc.),
 * and system functions (IR_POWER, IR_SOUND). Updates user activity timestamp
 * and provides audio feedback if enabled.
 * 
 * @param command 32-bit IR command code received from remote control
 */
void handleIRCommand(uint32_t command) {
    Serial.printf("Processing IR Command: 0x%02X\n", command);

    DisplayMode modeBeforeIRHandling = currentMode; // Store currentMode at the beginning of IR handling
    DisplayMode targetMode = currentMode; // Default target mode is the current mode
    modeChangeByIR = false; // Reset flag: true only if an actual mode switch is intended

    switch (command) {
        case IR_1: targetMode = MODE_CLOCK;     modeChangeByIR = true; break;
        case IR_2: targetMode = MODE_MQTT;      modeChangeByIR = true; break;
        case IR_3: targetMode = MODE_COUNTDOWN; modeChangeByIR = true; break;
        case IR_4: targetMode = MODE_PATTERN;   modeChangeByIR = true; break; // Was MODE_ANIMATION
        case IR_5: targetMode = MODE_IMAGE;     modeChangeByIR = true; break; // Image mode
        case IR_6: targetMode = MODE_GIF;       modeChangeByIR = true; break; // GIF mode
        case IR_7: targetMode = MODE_FONT;      modeChangeByIR = true; break; // Shifted
        case IR_8: targetMode = MODE_SYSINFO;   modeChangeByIR = true; break; // Shifted
        // Assuming IR_9 is defined in config.h for MODE_IR_SCAN
        case IR_9: targetMode = MODE_IR_SCAN;   modeChangeByIR = true; break; // Shifted

        case IR_UP:
            targetMode = (DisplayMode)((((int)currentMode - 1 + 1) % MODE_MAX) + 1); // MODE_MAX is already (COUNT-1)
            modeChangeByIR = true;
            break;
        case IR_DOWN:
            targetMode = (DisplayMode)((((int)currentMode - 1 - 1 + MODE_MAX) % MODE_MAX) + 1);
            modeChangeByIR = true;
            break;

        // Change sub-mode or feature within the current mode
        case IR_LEFT: // refer to definition of config.h for actual IR code
            
            if (currentMode == MODE_PATTERN) { // Was MODE_ANIMATION
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone(); // Feedback for pattern change
                modePattern.prevPattern(); // Renamed method
                lastUserActivityTime = millis(); // Consider pattern change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else if (currentMode == MODE_IMAGE) {
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone();
                modeImage.prevImage();
                lastUserActivityTime = millis(); // Consider image change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else if (currentMode == MODE_GIF) {
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone();
                modeGif.prevGif();
                lastUserActivityTime = millis(); // Consider pattern change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else if (currentMode == MODE_FONT) {
                // If in MODE_FONT, switch to next font
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone(); // Feedback for pattern change
                modeFont.prevFont();
                lastUserActivityTime = millis(); // Consider font change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else if (currentMode == MODE_SYSINFO) {
                // If in MODE_SYSINFO, switch to previous info screen
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone(); // Feedback for info change
                modeSysinfo.prevInfo();
                lastUserActivityTime = millis(); // Consider info change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else {
                // Serial.println("IR: LEFT command received - No action in current mode");
                if (utils.isSoundFeedbackEnabled()) utils.playErrorTone(); // Feedback for no action
            }
            break;
        case IR_RIGHT: // refer to definition of config.h for actual IR code
            if (currentMode == MODE_PATTERN) { // Was MODE_ANIMATION
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone(); // Feedback for pattern change
                modePattern.nextPattern(); // Renamed method
                lastUserActivityTime = millis(); // Consider pattern change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else if (currentMode == MODE_IMAGE) {
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone();
                modeImage.nextImage();
                lastUserActivityTime = millis(); // Consider image change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else if (currentMode == MODE_GIF) {
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone();
                modeGif.nextGif();
                // modeChangeByIR remains false, so no full mode switch occurs
            } else if (currentMode == MODE_FONT) {
                // If in MODE_FONT, switch to next font
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone(); // Feedback for pattern change
                modeFont.nextFont();
                lastUserActivityTime = millis(); // Consider font change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else if (currentMode == MODE_SYSINFO) {
                // If in MODE_SYSINFO, switch to next info screen
                if (utils.isSoundFeedbackEnabled()) utils.playSingleTone(); // Feedback for info change
                modeSysinfo.nextInfo();
                lastUserActivityTime = millis(); // Consider info change as user activity
                // modeChangeByIR remains false, so no full mode switch occurs
            } else {
                // Serial.println("IR: RIGHT command received - No action in current mode");
                if (utils.isSoundFeedbackEnabled()) utils.playErrorTone(); // Feedback for no action
            }
            break;
        case IR_POWER:
            Serial.println("IR: POWER command received - (Standby logic can be implemented here)");
            // Example: utils.enterStandby();
            break;
        case IR_SOUND:
            // Use Utils class's isSoundFeedbackEnabled() (assumed to be declared in utils.h)
            if (utils.isSoundFeedbackEnabled()) {
                utils.enableBuzzer(false);
            } else {
                utils.enableBuzzer(true);
            }
            Serial.printf("IR: SOUND command received - Buzzer %s\n", utils.isSoundFeedbackEnabled() ? "ON" : "OFF");
            break;

        default:
            Serial.printf("IR: Unknown command 0x%02X - ignoring\n", command);
            if (utils.isSoundFeedbackEnabled()) utils.playErrorTone();
            break;
    }

    // Perform mode switch only if modeChangeByIR is true
    if (modeChangeByIR) {
        lastUserActivityTime = millis(); // Update activity time for mode switch
        if (SHOW_MODE_PREVIEW && targetMode != modeBeforeIRHandling) { // Show preview only if mode actually changes
            utils.showModePreview(targetMode, utils.getShortModeName(targetMode));
            switchMode(targetMode, true); // Explicitly activate after preview
        } else { // Switch mode without preview (or if it's a restart of the same mode)
            switchMode(targetMode, true); // Explicitly activate
        }
    }
}


/**
 * @brief Ensures network connectivity and time synchronization are maintained.
 *
 * Monitors WiFi connection status and attempts reconnection if disconnected.
 * Also maintains MQTT connection and performs time synchronization when
 * network becomes available. Called periodically from main loop.
 */
void ensureNetworkAndTime() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi(); // Attempt WiFi connection
        if (WiFi.status() == WL_CONNECTED) {
            connectMQTT(); // Attempt MQTT connection if WiFi connection is successful
            setupTime();   // Perform time synchronization after network connection
        }
    } else { // WiFi is connected, but MQTT might not be
        if (!mqttClient.connected()) {
            connectMQTT();
        }
        // Time synchronization can be checked periodically or as needed
        // Example: if (time(nullptr) < 1000000000) setupTime(); // Check if time is very old
    }
}

/**
 * @brief Establishes WiFi connection using configured credentials.
 * 
 * Attempts to connect to WiFi network using SSID and password from configuration.
 * Optionally scans for best signal strength if WIFI_USE_BEST_SIGNAL is enabled.
 * Provides visual feedback via status LED during connection process.
 */
void connectWiFi() {
    unsigned long wifiStartTime = millis();
    unsigned long wifiLastLogTime = wifiStartTime;
    unsigned long wifiCurrentTime;

    WiFi.mode(WIFI_STA); // Set to Station mode

    uint8_t* bestBSSID = nullptr; // Move declaration outside so it's visible below

    if (WIFI_USE_BEST_SIGNAL) {
        Serial.println("Scanning for best WiFi signal...");
        int n = WiFi.scanNetworks();
        
        wifiCurrentTime = millis();
        Serial.printf("--- WiFi - Scan Complete: %lu ms, Total: %lu ms\n\n", wifiCurrentTime - wifiLastLogTime, wifiCurrentTime - wifiStartTime);
        wifiLastLogTime = wifiCurrentTime;

        int bestRSSI = -100;
        int bestNetwork = -1;

        for (int i = 0; i < n; i++) {
            if (WiFi.SSID(i) == g_wifiSsid && WiFi.RSSI(i) > bestRSSI) {
                bestRSSI = WiFi.RSSI(i);
                bestBSSID = WiFi.BSSID(i);
                bestNetwork = i;
            }
        }
        if (bestNetwork >= 0) {
            Serial.printf("Best signal for %s: RSSI %d dBm\n", g_wifiSsid, bestRSSI);
        }
    }

    Serial.printf("WiFi - Starting connection to %s...\n", g_wifiSsid);
    
    wifiLastLogTime = millis(); // Reset log time before begin    
    if (WIFI_USE_BEST_SIGNAL && bestBSSID) {
        Serial.printf("Attempting to connect to BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
                      bestBSSID[0], bestBSSID[1], bestBSSID[2], bestBSSID[3], bestBSSID[4], bestBSSID[5]);
        WiFi.begin(g_wifiSsid, g_wifiPass, 0, bestBSSID); // Connect to specific BSSID
    } else {
        WiFi.begin(g_wifiSsid, g_wifiPass); // Standard connection using loaded credentials
    }

    Serial.printf("Connecting to %s", g_wifiSsid);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) { // Max 30 attempts (15 seconds)
        Serial.print(".");
        delay(500);
    
        wifiCurrentTime = millis();
        // Log every few attempts or every second to avoid too much serial traffic
        // For now, logging each attempt's end time.
        // Serial.printf("--- WiFi - Attempt %d waited: %lu ms, Loop Total: %lu ms\n", attempts + 1, wifiCurrentTime - wifiLastLogTime, wifiCurrentTime - wifiStartTime);
        // wifiLastLogTime = wifiCurrentTime; // This would log duration of delay(500) mostly
        attempts++;
        utils.setStatusLED(attempts % 2); // Blinking LED during connection
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Connected!");
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
        utils.setStatusLED(true); // Turn on LED on successful connection
        // if (utils.isSoundFeedbackEnabled()) utils.playSingleTone(); // Sound can be played by ensureNetworkAndTime
    } else {
        Serial.println(" Failed to connect!");
        utils.setStatusLED(false); // Turn off LED on connection failure
        // if (utils.isSoundFeedbackEnabled()) utils.playErrorTone();
    }
    
    wifiCurrentTime = millis();
    Serial.printf("--- WiFi - Connection Process Finished: %lu ms, Total WiFi Setup: %lu ms\n\n", wifiCurrentTime - wifiStartTime, wifiCurrentTime - wifiStartTime);
}

/**
 * @brief Establishes MQTT connection and subscribes to configured topic.
 * 
 * Connects to MQTT broker using server details from configuration. Generates
 * unique client ID and subscribes to the configured topic for receiving commands.
 * Implements retry logic with exponential backoff on connection failures.
 * 
 * @return true if connection successful, false otherwise
 */
bool connectMQTT() {
    unsigned long mqttStartTime = millis();
    unsigned long mqttLastLogTime = mqttStartTime;
    unsigned long mqttCurrentTime;

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping MQTT connection.");
        return false;
    }

    Serial.println("MQTT - Setting up client...");
    
    mqttCurrentTime = millis();
    // Serial.printf("--- MQTT - Client Setup: %lu ms, Total: %lu ms\n\n", mqttCurrentTime - mqttLastLogTime, mqttCurrentTime - mqttStartTime);
    mqttLastLogTime = mqttCurrentTime;
    
    mqttClient.setServer(g_mqttServer, g_mqttPort); // Use loaded MQTT server info
    mqttClient.setKeepAlive(60);  // KeepAlive 60 seconds
    mqttClient.setSocketTimeout(15); // Socket timeout 15 seconds
    mqttClient.setCallback(mqttCallback); // Set message reception callback function

    int attempts = 0;
    while (!mqttClient.connected() && attempts < 3) { // Max 3 attempts
        Serial.printf("Attempting MQTT connection (attempt %d)... ", attempts + 1);
        String clientId = String(g_deviceId) + String("_") + String(random(0xFFFF), HEX); // Generate unique client ID using loaded g_deviceId

        if (mqttClient.connect(clientId.c_str())) {
            Serial.println("MQTT Connected!");
            utils.setStatusLED(true); // Turn on LED on successful connection (can be shared with WiFi)
            // Subscribe to the loaded MQTT topic
            if (mqttClient.subscribe(g_mqttTopic)) { 
                Serial.printf("Subscribed to topic: %s\n", g_mqttTopic);
            } else {
                Serial.println("MQTT subscription failed!");
            }
            g_lastMqttConnectTime = millis(); // Record MQTT connection time
            lastMqttActivity = millis();
            
            mqttCurrentTime = millis();
            Serial.printf("--- MQTT - Connected & Subscribed: %lu ms, Total MQTT Setup: %lu ms\n\n", mqttCurrentTime - mqttLastLogTime, mqttCurrentTime - mqttStartTime);
            
            return true;
        } else {
            Serial.printf("Failed, rc=%d. Retrying in 2 seconds...\n", mqttClient.state());
            attempts++;
            utils.setStatusLED(false); // LED status can be changed during connection attempt
            delay(2000);
            
            mqttCurrentTime = millis();
            Serial.printf("--- MQTT - Attempt %d Failed & Waited: %lu ms, Loop Total: %lu ms\n\n", attempts, mqttCurrentTime - mqttLastLogTime, mqttCurrentTime - mqttStartTime);
            mqttLastLogTime = mqttCurrentTime;
        }
    }
    Serial.println("MQTT connection failed after several attempts.");
    mqttCurrentTime = millis();
    // Serial.printf("--- MQTT - Connection Process Finished (Failed): %lu ms, Total MQTT Setup: %lu ms\n", mqttCurrentTime - mqttStartTime, mqttCurrentTime - mqttStartTime);
    return false;
}

/**
 * @brief MQTT message reception callback handler.
 * 
 * Called automatically when MQTT messages are received on subscribed topics.
 * Converts payload to null-terminated string and forwards to JSON message
 * processor. Updates activity timestamp and provides audio feedback.
 * 
 * @param topic MQTT topic that received the message
 * @param payload Raw message payload data
 * @param length Length of payload in bytes
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Serial.println("mqttCallback: Entered function."); // Callback start log
    lastUserActivityTime = millis(); // MQTT message received is considered an activity
    
    char jsonString[length + 1]; // Buffer for JSON string
    memcpy(jsonString, payload, length);
    jsonString[length] = '\0';
    // Serial.printf("mqttCallback: Received payload (len %u): %s\n", length, jsonString); // Payload log

    Serial.printf("MQTT TOPIC [%s]: %s\n", topic, jsonString);
    if (jsonString) {
        // Log the received JSON message
        utils.playDoubleTone();
    } else {
        Serial.println("MQTT: Received empty or null payload.");
        return; // Exit if payload is empty
    }

    processJsonMessage(jsonString);
    // Serial.println("mqttCallback: Exiting function."); // Callback end log
}


/**
 * @brief Processes JSON-formatted MQTT messages and executes commands.
 * 
 * Parses incoming JSON messages to extract stage, code, and message fields.
 * Handles mode switching commands, sub-mode selections, and message display
 * requests. Implements grace periods to ignore retained messages during
 * system initialization and connection phases.
 * 
 * @param jsonString Null-terminated JSON string to process
 */
void processJsonMessage(const char* jsonString) {
    // Serial.println("processJsonMessage: Entered function.");

    // 1. Check for system initialization grace period (for messages after full boot)
    if (g_systemInitializing) {
        Serial.printf("MQTT: Message [%s] received during system initialization grace period. Logged, no mode change or display update will occur.\n", jsonString);
        lastMqttActivity = millis(); // Update activity time
        // Do not update mqtt_message buffer with this message.
        Serial.println("processJsonMessage: Exiting due to system initialization grace period.");
        return;
    }

    // 2. Deserialize JSON (moved here to avoid parsing if in system init grace period)
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, jsonString);

    if (error) {
        Serial.printf("processJsonMessage: JSON parsing failed: %s\n", error.c_str());
        return;
    }

    const char* code_str = doc["code"];
    const char* stage_str = doc["stage"];
    const char* message_str = doc["message"];

    // 3. Check for retained message grace period after an MQTT connection (for reconnects)
    // This g_lastMqttConnectTime is updated each time connectMQTT() succeeds.
    bool isInConnectGracePeriod = (millis() - g_lastMqttConnectTime < RETAINED_MESSAGE_GRACE_PERIOD_MS);
    if (isInConnectGracePeriod) {
        Serial.printf("MQTT: Message [%s] received during post-connection grace period. Logged, no mode change or display update will occur.\n", jsonString);
        lastMqttActivity = millis(); // Update activity time
        // Do not update mqtt_message buffer with retained message content
        // Do not switch mode or update display
        // Serial.println("processJsonMessage: Exiting due to post-connection grace period.");
        return;
    }

    // 4. Duplicate message check (if not in any grace period)
    if (strcmp(mqtt_message, jsonString) == 0) {
        Serial.println("processJsonMessage: Duplicate MQTT message (non-grace period). Ignoring.");
        lastMqttActivity = millis(); // Update activity time even for duplicates
        // Serial.println("processJsonMessage: Exiting due to duplicate message.");
        return;
    }

    // 5. Process the new, non-duplicate, non-grace-period message
    if (stage_str) {
        if (strcmp(stage_str, "mode") == 0) {
            if (code_str) { // If 'code' field exists
                handleModeChangeRequest(code_str);
            } else {
                Serial.println("MQTT: Stage 'mode', but 'code' field is missing.");
            }
        } else { // Not a "mode" stage, assume it's for MODE_MQTT display
            Serial.printf("MQTT: Stage '%s', code '%s'. Preparing to display message.\n", stage_str, code_str ? code_str : "N/A");
            modeMqtt.isRequestDirectMessageDisplay = true; // Signal ModeMqtt about the new message

            if (currentMode != MODE_MQTT) { // Switch to MQTT mode if not already in it
                switchMode(MODE_MQTT, false); // Switch without activating default content, as a specific message will be displayed
            }
            // Whether we switched to MODE_MQTT or were already in it,
            // now call displayMqttMessage.
            // ModeMqtt::displayMqttMessage will use isRequestDirectMessageDisplay if needed
            // and then typically clear it.
            modeMqtt.displayMqttMessage(stage_str, code_str, message_str);
        }
    } else {
        Serial.println("MQTT: 'stage' field is missing in JSON.");
    }

    // Update mqtt_message buffer for the new, processed message
    strncpy(mqtt_message, jsonString, sizeof(mqtt_message) - 1);
    mqtt_message[sizeof(mqtt_message) - 1] = '\0'; // Null-terminate
    lastMqttActivity = millis();
    // Serial.println("processJsonMessage: Exiting function after processing new message.");
}


/**
 * @brief Configures NTP time synchronization with timezone settings.
 *
 * Initializes network time protocol synchronization using configured NTP servers
 * and timezone offsets. Attempts multiple sync attempts with timeout handling.
 * Provides fallback to system boot time if NTP sync fails.
 */
void setupTime() {
    unsigned long ntpStartTime = millis();
    unsigned long ntpLastLogTime = ntpStartTime;
    unsigned long ntpCurrentTime;

    Serial.println("Setting up NTP time sync...");
    // Use timezone and NTP server info from config.h
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    
    ntpCurrentTime = millis();
    // Serial.printf("--- NTP - configTime Called: %lu ms, Total: %lu ms\n", ntpCurrentTime - ntpLastLogTime, ntpCurrentTime - ntpStartTime);
    ntpLastLogTime = ntpCurrentTime;

    int attempts = 0;
    struct tm timeinfo;
    while (!getLocalTime(&timeinfo, 10000) && attempts < 10) { // Max 10 seconds wait, 10 attempts
        Serial.print(".");
        delay(1000); // Wait for NTP response
        
        ntpCurrentTime = millis();
        // Serial.printf("\n--- NTP - Attempt %d getLocalTime Failed & Waited: %lu ms, Loop Total: %lu ms\n", attempts + 1, ntpCurrentTime - ntpLastLogTime, ntpCurrentTime - ntpStartTime);
        ntpLastLogTime = ntpCurrentTime;
        attempts++;
    }

    if (getLocalTime(&timeinfo)) {
        Serial.println("\nNTP time sync successful!");
        Serial.printf("Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                    timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        Serial.println("\nNTP time sync failed! Using system boot time.");
    }
    ntpCurrentTime = millis();
    Serial.printf("--- NTP - Sync Process Finished: %lu ms, Total NTP Setup: %lu ms\n\n", ntpCurrentTime - ntpStartTime, ntpCurrentTime - ntpStartTime);
}

/**
 * @brief Executes the run() method of the currently active display mode.
 * 
 * Routes execution to the appropriate mode handler based on currentMode.
 * Also processes any pending IR commands received by the IRManager.
 * Handles unknown modes by switching back to clock mode as fallback.
 */
void updateCurrentMode() {
    switch(currentMode) {
        case MODE_CLOCK:     modeClock.run();     break;
        case MODE_MQTT:      modeMqtt.run();      break;
        case MODE_COUNTDOWN: modeCountdown.run(); break;
        case MODE_PATTERN:   modePattern.run();   break;
        case MODE_IMAGE:     modeImage.run();     break;
        case MODE_GIF:       modeGif.run();       break;
        case MODE_FONT:      modeFont.run();      break;
        case MODE_SYSINFO:   modeSysinfo.run();   break;
        case MODE_IR_SCAN:   modeIRScan.run();    break;
        default:
            Serial.printf("Error: Unknown currentMode %d\n", (int)currentMode);
            switchMode(MODE_CLOCK, true); // Switch to default mode if current mode is unknown
            break;
    }

    // Check and process new IR commands received by IRManager
    if (irManager.hasNewCommand()) {
        lastUserActivityTime = millis(); // User sent an IR command
        handleIRCommand(irManager.getLastCommand());
        irManager.clearCommand(); // Clear command flag after processing
    }
}

/**
 * @brief Maintains MQTT connection health and handles reconnection attempts.
 * 
 * Periodically calls mqttClient.loop() to process incoming messages and
 * maintain connection. Implements exponential backoff reconnection strategy
 * when connection is lost. Includes extended delay after multiple failures.
 */
void maintainMqttConnections() {
    static unsigned long lastMqttLoopTime = 0;
    static unsigned long lastMqttReconnectAttemptTime = 0;
    static int mqttReconnectAttemptCount = 0;

    // MQTT client loop (needs to be called periodically)
    if (mqttClient.connected()) {
        if (millis() - lastMqttLoopTime > 100) { // Call loop every 100ms
            if (!mqttClient.loop()) {
                Serial.println("MQTT loop failed, connection might be lost.");
                // Connection might be lost, so set a flag to proceed to immediate reconnection attempt logic
            }
            lastMqttLoopTime = millis();
        }
    } else { // If MQTT connection is lost
        if (WiFi.status() == WL_CONNECTED) { // Retry only if WiFi is connected
            unsigned long backoffDelay = 5000 * (1 << min(mqttReconnectAttemptCount, 4)); // Max 80 seconds (5*16)
            if (millis() - lastMqttReconnectAttemptTime > backoffDelay) {
                Serial.printf("MQTT disconnected. Attempting reconnection (attempt %d)...\n", mqttReconnectAttemptCount + 1);
                if (connectMQTT()) {
                    mqttReconnectAttemptCount = 0; // Reset count on success
                } else {
                    mqttReconnectAttemptCount++;
                    if (mqttReconnectAttemptCount > 5) { // After 5 attempts, wait for a long time
                        Serial.println("MQTT reconnection failed multiple times. Will retry in 5 minutes.");
                        lastMqttReconnectAttemptTime = millis() + 240000; // Wait an additional 4 minutes (total 5 minutes)
                        mqttReconnectAttemptCount = 0; // Reset count
                    }
                }
                lastMqttReconnectAttemptTime = millis();
            }
        }
    }
}
