#include "ir_manager.h"
#include "config.h" // Include config.h for pin definitions and IR_CODE_MAP
#include "utils.h"  // Include utils.h for Utils class definition

// IRManager setup function
// Initializes the IR receiver pin and stores the pointer to the Utils instance.
void IRManager::setup(Utils* utils_ptr) {
    // Store the Utils pointer for accessing utility functions like buzzer tones.
    utils = utils_ptr;
    if (!utils) { 
        Serial.println("ERROR: Utils pointer not provided to IRManager!"); 
        // If Utils is not available, disable sound feedback to prevent crashes.
        soundFeedbackEnabled = false; 
    }
    
    // Initialize IR receiver pin using the define from config.h.
    currentReceiveMode = IRReceiveMode::NEC_NON_BLOCKING; // Default to NEC non-blocking
    irPin = IR_RECEIVER_PIN; 
    // IR receivers are typically active LOW, so use input with pullup.
    pinMode(irPin, INPUT_PULLUP); 
    
    // Initialize state machine variables for non-blocking reception.
    currentState = IRState::IDLE;
    lastEdgeTime_us = micros(); // Initialize with current time.
    pulseStartTime_us = 0;
    rawData = 0;
    bitCount = 0;
    isRepeat = false;
    
    // Initialize command variables.
    lastCommand = 0;
    lastReceiveTime = 0;
    newCommandAvailable = false;
    scanMode = false;
    // soundFeedbackEnabled는 utils 사용 가능 여부에 따라 위에서 초기화됩니다.
    // utils가 null이면 false로 설정됩니다. utils가 있으면 기본적으로 true로 설정합니다.
    if (utils) {
        soundFeedbackEnabled = true; // utils 사용 가능 시 기본값 true
    }
    
    // Initialize button mappings from config.h.
    // This assumes IR_CODE_MAP in config.h has at least NUM_BUTTONS entries (defined in ir_manager.h).
    for(int i = 0; i < NUM_BUTTONS; ++i) {
        // Copy code, name, and set configured flag to true by default from config.
        buttonMappings[i] = {IR_CODE_MAP[i].code, IR_CODE_MAP[i].name}; 
    }

    Serial.println("IR Manager initialized (Non-blocking State Machine)");
    // Serial.printf("IR Receiver Pin: %d\n", irPin); // Log the actual pin number.
    // Serial.println("Using NEC protocol timing.");
    // Serial.println("Use enableScanMode(true) to scan IR codes");
    
    // Play startup tone through utils, checking if the pointer is valid.
    if (utils) {
        utils->playSingleTone(); 
    } else {
        Serial.println("WARNING: Utils not available, cannot play startup tone!");
    }
    
    // Print the initialized code mappings.
    // ----------------------------------------------------------
    // printCodeMappings();
}

// Set the IR reception mode
void IRManager::setReceiveMode(IRReceiveMode mode) {
    if (currentReceiveMode == mode) return;

    currentReceiveMode = mode;
    Serial.printf("IRManager: Switched to %s mode.\n",
                  (mode == IRReceiveMode::NEC_NON_BLOCKING) ? "NEC (Non-Blocking)" : "RAW (Blocking)");

    // Reset NEC state variables if switching to/from NEC mode
    if (mode == IRReceiveMode::NEC_NON_BLOCKING) {
        currentState = IRState::IDLE;
        lastEdgeTime_us = micros();
        pulseStartTime_us = lastEdgeTime_us;
        rawData = 0;
        bitCount = 0;
        isRepeat = false;
    }
    newCommandAvailable = false; // Clear any pending command
    if (utils && soundFeedbackEnabled) {
        utils->playSingleTone();
    }
}

// Main update function for the non-blocking IR state machine.
// This function should be called frequently in the main loop().
void IRManager::updateReadNonBlocking() {
    if (currentReceiveMode == IRReceiveMode::NEC_NON_BLOCKING) {
        unsigned long currentTime_us = micros();
        bool currentPinState = digitalRead(irPin);
        static bool lastPinState_static = HIGH;

        if (currentState != IRState::IDLE && (currentTime_us - lastEdgeTime_us) > STATE_TIMEOUT_US) {
            Serial.println("IR NEC: State Timeout! Resetting.");
            currentState = IRState::IDLE;
            rawData = 0;
            bitCount = 0;
            isRepeat = false;
            lastPinState_static = HIGH;
        }

        if (currentPinState != lastPinState_static) {
            unsigned long pulseDuration_us = currentTime_us - pulseStartTime_us;
            pulseStartTime_us = currentTime_us;
            lastEdgeTime_us = currentTime_us;
            lastPinState_static = currentPinState;

            switch (currentState) {
                case IRState::IDLE:
                    if (currentPinState == LOW) { currentState = IRState::START_LOW; }
                    break;
                case IRState::START_LOW:
                    if (currentPinState == HIGH) {
                        if (pulseDuration_us >= NEC_START_LOW_MIN && pulseDuration_us <= NEC_START_LOW_MAX) {
                            currentState = IRState::START_HIGH;
                        } else {
                            Serial.printf("IR NEC: Invalid START_LOW (%lu us).\n", pulseDuration_us);
                            currentState = IRState::ERROR;
                        }
                    }
                    break;
                case IRState::START_HIGH:
                    if (currentPinState == LOW) {
                        if (pulseDuration_us >= NEC_START_HIGH_MIN && pulseDuration_us <= NEC_START_HIGH_MAX) {
                            currentState = IRState::DATA_LOW; rawData = 0; bitCount = 0; isRepeat = false;
                        } else if (pulseDuration_us >= NEC_REPEAT_HIGH_MIN && pulseDuration_us <= NEC_REPEAT_HIGH_MAX) {
                            currentState = IRState::REPEAT_GAP; isRepeat = true;
                        } else {
                            Serial.printf("IR NEC: Invalid START_HIGH (%lu us).\n", pulseDuration_us);
                            currentState = IRState::ERROR;
                        }
                    }
                    break;
                case IRState::DATA_LOW:
                    if (currentPinState == HIGH) {
                        if (pulseDuration_us >= NEC_DATA_LOW_MIN && pulseDuration_us <= NEC_DATA_LOW_MAX) {
                            currentState = IRState::DATA_HIGH;
                        } else {
                            Serial.printf("IR NEC: Invalid DATA_LOW (%lu us).\n", pulseDuration_us);
                            currentState = IRState::ERROR;
                        }
                    }
                    break;
                case IRState::DATA_HIGH:
                    if (currentPinState == LOW) {
                        if (pulseDuration_us >= NEC_BIT_HIGH_SHORT_MIN && pulseDuration_us <= NEC_BIT_HIGH_SHORT_MAX) {
                            rawData >>= 1; bitCount++;
                        } else if (pulseDuration_us >= NEC_BIT_HIGH_LONG_MIN && pulseDuration_us <= NEC_BIT_HIGH_LONG_MAX) {
                            rawData >>= 1; rawData |= 0x80000000; bitCount++;
                        } else {
                            Serial.printf("IR NEC: Invalid DATA_HIGH (%lu us).\n", pulseDuration_us);
                            currentState = IRState::ERROR; break;
                        }
                        if (bitCount == 32) { currentState = IRState::DECODING; }
                        else { currentState = IRState::DATA_LOW; }
                    }
                    break;
                case IRState::REPEAT_GAP:
                    if (currentPinState == LOW) {
                        Serial.println("IR NEC: Unexpected LOW during REPEAT_GAP.");
                        currentState = IRState::ERROR;
                    }
                    break;
                case IRState::DECODING: {
                    uint8_t address    = (rawData >> 0)  & 0xFF;
                    uint8_t invAddress = (rawData >> 8)  & 0xFF;
                    uint8_t command    = (rawData >> 16) & 0xFF;
                    uint8_t invCommand = (rawData >> 24) & 0xFF;
                    if ((address + invAddress) == 0xFF && (command + invCommand) == 0xFF) {
                        unsigned long currentTime_ms = millis();
                        if (command != lastCommand || (currentTime_ms - lastReceiveTime) > COMMAND_TIMEOUT) {
                            lastCommand = command;
                            lastReceiveTime = currentTime_ms;
                            newCommandAvailable = true;
                            if (soundFeedbackEnabled && utils) utils->playSingleTone();
                            Serial.printf("IR NEC Command: 0x%02X (Addr: 0x%02X)\n", command, address);
                            if (scanMode) {
                                Serial.printf("IR SCAN (NEC): Code=0x%02X", command);
                                int buttonIndex = findButtonIndexByCode(command);
                                if (buttonIndex >= 0) Serial.printf(" -> %s", buttonMappings[buttonIndex].name);
                                else Serial.print(" -> UNKNOWN");
                                Serial.println();
                            }
                        }
                    } else {
                        Serial.printf("IR NEC: Invalid checksum (A:%02X ~A:%02X C:%02X ~C:%02X)\n", address, invAddress, command, invCommand);
                        if (soundFeedbackEnabled && utils) utils->playErrorTone();
                    }
                    currentState = IRState::IDLE; rawData = 0; bitCount = 0; isRepeat = false;
                    } break;
                case IRState::ERROR:
                    Serial.println("IR NEC: Error state. Resetting.");
                    currentState = IRState::IDLE; rawData = 0; bitCount = 0; isRepeat = false;
                    break;
                default: // Should not happen
                    currentState = IRState::IDLE;
                    break;
            }
        }

        if (currentState == IRState::REPEAT_GAP && (currentTime_us - pulseStartTime_us) > NEC_REPEAT_GAP_MIN) {
            Serial.println("IR NEC: REPEAT_GAP timeout. End of repeat. Resetting.");
            currentState = IRState::IDLE; rawData = 0; bitCount = 0; isRepeat = false;
        }
    } else if (currentReceiveMode == IRReceiveMode::YAHBOOM_BLOCKING) {
        // In YAHBOOM_BLOCKING mode, update() does nothing.
        // The user should call readRawCodeYahboom() explicitly.
        // Or, if desired, readRawCodeYahboom() could be called here if a flag is set,
        // but that would make update() blocking, which might not be intended.
        // For now, keep update() non-blocking for NEC and expect explicit calls for YAHBOOM_BLOCKING.
    }
}

// Enable or disable sound feedback for IR events.
void IRManager::enableSoundFeedback(bool enable) {
    soundFeedbackEnabled = enable;
    Serial.printf("IR sound feedback %s\n", enable ? "enabled" : "disabled");
    
    // Play a test tone if enabling and Utils is available.
    if (enable) {
        if (utils) { utils->playSingleTone(); } 
        else { Serial.println("WARNING: Utils not available for command tone!"); } 
    }
}

// Enable or disable IR scan mode.
// In scan mode, the manager prints received codes for learning purposes.
void IRManager::enableScanMode(bool enable) {
    scanMode = enable;
    if (enable) {
        Serial.println("=== IR SCAN MODE ENABLED ===");
        Serial.println("Press buttons on remote to map codes...");
        Serial.println("Use enableScanMode(false) to exit scan mode");
        
        // Play scan start tone if enabled and Utils is available.
        if (soundFeedbackEnabled) {
            if (utils) { utils->playSingleTone(); } 
            else { Serial.println("WARNING: Utils not available for scan start tone!"); } 
        }
    } else {
        Serial.println("=== IR SCAN MODE DISABLED ===");
        Serial.println("Configured buttons:");
        printCodeMappings(); // Print current mappings when exiting scan mode.
        
        // Play scan end tone if enabled and Utils is available.
        if (soundFeedbackEnabled) { 
            if (utils) {utils->playSingleTone();} 
            else { Serial.println("WARNING: Utils not available for scan end tone!"); } 
        }
    }
}

// Configure a specific button name with a received IR code.
// This is typically used by the IR scan mode (ModeIRScan).
void IRManager::configureCode(const char* buttonName, uint32_t code) {
    // Find the button index by its name.
    int index = findButtonIndex(buttonName);
    if (index >= 0) {
        // Store the command byte of the code.
        buttonMappings[index].code = code & 0xFF; 
        Serial.printf("Configured %s: 0x%02X\n", buttonName, buttonMappings[index].code);
        
        // Play success tone if enabled and Utils is available.
        if (soundFeedbackEnabled) {
            if (utils) { utils->playSingleTone(); } 
            else { Serial.println("WARNING: Utils not available for success tone!"); } 
        }
    } else {
        Serial.printf("Unknown button name: %s\n", buttonName);
        
        // Play error tone if enabled and Utils is available.
        if (soundFeedbackEnabled) {
            if (utils) { utils->playErrorTone(); } 
            else { Serial.println("WARNING: Utils not available for error tone!"); } 
        }
    }
}

// Print the current IR code mappings to the serial console.
void IRManager::printCodeMappings() const { 
    Serial.println("----- IR Code Mappings -----");
    for (int i = 0; i < NUM_BUTTONS; i++) {
        Serial.printf("%-8s: 0x%02X\n", buttonMappings[i].name, buttonMappings[i].code);
    }
    Serial.println("------------------------------");
}

// Reset all button mappings to the default codes defined in config.h.
void IRManager::resetToDefaults() {
    // Re-initialize mappings from config.h.
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttonMappings[i] = {IR_CODE_MAP[i].code, IR_CODE_MAP[i].name}; 
    }
    Serial.println("IR mappings reset to defaults from config.h");
    
    // Play reset tone if enabled and Utils is available.
    if (soundFeedbackEnabled) {
        if (utils) { utils->playSingleTone(); } 
        else { Serial.println("WARNING: Utils not available for reset tone!"); } 
    }
}

// Check if a specific button name corresponds to the last received command.
// This method is less useful with the new async update approach.
// The main loop should check hasNewCommand() and getLastCommand()
// and then compare the command code to the desired button code using findButtonIndexByCode().
bool IRManager::isButtonPressed(const char* buttonName) {
    // Check if a new command is available and find the button index by name.
    if (!newCommandAvailable) return false;
    
    int index = findButtonIndex(buttonName);
    // If the button name is found and configured, compare its code with the last received command.
    if (index >= 0) { // lastCommand는 이미 command 바이트만 저장
        return lastCommand == buttonMappings[index].code;
    }
    return false; // Button not found, not configured, or no new command.
}

// Get the IR code associated with a specific button name.
uint32_t IRManager::getButtonCode(const char* buttonName) {
    // Find the button index by name.
    int index = findButtonIndex(buttonName);
    // Return the code if found, otherwise return an invalid code (0xFF).
    // 0xFFFFFFFF를 사용하여 오류 또는 찾지 못함을 나타내는 것이 더 명확할 수 있습니다.
    if (index >= 0) {
        return buttonMappings[index].code;
    }
    return 0xFFFFFFFF; // 찾지 못했을 때의 반환 값
}

// Check if a new IR command has been successfully decoded since the last clearCommand().
bool IRManager::hasNewCommand() {
    return newCommandAvailable;
}
// Get the last successfully decoded IR command code (command byte only).
uint32_t IRManager::getLastCommand() {
    return lastCommand & 0xFF; // Return only the command byte (last 8 bits).
}

// Clear the new command available flag after the command has been processed by the main loop.
void IRManager::clearCommand() {
    newCommandAvailable = false;
}

// Helper method to find the index of a button mapping by its name.
int IRManager::findButtonIndex(const char* buttonName) const { 
    // Iterate through the button mappings array.
    for (int i = 0; i < NUM_BUTTONS; ++i) {
        // Compare the provided name with the stored button name (case-sensitive).
        if (strcmp(buttonMappings[i].name, buttonName) == 0) {
            return i; // Return the index if found.
        }
    }
    return -1; // Return -1 if not found.
}

// Blocking raw IR code reader, similar to old Utils::readIRCode()
// This function will set newCommandAvailable and lastCommand if a code is successfully read.
void IRManager::readRawCodeYahboom(unsigned long timeout_ms) {
    if (currentReceiveMode != IRReceiveMode::YAHBOOM_BLOCKING) {
        Serial.println("IRManager: readRawCodeYahboom() called but not in YAHBOOM_BLOCKING mode. Call setReceiveMode() first.");
        return;
    }
    newCommandAvailable = false; // Reset before attempting to read

    // Wait for a signal to start or timeout
    unsigned long startTime = millis();
    while (digitalRead(irPin) == HIGH) {
        if (millis() - startTime > timeout_ms) {
            // Serial.println("IR YAHBOOM_BLOCKING: Timeout waiting for signal start.");
            return; // No signal detected within timeout
        }
        delayMicroseconds(RAW_DELAY_MICROSECONDS); // Small delay to yield
    }

    // Signal started (pin is LOW)
    // This part is a direct adaptation of the core logic from the old Utils::readIRCode
    // It's blocking.

    int count = 0;
    // Wait for the initial LOW part of the signal to end (or a short timeout)
    // This is part of the "start bit" or header detection in many protocols
    while (digitalRead(irPin) == LOW && count < (15000 / RAW_DELAY_MICROSECONDS)) { // Max ~15ms for initial low
        count++;
        delayMicroseconds(RAW_DELAY_MICROSECONDS);
    }
    if (count >= (15000 / RAW_DELAY_MICROSECONDS)) {
        //Serial.println("IR : Timeout during initial LOW pulse.");
        //Serial.println("L.");
        return;
    }

    count = 0;
    // Wait for the initial HIGH part of the signal to end (or a short timeout)
    while (digitalRead(irPin) == HIGH && count < (10000 / RAW_DELAY_MICROSECONDS) ) { // Max ~10ms for initial high
        count++;
        delayMicroseconds(RAW_DELAY_MICROSECONDS);
    }
     if (count >= (10000 / RAW_DELAY_MICROSECONDS)) {
        //Serial.println("IR : Timeout during initial HIGH pulse.");
        //Serial.println("H.");
        return;
    }

    uint8_t data[4] = {0, 0, 0, 0};
    int idx = 0;
    int bit_idx_in_byte = 0;

    for (int i = 0; i < 32; i++) { // Attempt to read 32 bits
        count = 0;
        while (digitalRead(irPin) == LOW && count < RAW_MAX_COUNT_SHORT_PULSE) {
            count++; delayMicroseconds(RAW_DELAY_MICROSECONDS);
        }
        if (count == RAW_MAX_COUNT_SHORT_PULSE) { Serial.println("IR : Bit LOW timeout"); return; }

        count = 0;
        while (digitalRead(irPin) == HIGH && count < RAW_MAX_COUNT_LONG_PULSE) {
            count++; delayMicroseconds(RAW_DELAY_MICROSECONDS);
        }
        if (count == RAW_MAX_COUNT_LONG_PULSE) { Serial.println("IR : Bit HIGH timeout"); return; }

        if (count > RAW_BIT_THRESHOLD_COUNT) { data[idx] |= (1 << bit_idx_in_byte); }

        if (++bit_idx_in_byte == 8) { bit_idx_in_byte = 0; idx++; }
    }

    // Basic validation (like NEC checksum, but might not apply to all raw signals)
    // For raw signals, often the "command" part is what's useful.
    // Here, we'll assume data[2] is the command byte, similar to the old code.
    // A more robust raw decoder might return all 4 bytes or a hash.
    uint8_t rawCommand = data[2]; // Assuming the 3rd byte is the command

    // Return rawCommand if it was called form modeIRScan

    // Check if it's a potentially valid command (not all zeros or all ones, for example)
    if (rawCommand != 0x00 && rawCommand != 0xFF) { // Basic check
        lastCommand = rawCommand;
        newCommandAvailable = true;
        lastReceiveTime = millis();
        Serial.printf("IR : Code 0x%02X received.\n", rawCommand);
        if (soundFeedbackEnabled && utils) utils->playSingleTone();
        if (scanMode) {
            Serial.printf("IR SCAN (YAHBOOM_BLOCKING): Code=0x%02X", rawCommand);
            int buttonIndex = findButtonIndexByCode(rawCommand);
            if (buttonIndex >= 0) Serial.printf(" -> %s", buttonMappings[buttonIndex].name);
            else Serial.print(" -> UNKNOWN");
            Serial.println();
        }
    } else {
        Serial.printf("IR : Received potentially invalid data (0x%02X).\n", rawCommand);
        if (soundFeedbackEnabled && utils) utils->playErrorTone();
        // newCommandAvailable remains false
    }
}

// Helper method to find the index of a button mapping by its IR code.
int IRManager::findButtonIndexByCode(uint32_t code) {
    // 코드의 command 바이트 부분만 비교하도록 합니다.
    uint8_t commandByte = code & 0xFF;
    // Iterate through the button mappings array.
    for (int i = 0; i < NUM_BUTTONS; ++i) {
        if (buttonMappings[i].code == commandByte) { // 저장된 버튼 코드와 비교
            return i; // Return the index if found.
        }
    }
    return -1; // Return -1 if not found.
}
