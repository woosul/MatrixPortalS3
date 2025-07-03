#include "mode_ir_scan.h"
#include "common.h"
#include "utils.h"
#include "ir_manager.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

/**
 * @brief Constructor for ModeIRScan class
 * 
 * Initializes the IR scan mode with default values and prepares
 * the learning key storage array.
 */
ModeIRScan::ModeIRScan() : m_irManager(nullptr), selectedMode(YAHBOOM_BLOCKING_MODE), 
                           isFirstRun(true), duplicateIRCode(0) {
    totalLearnedKeys = 0;
    strcpy(currentKeyName, "");
}

/**
 * @brief Initialize the IR scan mode with required component references
 * 
 * Sets up the mode with pointers to utility functions, matrix display,
 * and IR manager. Initializes all internal state variables and storage arrays.
 */
void ModeIRScan::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr, IRManager* irManager_ptr) {
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;
    m_irManager = irManager_ptr;
    
    learningState = READY;
    selectedMode = YAHBOOM_BLOCKING_MODE; 
    totalLearnedKeys = 0;
    
    // Initialize input buffer for real-time processing
    bufferIndex = 0;
    inputBuffer[0] = '\0';
    
    // Initialize IR scan state management
    scanState = ScanState::WAITING_FOR_INPUT;
    scanStartTime = 0;
    irScanInitiated = false;

    // Initialize learned key storage array
    for (int i = 0; i < 50; i++) {
        keys[i].isValid = false;
        strcpy(keys[i].keyName, "");
        keys[i].irCode = 0;
    }

    Serial.println("IR Scan mode ready.");
}

/**
 * @brief Main execution loop for IR scan mode
 * 
 * Handles the initial display and guide on first run, then manages
 * different operational states including READY state for command input
 * and internal loop for active learning processes.
 */
void ModeIRScan::run() {
    if (isFirstRun) {
        // Display initial state and show usage guide on first execution
        updateDisplay();
        showGuide();
        isFirstRun = false;
    }

    if (learningState == READY) {
        // Handle READY state: Process serial input and receive IR signals from main loop
        // Process real-time user input
        handleRealTimeInput();
        updateDisplay();
    } else if (learningState != EXIT_MODE) {
        // Enter internal loop only when transitioning from READY via SCAN command
        // and not in a final EXIT state.
        runInternalLoop();
    }
}

/**
 * @brief Internal processing loop for active learning operations
 * 
 * Continues processing until EXIT_MODE is reached, handling real-time input
 * and IR scanning operations with periodic display updates.
 */
void ModeIRScan::runInternalLoop() {
    // Continue loop until EXIT_MODE is requested
    while (learningState != EXIT_MODE && learningState != READY) {
        handleRealTimeInput();
        
        if (learningState == SCAN) {
            handleIRScan();
        }
        
        updateDisplay();
        delay(10);
    }
    
    if (learningState == EXIT_MODE) {
        Serial.println("Exiting IR Scan internal loop...");
    }
}

/**
 * @brief Process and route user input commands to appropriate handlers
 * 
 * Parses user input and routes to the correct handler based on current state.
 * Handles global commands that are available in any state, as well as
 * state-specific commands.
 */
void ModeIRScan::processUserInput(const String& input) {
    // Add newline before processing to ensure clean output formatting
    Serial.println();
    
    // Handle global commands that are available in any state
    if (input == "SUMMARY") {
        showSummary();
        return;
    } else if (input == "UPDATE") {
        updateCodes();
        return;
    } else if (input == "CLEAR") {
        clearKeys();
        return;
    } else if (input == "DONE" || input == "EXIT") {
        if (learningState == KEY_INPUT || learningState == SCAN) {
            learningState = READY;
            updateDisplay();
            Serial.println("SCAN mode ended. Back to READY.");
            showPrompt();
        } else {
            learningState = EXIT_MODE;
            Serial.println("Exiting...");
        }
        return;
    } else if (input == "SCAN") {
        startScanMode();
        return;
    } else if (input == "MODE") {
        handleModeCommand();
        return;
    }
    
    // Handle state-specific commands
    switch (learningState) {
        case READY:
            Serial.println("Invalid command in READY state!");
            showPrompt();
            break;
        case KEY_INPUT:
            handleKeyNameInput(input);
            break;
        case MODE_SELECT:
            handleModeSelection(input);
            break;
        case DUPLICATE_CONFIRM:
            handleDuplicateChoice(input);
            break;
        case SCAN:
            // Ready to receive IR signals in SCAN state
            Serial.println("Waiting for IR signal... Use DONE to cancel.");
            break;
        default:
            Serial.println("Invalid command!");
            showPrompt();
            break;
    }
}

/**
 * @brief Handle IR key name input and validate before starting scan
 * 
 * Validates the provided key name and transitions to SCAN state if valid.
 * Initializes scan state variables for the IR signal reception process.
 */
void ModeIRScan::handleKeyNameInput(const String& input) {
    if (isValidKeyName(input.c_str())) {
        strcpy(currentKeyName, input.c_str());
        learningState = SCAN;
        
        // Initialize scan state variables
        scanState = ScanState::WAITING_FOR_INPUT;
        scanStartTime = 0;
        irScanInitiated = false;
        
        Serial.printf("\nLearning '%s' - press IR button now\n", currentKeyName);
        updateDisplay();
    } else {
        Serial.println("Invalid key name!");
        showPrompt();
    }
}

/**
 * @brief Initialize key learning scan mode
 * 
 * Transitions to KEY_INPUT state and prompts user for IR key name input.
 */
void ModeIRScan::startScanMode() {
    learningState = KEY_INPUT;
    updateDisplay();
    showPrompt();
}

/**
 * @brief Handle mode selection command and display available options
 * 
 * Transitions to MODE_SELECT state and displays current mode along with
 * available IR reception mode options.
 */
void ModeIRScan::handleModeCommand() {
    learningState = MODE_SELECT;
    const char* currentModeStr = (selectedMode == YAHBOOM_BLOCKING_MODE) ? "YAHBOOM-Blocking" : "NEC-Non-blocking";
    Serial.printf("Current mode: %s\n", currentModeStr);
    Serial.println("Select mode: 1=YAHBOOM-Blocking, 2=NEC-Non-blocking");
    Serial.println("---------------------------------------------------------------------");
    showPrompt();
}

/**
 * @brief Handle user selection of IR reception mode
 * 
 * Processes user input for mode selection and updates the selected IR mode.
 * Returns to READY state after successful mode selection.
 */
void ModeIRScan::handleModeSelection(const String& input) {
    if (input == "1") {
        selectedMode = YAHBOOM_BLOCKING_MODE;
        Serial.println("YAHBOOM-Blocking mode selected");
    } else if (input == "2") {
        selectedMode = NEC_NON_BLOCKING_MODE;
        Serial.println("NEC-Non-blocking mode selected");
    } else {
        Serial.println("Invalid selection!");
        showPrompt();
        return;
    }
    
    learningState = READY;
    updateDisplay();
    showPrompt();
}

/**
 * @brief Handle user confirmation for duplicate IR code overwrites
 * 
 * Processes user confirmation (Y/N) when a duplicate IR code is detected
 * and either updates the existing key or cancels the operation.
 */
void ModeIRScan::handleDuplicateChoice(const String& input) {
    if (input == "Y" || input == "YES") {
        if (storeKey(currentKeyName, duplicateIRCode)) {
            Serial.printf("Key '%s' updated!\n", currentKeyName);
            m_utils->playSingleTone();
        }
    } else {
        Serial.println("Cancelled.");
    }
    
    learningState = KEY_INPUT;
    updateDisplay();
    showPrompt();
}

/**
 * @brief Main IR signal scanning and reception handler
 * 
 * Manages the IR scanning process using state machine logic to handle
 * both blocking and non-blocking IR reception modes with timeout handling.
 */
void ModeIRScan::handleIRScan() {
    if (!m_irManager) return;
    
    bool codeReceived = false;
    uint32_t receivedCode = 0;
    
    // State-based scan control
    switch (scanState) {
        case ScanState::WAITING_FOR_INPUT:
            // Initialize scan process
            scanState = ScanState::SCANNING_IR;
            scanStartTime = millis();
            irScanInitiated = false;
            // Fall through to SCANNING_IR
            
        case ScanState::SCANNING_IR:
            if (selectedMode == YAHBOOM_BLOCKING_MODE) {
                if (!irScanInitiated) {
                    // Blocking mode: Initialize scan once
                    m_irManager->setReceiveMode(IRManager::IRReceiveMode::YAHBOOM_BLOCKING);
                    m_irManager->enableScanMode(true);
                    m_irManager->clearCommand();
                    irScanInitiated = true;
                }
                
                // Check for timeout (10 seconds)
                if (millis() - scanStartTime > 10000) {
                    scanState = ScanState::WAITING_FOR_INPUT;
                    irScanInitiated = false;
                    learningState = KEY_INPUT;
                    Serial.println("IR scan timeout. Ready for next key name.");
                    showPrompt();
                    return;
                }
                
                // Check for received IR signal with short timeout
                m_irManager->readRawCodeYahboom(100);
                
                if (m_irManager->hasNewCommand()) {
                    receivedCode = m_irManager->getLastCommand();
                    codeReceived = true;
                    scanState = ScanState::PROCESSING_RESULT;
                }
            } else {
                // NEC non-blocking mode
                if (!irScanInitiated) {
                    m_irManager->setReceiveMode(IRManager::IRReceiveMode::NEC_NON_BLOCKING);
                    m_irManager->enableScanMode(true);
                    irScanInitiated = true;
                }
                
                // Check for timeout (10 seconds)
                if (millis() - scanStartTime > 10000) {
                    scanState = ScanState::WAITING_FOR_INPUT;
                    irScanInitiated = false;
                    learningState = KEY_INPUT;
                    Serial.println("IR scan timeout. Ready for next key name.");
                    showPrompt();
                    return;
                }
                
                m_irManager->updateReadNonBlocking();
                
                if (m_irManager->hasNewCommand()) {
                    receivedCode = m_irManager->getLastCommand();
                    codeReceived = true;
                    Serial.printf("IR code (NEC) received: 0x%02X\n", receivedCode);
                    scanState = ScanState::PROCESSING_RESULT;
                }
            }
            break;
            
        case ScanState::PROCESSING_RESULT:
            // Reset to waiting state after result processing
            scanState = ScanState::WAITING_FOR_INPUT;
            irScanInitiated = false;
            break;
    }
    
    if (codeReceived) {
        processReceivedCode(receivedCode);
    }
}

/**
 * @brief Process received IR code and handle storage or duplicates
 * 
 * Processes the received IR code by checking for duplicates and either
 * storing the new key or prompting for confirmation to overwrite existing keys.
 */
void ModeIRScan::processReceivedCode(uint32_t code) {
    // Clear IR command immediately after processing to prevent reprocessing in main loop
    m_irManager->clearCommand();
    
    // Check for duplicate IR codes
    if (isDuplicateCode(code)) {
        Serial.printf("Warning: Code 0x%02X already exists!\n", code);
        duplicateIRCode = code;
        learningState = DUPLICATE_CONFIRM;
        updateDisplay();
        Serial.printf("Overwrite? (Y/N): ");
        return;
    }

    // Store the new key
    if (storeKey(currentKeyName, code)) {
        m_utils->playSingleTone();
    } else {
        Serial.println("Failed to store key!");
        m_utils->playErrorTone();
    }
    
    // Continue SCAN mode: Return to KEY_INPUT state for next key name
    learningState = KEY_INPUT;
    
    // Reset scan state variables
    scanState = ScanState::WAITING_FOR_INPUT;
    irScanInitiated = false;
    
    updateDisplay();
    showPrompt();
}

/**
 * @brief Store a learned IR key in the internal storage array
 * 
 * Stores the provided key name and IR code, either updating an existing
 * key or creating a new entry if space is available.
 */
bool ModeIRScan::storeKey(const char* keyName, uint32_t irCode) {
    int index = findKeyIndex(keyName);
    
    if (index < 0) {
        // Find empty slot for new key
        index = findEmptySlot();
        if (index < 0) {
            Serial.println("No space available!");
            return false;
        }
        totalLearnedKeys++;
    }
    
    keys[index].isValid = true;
    strcpy(keys[index].keyName, keyName);
    keys[index].irCode = irCode;

    Serial.printf("Key '%s' stored with code 0x%02X\n", keys[index].keyName, keys[index].irCode);
    return true;
}

/**
 * @brief Validate IR key name according to naming conventions
 * 
 * Checks that the key name meets requirements: non-empty, within length limits,
 * starts with alphanumeric character, and contains only alphanumeric characters and underscores.
 */
bool ModeIRScan::isValidKeyName(const char* keyName) {
    if (!keyName || strlen(keyName) == 0 || strlen(keyName) >= 32) {
        return false;
    }

    // First character must be alphanumeric
    if (!isalnum(keyName[0])) return false;

    // Remaining characters must be alphanumeric or underscore
    for (int i = 1; keyName[i]; i++) {
        if (!isalnum(keyName[i]) && keyName[i] != '_') {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Find the storage index of a key by name (case-insensitive)
 * 
 * Searches the learned keys array for a key with the specified name.
 * Returns the index if found, -1 if not found.
 */
int ModeIRScan::findKeyIndex(const char* keyName) {
    for (int i = 0; i < 50; i++) {
        if (keys[i].isValid && strcasecmp(keys[i].keyName, keyName) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Find the first available empty slot in the key storage array
 * 
 * Searches for the first invalid/empty slot that can be used for storing
 * a new learned key. Returns the index if found, -1 if storage is full.
 */
int ModeIRScan::findEmptySlot() {
    for (int i = 0; i < 50; i++) {
        if (!keys[i].isValid) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Check if an IR code already exists in the learned keys
 * 
 * Searches through all valid learned keys to determine if the specified
 * IR code has already been stored.
 */
bool ModeIRScan::isDuplicateCode(uint32_t irCode) {
    for (int i = 0; i < 50; i++) {
        if (keys[i].isValid && keys[i].irCode == irCode) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Display usage instructions and available commands
 * 
 * Shows the user guide with all available commands and their descriptions
 * for the IR learning mode.
 */
void ModeIRScan::showGuide() {
    Serial.println("IR REMOTE LEARNING MODE");
    Serial.println("---------------------------------------------------------------------");
    Serial.println("Commands:");
    Serial.println(" SCAN     - Start key learning process");
    Serial.println(" SUMMARY  - Show all learned keys");
    Serial.println(" UPDATE   - Generate config.h format");
    Serial.println(" CLEAR    - Clear all learned keys");
    Serial.println(" MODE     - Change IR mode (Blocking/Non-blocking)");
    Serial.println(" DONE     - Exit current operation or mode");
    Serial.println("---------------------------------------------------------------------");
    showPrompt();
}

/**
 * @brief Display summary of all learned IR keys
 * 
 * Shows a formatted list of all successfully learned keys with their
 * associated IR codes and displays the total count.
 */
void ModeIRScan::showSummary() {
    Serial.println();
    Serial.println("# LEARNED KEYS ----------------");
    if (totalLearnedKeys == 0) {
        Serial.println("No keys learned yet.");
    } else {
        for (int i = 0; i < 50; i++) {
            if (keys[i].isValid) {
                Serial.printf("    %s = 0x%02X\n", keys[i].keyName, keys[i].irCode);
            }
        }
    }
    Serial.printf("Total: %d/50\n", totalLearnedKeys);
    showPrompt();
}

/**
 * @brief Generate config.h format definitions for learned IR keys
 * 
 * Outputs all learned keys in C preprocessor #define format suitable
 * for inclusion in config.h files. Plays feedback tones based on success.
 */
void ModeIRScan::updateCodes() {
    if (!m_irManager) {
        Serial.println("ERROR: IRManager not set!");
        return;
    }
    Serial.println();
    Serial.println("# CONFIG.H IR_CODE_MAP FORMAT ----------------");
    if (totalLearnedKeys == 0) {
        Serial.println("No keys to update.");
    } else {
        for (int i = 0; i < 50; i++) {
            if (keys[i].isValid && keys[i].irCode != 0) {
                Serial.printf("#define IR_%s    0x%02X                    // %s button code\n", 
                    keys[i].keyName, keys[i].irCode, keys[i].keyName);
            }
        }
    }
    
    Serial.printf("Generated %d code definitions. ", totalLearnedKeys);
    if (totalLearnedKeys > 0) {
        m_utils->playSingleTone();
    } else {
        m_utils->playErrorTone();
    }
    // Add newline for clean formatting before next prompt
    Serial.println();
    showPrompt();
}

/**
 * @brief Clear all learned IR keys from storage
 * 
 * Resets the entire learned keys array and counter, effectively
 * clearing all previously stored IR key mappings.
 */
void ModeIRScan::clearKeys() {
    // Reset all key storage slots
    for (int i = 0; i < 50; i++) {
        keys[i].isValid = false;
        strcpy(keys[i].keyName, "");
        keys[i].irCode = 0;
    }
    totalLearnedKeys = 0;
    
    Serial.println("All learned keys cleared.");
    m_utils->playSingleTone();
    showPrompt();
}

/**
 * @brief Display appropriate input prompt based on current learning state
 * 
 * Shows context-appropriate prompts and available commands based on
 * the current operational state and selected IR mode.
 */
void ModeIRScan::showPrompt() {
    const char* modeStr = (selectedMode == YAHBOOM_BLOCKING_MODE) ? "(Blocking)" : "(Non-blocking)";
    
    switch (learningState) {
        case READY:
            Serial.printf("Commands: SCAN | SUMMARY | UPDATE | CLEAR | MODE %s | DONE\n", modeStr);
            Serial.println("---------------------------------------------------------------------");
            Serial.print("Input command : ");
            break;
        case KEY_INPUT:
            Serial.println();
            Serial.print("Command or IR key name : ");
            break;
        case MODE_SELECT:
            Serial.print("Select scan mode (1/2) : ");
            break;
        case DUPLICATE_CONFIRM:
            Serial.printf("Overwrite key with code 0x%02X? (Y/N): ", duplicateIRCode);
            break;
        default:
            break;
    }
}

/**
 * @brief Update the LED matrix display with current operational status
 * 
 * Periodically updates the matrix display with the current learning state
 * to provide visual feedback to the user.
 */
void ModeIRScan::updateDisplay() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 1000) {
        const char* stateText = "READY";
        switch (learningState) {
            case SCAN: stateText = "SCAN"; break;
            case KEY_INPUT: stateText = "INPUT"; break;
            case MODE_SELECT: stateText = "MODE"; break;
            case DUPLICATE_CONFIRM: stateText = "CONFIRM"; break;
            case READY: stateText = "READY"; break;
        }
        scanProcessDisplay(stateText);
        lastUpdate = millis();
    }
}

/**
 * @brief Display current scan phase on LED matrix
 * 
 * Renders the IR scan status information on the LED matrix display
 * with appropriate colors and positioning.
 */
void ModeIRScan::scanProcessDisplay(const char* phaseText) {
    m_matrix->fillScreen(0);

    const char* str1 = "IR SCAN";
    char str2[32];
    snprintf(str2, sizeof(str2), "%s", phaseText);

    int str1_x = m_utils->calculateTextCenterX(str1, MATRIX_WIDTH);
    int str2_x = m_utils->calculateTextCenterX(str2, MATRIX_WIDTH);
    
    // Display "IR SCAN" in gray
    m_matrix->setTextColor(m_utils->hexToRgb565(0x808080));
    m_utils->setCursorTopBased(str1_x, 4, false);
    m_matrix->print(str1);
    
    // Display current phase in white
    m_matrix->setTextColor(m_utils->hexToRgb565(0xFFFFFF));
    m_utils->setCursorTopBased(str2_x, 28, false);
    m_matrix->print(str2);
    
    m_utils->displayShow();
}

/**
 * @brief Clean up resources and reset IR manager when exiting mode
 * 
 * Performs cleanup operations including clearing any remaining IR commands,
 * resetting the IR manager to default mode, and clearing the display.
 */
void ModeIRScan::cleanup() {
    Serial.println("IR Scan mode cleanup.");
    if (m_irManager) {
        // Clear any learned commands from IR scan mode
        m_irManager->clearCommand();
        
        // Clear any additional remaining commands
        while (m_irManager->hasNewCommand()) {
            // Consume remaining commands
            m_irManager->getLastCommand();
            m_irManager->clearCommand();
        }
        
        // Reset to default IR reception mode
        #if defined(USE_YAHBOOM_IR_BLOCKING_MODE)
            m_irManager->setReceiveMode(IRManager::IRReceiveMode::YAHBOOM_BLOCKING);
        #else
            m_irManager->setReceiveMode(IRManager::IRReceiveMode::NEC_NON_BLOCKING);
        #endif
    }
    
    // Clear display and provide audio feedback
    m_matrix->fillScreen(0);
    m_utils->displayShow();
    m_utils->playSingleTone();
}

/**
 * @brief Handle real-time keyboard input with immediate character echo
 * 
 * Processes individual characters from serial input, handling backspace,
 * enter key, and printable characters. Provides immediate echo feedback
 * and processes complete lines when enter is pressed.
 */
bool ModeIRScan::handleRealTimeInput() {
    if (!Serial.available()) {
        return false;
    }
    
    char c = Serial.read();
    
    // Handle backspace and delete characters
    if (c == '\b' || c == 127) {
        if (bufferIndex > 0) {
            bufferIndex--;
            inputBuffer[bufferIndex] = '\0';
            // Echo backspace sequence to terminal
            Serial.print("\b \b");
        }
        return false;
    }
    
    static bool lastWasCR = false;

    // Handle enter key (newline or carriage return)
    if (c == '\n' || c == '\r') {
        // Skip LF if previous character was CR (Windows CR+LF handling)
        if (c == '\n' && lastWasCR) {
            lastWasCR = false;
            return false;
        }
        
        lastWasCR = (c == '\r');
        
        if (bufferIndex > 0) {
            // Process completed input
            inputBuffer[bufferIndex] = '\0';
            String input = String(inputBuffer);
            input.trim();
            input.toUpperCase();
            
            bufferIndex = 0;
            inputBuffer[0] = '\0';
            
            processUserInput(input);
            return true;
        } else {
            // Only output newline once
            Serial.println();
            return false;
        }
    }
    lastWasCR = false;
    
    // Handle printable ASCII characters
    // Protect buffer overflow
    if (c >= 32 && c <= 126 && bufferIndex < 63) {
        inputBuffer[bufferIndex] = c;
        bufferIndex++;
        inputBuffer[bufferIndex] = '\0';
        Serial.print(c);
    } else if (bufferIndex >= 63) {
        // Buffer overflow warning
        Serial.println("\nInput too long!");
        bufferIndex = 0;
        inputBuffer[0] = '\0';
    }

    // Clarify the meaning of return values
    if (bufferIndex > 0) {
        // Process completed input
        return true;
    } else {
        // Process empty input (newline only)
        return false; 
    }
}
