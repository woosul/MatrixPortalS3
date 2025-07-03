#ifndef MODE_IR_SCAN_H
#define MODE_IR_SCAN_H

#include "utils.h"
#include "ir_manager.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/**
 * @brief Enumeration defining the different states of the IR learning process
 */
enum LearningState {
    READY,              // Ready state waiting for user commands
    KEY_INPUT,          // Waiting for IR key name input from user
    SCAN,               // Actively scanning for IR signal reception
    MODE_SELECT,        // Waiting for IR mode selection (Blocking/Non-blocking)
    DUPLICATE_CONFIRM,  // Waiting for user confirmation on duplicate IR code
    EXIT_MODE           // Exit the current operation or mode
};

/**
 * @brief Enumeration defining the available IR reception modes
 */
enum IRMode {
    YAHBOOM_BLOCKING_MODE,
    NEC_NON_BLOCKING_MODE
};

/**
 * @brief Structure to store learned IR key information
 */
struct LearnedKey {
    bool isValid;           // Flag indicating if this slot contains valid data
    char keyName[32];       // Human-readable name for the IR key
    uint32_t irCode;        // The actual IR command code value
};

/**
 * @brief IR Remote Learning Mode Class
 * 
 * This class provides functionality for learning IR remote control codes
 * and mapping them to human-readable key names. It supports both blocking
 * and non-blocking IR reception modes.
 */
class ModeIRScan {
public:
    /**
     * @brief Constructor for ModeIRScan
     */
    ModeIRScan();
    
    /**
     * @brief Initialize the IR scan mode with required dependencies
     * @param utils_ptr Pointer to utilities instance
     * @param matrix_ptr Pointer to LED matrix display instance
     * @param irManager_ptr Pointer to IR manager instance
     */
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr, IRManager* irManager_ptr);
    
    /**
     * @brief Main execution loop for IR scan mode
     */
    void run();
    
    /**
     * @brief Clean up resources and reset state when exiting mode
     */
    void cleanup();

private:
    /**
     * @brief Internal loop that handles the learning process
     */
    void runInternalLoop();
    
    /**
     * @brief Display usage instructions and available commands
     */
    void showGuide();
    
    /**
     * @brief Update the LED matrix display with current scan phase
     * @param phaseText Text to display indicating current phase
     */
    void scanProcessDisplay(const char* phaseText);
    
    /**
     * @brief Process user input commands and route to appropriate handlers
     * @param input User input string to process
     */
    void processUserInput(const String& input);
    
    /**
     * @brief Handle input of IR key names during learning process
     * @param input Key name entered by user
     */
    void handleKeyNameInput(const String& input);
    
    /**
     * @brief Handle user selection of IR reception mode
     * @param input Mode selection input from user
     */
    void handleModeSelection(const String& input);
    
    /**
     * @brief Handle user confirmation for duplicate IR code overwrites
     * @param input User confirmation input (Y/N)
     */
    void handleDuplicateChoice(const String& input);
    
    /**
     * @brief Main IR signal scanning and reception handler
     */
    void handleIRScan();
    
    /**
     * @brief Process received IR code and store or handle duplicates
     * @param code The IR code received from remote
     */
    void processReceivedCode(uint32_t code);
    
    /**
     * @brief Store a learned IR key in the internal storage
     * @param keyName Human-readable name for the key
     * @param irCode IR command code value
     * @return true if key was successfully stored, false otherwise
     */
    bool storeKey(const char* keyName, uint32_t irCode);
    
    /**
     * @brief Validate that a key name follows naming conventions
     * @param keyName Key name to validate
     * @return true if key name is valid, false otherwise
     */
    bool isValidKeyName(const char* keyName);
    
    /**
     * @brief Check if an IR code is already stored
     * @param irCode IR code to check for duplicates
     * @return true if code already exists, false otherwise
     */
    bool isDuplicateCode(uint32_t irCode);
    
    /**
     * @brief Find the index of a stored key by name
     * @param keyName Name of key to find
     * @return Index of key if found, -1 otherwise
     */
    int findKeyIndex(const char* keyName);
    
    /**
     * @brief Find the first available empty storage slot
     * @return Index of empty slot if available, -1 if storage is full
     */
    int findEmptySlot();
    
    /**
     * @brief Display summary of all learned keys
     */
    void showSummary();
    
    /**
     * @brief Generate config.h format output for learned keys
     */
    void updateCodes();
    
    /**
     * @brief Clear all learned keys from storage
     */
    void clearKeys();
    
    /**
     * @brief Display appropriate input prompt based on current state
     */
    void showPrompt();
    
    /**
     * @brief Update the LED matrix display with current status
     */
    void updateDisplay();
    
    /**
     * @brief Initialize key learning scan mode
     */
    void startScanMode();
    
    /**
     * @brief Handle mode selection command
     */
    void handleModeCommand();
    
    // Core component references
    Utils* m_utils;                     // Pointer to utility functions instance
    MatrixPanel_I2S_DMA* m_matrix;      // Pointer to LED matrix display instance
    IRManager* m_irManager;             // Pointer to IR signal manager instance
    
    // Current operational state
    LearningState learningState;        // Current state in the learning process
    IRMode selectedMode;                // Currently selected IR reception mode
    bool isFirstRun;                    // Flag to track first execution of run()
    
    // IR key learning storage
    LearnedKey keys[50];                // Array to store learned IR key mappings
    char currentKeyName[32];            // Buffer for currently processing key name
    int totalLearnedKeys;               // Count of successfully learned keys
    uint32_t duplicateIRCode;           // Temporary storage for duplicate code confirmation

    // Real-time input processing
    char inputBuffer[64];               // Buffer for accumulating user input
    int bufferIndex;                    // Current position in input buffer
    
    /**
     * @brief Handle real-time keyboard input with immediate echo
     * @return true if complete input was processed, false otherwise
     */
    bool handleRealTimeInput();
    
    /**
     * @brief Enumeration for IR scan operation states
     */
    enum class ScanState {
        WAITING_FOR_INPUT,              // Waiting for key name input from user
        SCANNING_IR,                    // Actively scanning for IR signal
        PROCESSING_RESULT               // Processing received IR signal result
    };
    
    // IR scan timing and state management
    ScanState scanState;                // Current state of IR scanning process
    unsigned long scanStartTime;        // Timestamp when IR scan was initiated
    bool irScanInitiated;               // Flag indicating if IR scan has been started
};

#endif // MODE_IR_SCAN_H