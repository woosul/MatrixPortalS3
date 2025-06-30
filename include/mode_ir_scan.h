#ifndef MODE_IR_SCAN_H
#define MODE_IR_SCAN_H

#include "utils.h"
#include "ir_manager.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

// enum for learning state
enum LearningState {
    READY,              // Ready state before initial waiting state
    KEY_INPUT,          // Waiting for key name input
    SCAN,               // Waiting for IR signal scan
    MODE_SELECT,        // Waiting for mode selection (Blocking/Non-blocking)
    DUPLICATE_CONFIRM,  // Waiting for duplicate confirmation
    EXIT_MODE           // Exit current operation or mode
};

// enum for IR mode
enum IRMode {
    YAHBOOM_BLOCKING_MODE,
    NEC_NON_BLOCKING_MODE
};

// Learned key structure
struct LearnedKey {
    bool isValid;
    char keyName[32];
    uint32_t irCode;
};

class ModeIRScan {
public:
    ModeIRScan();
    
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr, IRManager* irManager_ptr);
    void run();
    void cleanup();

private:
    // Core functionality
    void runInternalLoop();
    void showGuide();
    void scanProcessDisplay(const char* phaseText);
    
    // Input processing
    void processUserInput(const String& input);
    void handleKeyNameInput(const String& input);
    void handleModeSelection(const String& input);
    void handleDuplicateChoice(const String& input);
    
    // IR learning
    void handleIRScan();
    void processReceivedCode(uint32_t code);
    
    // Key management
    bool storeKey(const char* keyName, uint32_t irCode);
    bool isValidKeyName(const char* keyName);
    bool isDuplicateCode(uint32_t irCode);
    int findKeyIndex(const char* keyName);
    int findEmptySlot();
    
    // Commands
    void showSummary();
    void updateCodes();
    void clearKeys();
    void showPrompt();
    void updateDisplay();
    void startScanMode();
    void handleModeCommand();
    
    // Member variables
    Utils* m_utils;
    MatrixPanel_I2S_DMA* m_matrix;
    IRManager* m_irManager;
    
    // State variables
    LearningState learningState;
    IRMode selectedMode;
    bool isFirstRun;
    
    // Learning data
    LearnedKey keys[50];
    char currentKeyName[32];
    int totalLearnedKeys;
    uint32_t duplicateIRCode;
};

#endif // MODE_IR_SCAN_H