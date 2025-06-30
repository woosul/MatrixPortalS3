#ifndef MODE_MQTT_H
#define MODE_MQTT_H

#include "config.h"
// #include <ArduinoJson.h>

// Forward declarations
class Utils;
class MatrixPanel_I2S_DMA;

class ModeMqtt {
public:
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr);
    void run();
    void cleanup();
    void displayMqttMessage(const char* stage_str, const char* code_str, const char* message_str = nullptr);
    bool isRequestDirectMessageDisplay; // True if a direct message display request is made

    bool isDisplayingMessage() const;
    void setIsDisplayingMessage(bool displaying);
    bool isModeSetupDone() const;
    void setIsModeSetupDone(bool setupDone);

private:
    Utils* m_utils;
    MatrixPanel_I2S_DMA* m_matrix;
    
    void standbyMqtt();
    // Helper method for stage mark
    char getStageMarkChar(const char* stage);
    
    // Member variables
    // char lastDisplayedMessage[256]; // Removed, duplicate check handled in main.cpp
    bool isStandbyScreenActive; // To track if standby screen is currently displayed
    void displayMessageWithBackground(const char* stage, char stageMark, const char* code, const char* message,
                                    uint16_t backgroundColor, bool invertText);
    bool isInitialDisplayDone; // True if the initial standby screen has been shown for the current activation

    // Store current display parameters for redraw during scrolling
    char currentStageForDisplay[32];
    char currentStageMarkForDisplay;
    char currentCodeForDisplay[32];
    char currentMessageForDisplay[256];
    uint16_t currentBgColorForDisplay;

};

#endif
