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
    selectedMode = YAHBOOM_BLOCKING_MODE; 
    totalLearnedKeys = 0;
    
    // 입력 버퍼 초기화
    bufferIndex = 0;
    inputBuffer[0] = '\0';
    
    // 스캔 상태 초기화
    scanState = ScanState::WAITING_FOR_INPUT;
    scanStartTime = 0;
    irScanInitiated = false;

    // Learned key array initialization
    for (int i = 0; i < 50; i++) {
        keys[i].isValid = false;
        strcpy(keys[i].keyName, "");
        keys[i].irCode = 0;
    }

    Serial.println("IR Scan mode ready.");
}

void ModeIRScan::run() {
    if (isFirstRun) {
        updateDisplay();  // Display initial "READY" state in run()
        showGuide();
        isFirstRun = false;
    }

    // Process READY state: Only serial input is handled, and IR signals controlled by the main are received.
    if (learningState == READY) {
        handleRealTimeInput(); // 실시간 입력 처리로 변경
        updateDisplay();
    } else {
        // Enter SCAN mode only through the SCAN command
        runInternalLoop();
    }
}

void ModeIRScan::runInternalLoop() {
    // EXIT_MODE가 될 때까지만 루프 (READY 체크 제거)
    while (learningState != EXIT_MODE) {
        handleRealTimeInput();
        
        if (learningState == SCAN) {
            handleIRScan();
        }
        
        updateDisplay();
        delay(10);
    }
    
    Serial.println("Exiting IR Scan internal loop...");
}

void ModeIRScan::processUserInput(const String& input) {
    // 입력 처리 전에 줄바꿈 추가하여 다음 출력이 새 줄에서 시작되도록 함
    Serial.println();
    
    // Global commands : can input at any time (in input)
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
            // Ready to receive IR signals in SCAN state
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
        
        // 스캔 상태 초기화
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

void ModeIRScan::startScanMode() {
    learningState = KEY_INPUT;
    updateDisplay();
    showPrompt();
}

void ModeIRScan::handleModeCommand() {
    learningState = MODE_SELECT;
    const char* currentModeStr = (selectedMode == YAHBOOM_BLOCKING_MODE) ? "YAHBOOM-Blocking" : "NEC-Non-blocking";
    Serial.printf("Current mode: %s\n", currentModeStr);
    Serial.println("Select mode: 1=YAHBOOM-Blocking, 2=NEC-Non-blocking");
    Serial.println("---------------------------------------------------------------------");
    showPrompt();
}

void ModeIRScan::handleModeSelection(const String& input) {
    // 에코 제거
    // if (input.length() > 0) {
    //     Serial.println(input);
    // }
    
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

void ModeIRScan::handleDuplicateChoice(const String& input) {
    // 에코 제거
    // if (input.length() > 0) {
    //     Serial.println(input);
    // }
    
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
    // Serial.println("Ready for next key. Enter key name or DONE to finish:");
    showPrompt();
}

void ModeIRScan::handleIRScan() {
    if (!m_irManager) return;
    
    bool codeReceived = false;
    uint32_t receivedCode = 0;
    
    // 상태 기반 스캔 제어
    switch (scanState) {
        case ScanState::WAITING_FOR_INPUT:
            // 스캔 시작
            scanState = ScanState::SCANNING_IR;
            scanStartTime = millis();
            irScanInitiated = false;
            // Fall through to SCANNING_IR
            
        case ScanState::SCANNING_IR:
            if (selectedMode == YAHBOOM_BLOCKING_MODE) {
                if (!irScanInitiated) {
                    // 블로킹 모드: 한 번만 스캔 시작
                    m_irManager->setReceiveMode(IRManager::IRReceiveMode::YAHBOOM_BLOCKING);
                    m_irManager->enableScanMode(true);
                    m_irManager->clearCommand();
                    irScanInitiated = true;
                }
                
                // 타임아웃 체크 (10초)
                if (millis() - scanStartTime > 10000) {
                    scanState = ScanState::WAITING_FOR_INPUT;
                    irScanInitiated = false;
                    learningState = KEY_INPUT;  // 키 입력 대기 상태로 변경
                    Serial.println("IR scan timeout. Ready for next key name.");
                    showPrompt();
                    return;
                }
                
                m_irManager->readRawCodeYahboom(100); // 짧은 타임아웃으로 체크
                
                if (m_irManager->hasNewCommand()) {
                    receivedCode = m_irManager->getLastCommand();
                    codeReceived = true;
                    scanState = ScanState::PROCESSING_RESULT;
                }
            } else {
                // NEC 논블로킹 모드
                if (!irScanInitiated) {
                    m_irManager->setReceiveMode(IRManager::IRReceiveMode::NEC_NON_BLOCKING);
                    m_irManager->enableScanMode(true);
                    irScanInitiated = true;
                }
                
                // 타임아웃 체크 (10초)
                if (millis() - scanStartTime > 10000) {
                    scanState = ScanState::WAITING_FOR_INPUT;
                    irScanInitiated = false;
                    learningState = KEY_INPUT;  // 키 입력 대기 상태로 변경
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
            // 결과 처리 완료 후 다음 입력 대기 상태로 전환
            scanState = ScanState::WAITING_FOR_INPUT;
            irScanInitiated = false;
            break;
    }
    
    if (codeReceived) {
        processReceivedCode(receivedCode);
    }
}

void ModeIRScan::processReceivedCode(uint32_t code) {
    // IR 코드 처리 후 즉시 클리어 (main에서 재처리 방지)
    m_irManager->clearCommand();
    
    // Duplicate check
    if (isDuplicateCode(code)) {
        Serial.printf("Warning: Code 0x%02X already exists!\n", code);
        duplicateIRCode = code;
        learningState = DUPLICATE_CONFIRM;
        updateDisplay();
        Serial.printf("Overwrite? (Y/N): ");
        return;
    }

    // Store key
    if (storeKey(currentKeyName, code)) {
        m_utils->playSingleTone();
    } else {
        Serial.println("Failed to store key!");
        m_utils->playErrorTone();
    }
    
    // SCAN 모드 지속: KEY_INPUT 상태로 변경하여 다음 키 이름 입력 대기
    learningState = KEY_INPUT;
    
    // 스캔 상태 초기화
    scanState = ScanState::WAITING_FOR_INPUT;
    irScanInitiated = false;
    
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

    Serial.printf("Key '%s' stored with code 0x%02X\n", keys[index].keyName, keys[index].irCode);
    return true;
}

bool ModeIRScan::isValidKeyName(const char* keyName) {
    if (!keyName || strlen(keyName) == 0 || strlen(keyName) >= 32) {
        return false;
    }

    // First character must be alphanumeric (alphabet or digit)
    if (!isalnum(keyName[0])) return false;

    // Remaining characters must be alphanumeric or underscore
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
    Serial.println(); // Add a newline for clean formatting before the next prompt.
    showPrompt();
}

void ModeIRScan::clearKeys() {
    // Initialize current session key storage (Keys)
    for (int i = 0; i < 50; i++) {
        keys[i].isValid = false;
        strcpy(keys[i].keyName, "");
        keys[i].irCode = 0;
    }
    totalLearnedKeys = 0;
    
    Serial.println("All learned keys cleared. "); // Use println to ensure a newline.
    m_utils->playSingleTone();
    showPrompt();
}

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
    snprintf(str2, sizeof(str2), "%s", phaseText);

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
        
        // IR 스캔 모드에서 학습된 명령들을 모두 클리어
        m_irManager->clearCommand();
        
        // 혹시 남아있을 수 있는 추가 명령들도 클리어
        while (m_irManager->hasNewCommand()) {
            m_irManager->getLastCommand();  // 명령 읽어서 소비
            m_irManager->clearCommand();     // 추가 클리어
        }
        
        // Reset to default mode
        #if defined(USE_YAHBOOM_IR_BLOCKING_MODE)
            m_irManager->setReceiveMode(IRManager::IRReceiveMode::YAHBOOM_BLOCKING);
        #else
            m_irManager->setReceiveMode(IRManager::IRReceiveMode::NEC_NON_BLOCKING);
        #endif
        
        // 최종 클리어
        // m_irManager->clearCommand();
    }
    m_matrix->fillScreen(0);
    m_utils->displayShow();
    m_utils->playSingleTone();
}

// 실시간 키보드 입력 처리 함수
bool ModeIRScan::handleRealTimeInput() {
    if (!Serial.available()) {
        return false;
    }
    
    char c = Serial.read();
    
    // 백스페이스 처리
    if (c == '\b' || c == 127) { // 백스페이스 또는 DEL
        if (bufferIndex > 0) {
            bufferIndex--;
            inputBuffer[bufferIndex] = '\0';
            Serial.print("\b \b"); // 백스페이스 에코
        }
        return false;
    }
    
    // 엔터 처리
    if (c == '\n' || c == '\r') {
        if (bufferIndex > 0) {
            inputBuffer[bufferIndex] = '\0';
            String input = String(inputBuffer);
            input.trim();
            input.toUpperCase();
            
            // 버퍼 초기화
            bufferIndex = 0;
            inputBuffer[0] = '\0';
            
            // 입력 처리 - 줄바꿈 제거로 프롬프트와 같은 줄에 유지
            processUserInput(input);
            return true;
        } else {
            // 빈 입력인 경우에도 줄바꿈
            Serial.println();
        }
        return false;
    }
    
    // 일반 문자 처리
    if (c >= 32 && c <= 126 && bufferIndex < 63) { // 출력 가능한 ASCII 문자
        inputBuffer[bufferIndex] = c;
        bufferIndex++;
        inputBuffer[bufferIndex] = '\0';
        Serial.print(c); // 즉시 에코
    }
    
    return false;
}

