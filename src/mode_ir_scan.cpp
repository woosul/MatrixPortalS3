#include "mode_ir_scan.h"
#include "common.h"
#include "utils.h"
#include "ir_manager.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

ModeIRScan::ModeIRScan() : m_irManager(nullptr), selectedMode(YAHBOOM_BLOCKING_MODE), 
                           isFirstRun(true), duplicateIRCode(0) {
    totalLearnedKeys = 0;
    strcpy(currentKeyName, "");
}

void ModeIRScan::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr, IRManager* irManager_ptr) {
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;
    m_irManager = irManager_ptr;
    
    learningState = READY;
    selectedMode = YAHBOOM_BLOCKING_MODE;   // 기본값은 블로킹 모드
    totalLearnedKeys = 0;
    
    // 키 배열 초기화
    for (int i = 0; i < 50; i++) {
        keys[i].isValid = false;
        strcpy(keys[i].keyName, "");
        keys[i].irCode = 0;
    }

    Serial.println("IR Scan mode ready.");
}

void ModeIRScan::run() {
    if (isFirstRun) {
        updateDisplay();  // run()에서 초기 READY 상태 표시
        showGuide();
        isFirstRun = false;
    }
    
    // READY 상태에서는 시리얼 입력만 처리하고 main에서 제어되는 IR 신호를 수신할 수 있도록 함
    if (learningState == READY) {
        if (Serial.available()) {
            String input = Serial.readStringUntil('\n');
            input.trim();
            input.toUpperCase();
            processUserInput(input);
        }
        updateDisplay();
    } else {
        // SCAN 명령어를 통해서만 runInternalLoop 진입
        runInternalLoop();
    }
}

void ModeIRScan::runInternalLoop() {
    // READY 상태가 아닐 때만 처리
    if (learningState == EXIT_MODE) {
        return;
    }
    
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toUpperCase();
        processUserInput(input);
    }
    
    if (learningState == SCAN) {
        handleIRScan();
    }
    
    updateDisplay();
}

void ModeIRScan::processUserInput(const String& input) {
    // 전역 명령어 - 언제든지 입력 가능
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
    
    // 상태별 처리
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
            // SCAN 상태에서는 IR 신호만 대기
            Serial.println("Waiting for IR signal... Use DONE to cancel.");
            break;
        default:
            Serial.println("Invalid command!");
            showPrompt();
            break;
    }
}

void ModeIRScan::handleKeyNameInput(const String& input) {
    if (isValidKeyName(input.c_str())) {
        strcpy(currentKeyName, input.c_str());
        learningState = SCAN;
        Serial.printf("Learning '%s' - press IR button now\n", currentKeyName);
        updateDisplay();
    } else {
        Serial.println("Invalid key name!");
        showPrompt();
    }
}

void ModeIRScan::startScanMode() {
    learningState = KEY_INPUT;
    updateDisplay();
    Serial.println("SCAN mode started. Enter key name:");
    showPrompt();
}

void ModeIRScan::handleModeCommand() {
    learningState = MODE_SELECT;
    const char* currentModeStr = (selectedMode == YAHBOOM_BLOCKING_MODE) ? "Blocking" : "Non-blocking";
    Serial.printf("Current mode: %s\n", currentModeStr);
    Serial.println("Select mode: 1=Blocking, 2=Non-blocking");
    Serial.println("-----------------------------------------------------------");
    showPrompt();
}

void ModeIRScan::handleModeSelection(const String& input) {
    if (input == "1") {
        selectedMode = YAHBOOM_BLOCKING_MODE;
        Serial.println("Blocking mode selected");
    } else if (input == "2") {
        selectedMode = NEC_NON_BLOCKING_MODE;
        Serial.println("Non-blocking mode selected");
    } else {
        Serial.println("Invalid selection!");
        showPrompt();
        return;
    }
    
    learningState = READY;
    updateDisplay();
    showPrompt();
}

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

void ModeIRScan::handleIRScan() {
    if (!m_irManager) return;
    
    bool codeReceived = false;
    uint32_t receivedCode = 0;
    
    if (selectedMode == YAHBOOM_BLOCKING_MODE) {
        m_irManager->setReceiveMode(IRManager::IRReceiveMode::YAHBOOM_BLOCKING);
        m_irManager->enableScanMode(true);
        m_irManager->clearCommand();
        m_irManager->readRawCodeYahboom(5000);
        
        if (m_irManager->hasNewCommand()) {
            receivedCode = m_irManager->getLastCommand();
            codeReceived = true;
            Serial.printf("IR code received: 0x%02X\n", receivedCode);
        }
    } else {
        m_irManager->setReceiveMode(IRManager::IRReceiveMode::NEC_NON_BLOCKING);
        m_irManager->enableScanMode(true);
        m_irManager->update();
        
        if (m_irManager->hasNewCommand()) {
            receivedCode = m_irManager->getLastCommand();
            codeReceived = true;
            Serial.printf("IR code (NEC) received: 0x%02X\n", receivedCode);
        }
    }
    
    if (codeReceived) {
        processReceivedCode(receivedCode);
    }
}

void ModeIRScan::processReceivedCode(uint32_t code) {
    // 중복 체크
    if (isDuplicateCode(code)) {
        Serial.printf("Warning: Code 0x%02X already exists!\n", code);
        duplicateIRCode = code;
        learningState = DUPLICATE_CONFIRM;
        updateDisplay();
        Serial.printf("Overwrite? (Y/N): ");
        return;
    }
    
    // 키 저장
    if (storeKey(currentKeyName, code)) {
        Serial.printf("Key '%s' learned! (0x%02X)\n", currentKeyName, code);
        m_utils->playSingleTone();
    } else {
        Serial.println("Failed to store key!");
        m_utils->playErrorTone();
    }
    
    learningState = KEY_INPUT;
    updateDisplay();
    showPrompt();
}

bool ModeIRScan::storeKey(const char* keyName, uint32_t irCode) {
    int index = findKeyIndex(keyName);
    
    if (index < 0) {
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
    return true;
}

bool ModeIRScan::isValidKeyName(const char* keyName) {
    if (!keyName || strlen(keyName) == 0 || strlen(keyName) >= 32) {
        return false;
    }
    
    // 첫 글자는 알파벳
    if (!isalpha(keyName[0])) return false;
    
    // 나머지는 알파벳, 숫자, 언더스코어
    for (int i = 1; keyName[i]; i++) {
        if (!isalnum(keyName[i]) && keyName[i] != '_') {
            return false;
        }
    }
    
    return true;
}

int ModeIRScan::findKeyIndex(const char* keyName) {
    for (int i = 0; i < 50; i++) {
        if (keys[i].isValid && strcasecmp(keys[i].keyName, keyName) == 0) {
            return i;
        }
    }
    return -1;
}

int ModeIRScan::findEmptySlot() {
    for (int i = 0; i < 50; i++) {
        if (!keys[i].isValid) {
            return i;
        }
    }
    return -1;
}

bool ModeIRScan::isDuplicateCode(uint32_t irCode) {
    for (int i = 0; i < 50; i++) {
        if (keys[i].isValid && keys[i].irCode == irCode) {
            return true;
        }
    }
    return false;
}

void ModeIRScan::showGuide() {
    Serial.println("IR REMOTE LEARNING MODE");
    Serial.println("-----------------------------------------------------------");
    Serial.println("Commands:");
    Serial.println(" SCAN     - Start key learning process");
    Serial.println(" SUMMARY  - Show all learned keys");
    Serial.println(" UPDATE   - Generate config.h format");
    Serial.println(" CLEAR    - Clear all learned keys");
    Serial.println(" MODE     - Change IR mode (Blocking/Non-blocking)");
    Serial.println(" DONE     - Exit current operation or mode");
    Serial.println("-----------------------------------------------------------");
    showPrompt();
}

void ModeIRScan::showSummary() {
    Serial.println("=== LEARNED KEYS ===");
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

void ModeIRScan::updateCodes() {
    if (!m_irManager) {
        Serial.println("ERROR: IRManager not set!");
        return;
    }
    
    Serial.println("=== CONFIG.H IR_CODE_MAP FORMAT ===");
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
    
    Serial.printf("Generated %d code definitions\n", totalLearnedKeys);
    if (totalLearnedKeys > 0) {
        m_utils->playSingleTone();
    } else {
        m_utils->playErrorTone();
    }
    showPrompt();
}

void ModeIRScan::clearKeys() {
    // 현재 세션의 키 저장소 초기화
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

void ModeIRScan::showPrompt() {
    const char* modeStr = (selectedMode == YAHBOOM_BLOCKING_MODE) ? "(Blocking)" : "(Non-blocking)";
    
    switch (learningState) {
        case READY:
            Serial.printf("Commands: SCAN | SUMMARY | UPDATE | CLEAR | MODE %s | DONE\n", modeStr);
            Serial.print("Input command : ");
            break;
        case KEY_INPUT:
            Serial.print("IR Key name : ");
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

void ModeIRScan::scanProcessDisplay(const char* phaseText) {
    m_matrix->fillScreen(0);

    const char* str1 = "IR SCAN";
    char str2[32];
    snprintf(str2, sizeof(str2), "[%s]", phaseText);

    int str1_x = m_utils->calculateTextCenterX(str1, MATRIX_WIDTH);
    int str2_x = m_utils->calculateTextCenterX(str2, MATRIX_WIDTH);
    
    m_matrix->setTextColor(m_utils->hexToRgb565(0x808080)); // Gray
    m_utils->setCursorTopBased(str1_x, 4, false);
    m_matrix->print(str1);
    
    m_matrix->setTextColor(m_utils->hexToRgb565(0xFFFFFF)); // White
    m_utils->setCursorTopBased(str2_x, 28, false);
    m_matrix->print(str2);
    
    m_utils->displayShow();
}

void ModeIRScan::cleanup() {
    Serial.println("IR Scan mode cleanup.");
    if (m_irManager) {
        // enableScanMode(false) 호출 제거 - 기존 IR 코드 맵 출력을 방지
        // m_irManager->enableScanMode(false);
        
        // Reset to default mode
        #if defined(USE_YAHBOOM_IR_BLOCKING_MODE)
            m_irManager->setReceiveMode(IRManager::IRReceiveMode::YAHBOOM_BLOCKING);
        #else
            m_irManager->setReceiveMode(IRManager::IRReceiveMode::NEC_NON_BLOCKING);
        #endif
        m_irManager->clearCommand();
    }
    m_matrix->fillScreen(0);
    m_utils->displayShow();
    m_utils->playSingleTone();
}
