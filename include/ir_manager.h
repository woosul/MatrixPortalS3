#ifndef IR_MANAGER_H
#define IR_MANAGER_H

#include <Arduino.h> // For uint32_t, unsigned long, etc.
#include "config.h"  // For IR_RECEIVER_PIN, IR_CODE_MAP, NUM_BUTTONS/IR_CODE_MAP_SIZE

// Forward declaration for Utils class to avoid circular dependencies
class Utils;

// IRCodeMapping struct is defined in config.h

// State machine states for non-blocking IR reception
enum class IRState {
    IDLE,
    START_LOW,
    START_HIGH,
    DATA_LOW,
    DATA_HIGH,
    REPEAT_HIGH,
    REPEAT_GAP,
    DECODING,
    ERROR     // Decoding error, reset
};

class IRManager {
public:
    // Enum for selecting IR reception mode
    enum class IRReceiveMode {
        NEC_NON_BLOCKING,   // Original non-blocking NEC decoder
        YAHBOOM_BLOCKING    // Blocking raw signal decoder (like old Utils::readIRCode)
    };

    void setup(Utils* utils_ptr);
    void update(); // Primarily for NEC_NON_BLOCKING mode
    bool hasNewCommand();
    uint32_t getLastCommand();
    void clearCommand();

    // Mode selection
    void setReceiveMode(IRReceiveMode mode);
    IRReceiveMode getCurrentReceiveMode() const { return currentReceiveMode; }

    // Blocking raw IR code reader
    // Updates lastCommand and newCommandAvailable internally if a code is read.
    void readRawCodeYahboom(unsigned long timeout_ms = 100); // Timeout for waiting for a signal

    // Configuration and scanning
    void enableScanMode(bool enable = true);
    bool isScanModeEnabled() const { return scanMode; }
    void configureCode(const char* buttonName, uint32_t code);
    void printCodeMappings() const;
    void resetToDefaults();

    // Button checking (primarily for NEC mode or after a raw code is processed)
    bool isButtonPressed(const char* buttonName);
    uint32_t getButtonCode(const char* buttonName);

    // Sound feedback control
    void enableSoundFeedback(bool enable = true);
    bool isSoundFeedbackEnabled() const { return soundFeedbackEnabled; }

private:
    Utils* utils;

    uint32_t lastCommand;
    unsigned long lastReceiveTime;
    bool newCommandAvailable;
    bool scanMode;
    bool soundFeedbackEnabled;

    // Common IR variables
    int irPin;
    IRReceiveMode currentReceiveMode;

    // Variables for NEC_NON_BLOCKING mode
    IRState currentState;
    unsigned long lastEdgeTime_us;
    unsigned long pulseStartTime_us;
    uint32_t rawData;
    int bitCount;
    bool isRepeat;

    // Variables for RAW_BLOCKING mode (can be local to readRawCodeYahboom or members if state needs to persist)
    // For simplicity, some might be local to the readRawCodeYahboom method.

    // Command timeout (ignore repeated signals in NEC mode)
    static const unsigned long COMMAND_TIMEOUT = 200; // ms

    // Timings for NEC protocol
    static const unsigned long NEC_START_LOW_MIN = 8500;
    static const unsigned long NEC_START_LOW_MAX = 9500;
    static const unsigned long NEC_START_HIGH_MIN = 4000;
    static const unsigned long NEC_START_HIGH_MAX = 5000;
    static const unsigned long NEC_DATA_LOW_MIN = 400;
    static const unsigned long NEC_DATA_LOW_MAX = 700;
    static const unsigned long NEC_BIT_HIGH_SHORT_MIN = 400;
    static const unsigned long NEC_BIT_HIGH_SHORT_MAX = 700;
    static const unsigned long NEC_BIT_HIGH_LONG_MIN = 1500;
    static const unsigned long NEC_BIT_HIGH_LONG_MAX = 1800;
    static const unsigned long NEC_REPEAT_HIGH_MIN = 2000;
    static const unsigned long NEC_REPEAT_HIGH_MAX = 2500;
    static const unsigned long NEC_REPEAT_GAP_MIN = 100000;
    static const unsigned long NEC_REPEAT_GAP_MAX = 120000;
    static const unsigned long STATE_TIMEOUT_US = 50000; // us, for NEC non-blocking

    // Constants for RAW_BLOCKING mode (from old Utils::readIRCode)
    static const uint8_t RAW_NO_DATA = 0xFE; // Special value for no valid IR data in RAW mode
    static const int RAW_REPEAT_CNT_THRESHOLD = 110; // From old Utils::readIRCode
    static const int RAW_MAX_COUNT_SHORT_PULSE = 30; // From old Utils::readIRCode
    static const int RAW_MAX_COUNT_LONG_PULSE = 80;  // From old Utils::readIRCode
    static const int RAW_BIT_THRESHOLD_COUNT = 35;   // From old Utils::readIRCode
    static const int RAW_DELAY_MICROSECONDS = 30;    // From old Utils::readIRCode

    static const int NUM_BUTTONS = IR_CODE_MAP_SIZE;
    IRCodeMapping buttonMappings[NUM_BUTTONS];

    int findButtonIndex(const char* buttonName) const;
    int findButtonIndexByCode(uint32_t code);
};

#endif