#include "mode_mqtt.h"
#include "mode_pattern.h"
#include "config.h"
#include "common.h"
#include "font_manager.h"
#include "utils.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h> // For MatrixPanel_I2S_DMA
#include <ArduinoJson.h>

void ModeMqtt::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) {
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;

    isStandbyScreenActive = false;
    isInitialDisplayDone = false;
    isRequestDirectMessageDisplay = false;
    
    currentStageForDisplay[0] = '\0';
    currentStageMarkForDisplay = ' ';
    currentCodeForDisplay[0] = '\0';
    currentMessageForDisplay[0] = '\0';
    currentBgColorForDisplay = 0;
    Serial.println("MQTT mode setup complete.");
}

void ModeMqtt::run() {
    unsigned long currentTime = millis();
    if (!m_matrix || !m_utils) return;

    // MQTT-specific idle timeout check.
    // If IDLE_TIMEOUT_MQTT has passed since the last MQTT activity,
    // stop any scrolling and switch to standbyMqtt screen.    
    if (currentTime - lastMqttActivity > g_idleTimeoutMqtt) {
        if (!isStandbyScreenActive) { 
        if (m_utils->isTextScrolling()) { // Changed from isScrollingActive
            m_utils->stopTextScroll(); // Changed from stopScrolling
            }
        m_utils->playSingleTone(); // Play a single tone to indicate timeout
        standbyMqtt(); // Switch to standby screen
        return;        // Return after handling timeout to prevent further drawing in this cycle
        }
    }

    // If not timed out to standby, handle active display (scrolling or static).
    // This block will not execute if standbyMqtt() was called and returned.
    if (m_utils->isTextScrolling()) { // Changed from isScrollingActive
        // Clear title area (full width, Y < 16)
        m_matrix->fillRect(0, 0, MATRIX_WIDTH, 16, 0);

        m_utils->updateTextScroll(); // Changed from handleScrolling

        // After handleScrolling updates the scroll area (Y=16 onwards) in the back buffer,
        // redraw the static title area (Y=0 to Y=15) on top of it in the same back buffer.
        // Title area was already cleared above, so just draw the title text.
        if (currentStageForDisplay[0] != '\0') { // Check if there's a stage to display
            m_matrix->setFont(); 
            m_matrix->setTextSize(1);
            m_matrix->setTextColor(m_utils->hexToRgb565(0xFFFFFF));
            int stage_x = m_utils->calculateTextCenterX(currentStageForDisplay, MATRIX_WIDTH);
            m_utils->setCursorTopBased(stage_x, 4, false);
            m_matrix->print(currentStageForDisplay); // Use stored stage name
        }

        // Explicitly clear left and right margins of the scroll area to prevent artifacts
        // like the 'S' trace, which might occur due to font glyphs extending
        // outside their calculated bounds (e.g., negative xOffset).
        int scroll_text_logical_start_x = m_utils->getScrollTextX();
        int scroll_text_y = m_utils->getScrollTextY();
        int scroll_display_height = m_utils->getDisplayableScrollHeight();
        int scroll_max_width = m_utils->getScrollTextMaxWidth(); // Logical width

        // Clear left margin: from physical X=0 up to scroll_text_physical_start_x
        if (scroll_text_logical_start_x > 0) {
            m_matrix->fillRect(0, scroll_text_y,
                               scroll_text_logical_start_x, scroll_display_height,
                               0); // Black
        }

        // Clear right margin: from (scroll_text_physical_start_x + scroll_max_width) to MATRIX_WIDTH
        int scroll_text_physical_end_x = scroll_text_logical_start_x + scroll_max_width;
        if (scroll_text_physical_end_x < MATRIX_WIDTH) {
            m_matrix->fillRect(setPhysicalX(scroll_text_physical_end_x), scroll_text_y,
                               MATRIX_WIDTH - scroll_text_physical_end_x, scroll_display_height,
                               0); // Black
        }

        m_utils->displayShow(); // Now show the combined frame (title + scrolled text)

    } else { // Not scrolling.
        // This could mean:
        // 1. A static message is being displayed (isStandbyScreenActive = false, isInitialDisplayDone = true).
        // 2. Standby screen is active (isStandbyScreenActive = true).
        // 3. Mode just started, nothing displayed yet (isInitialDisplayDone = false), and no direct display request.
        if (!isStandbyScreenActive && !isInitialDisplayDone && !isRequestDirectMessageDisplay) {
            // This case implies we entered MQTT mode but no message was immediately displayed
            // and we haven't shown standby yet, and no new message is pending display.
            m_utils->playSingleTone(); // Play a single tone to enter standby state
            standbyMqtt();
        }
    }
}


void ModeMqtt::cleanup() {
    Serial.println("MQTT mode cleanup");
    if (m_utils) {
        m_utils->stopTextScroll(); // Changed from stopScrolling
    }
    if (m_matrix) {
        m_matrix->fillScreen(0);
    }
    isStandbyScreenActive = false; 
    isInitialDisplayDone = false;  
    isRequestDirectMessageDisplay = false; 
    // Clear stored display parameters
    currentStageForDisplay[0] = '\0';
    currentStageMarkForDisplay = ' ';
    currentCodeForDisplay[0] = '\0';
    currentMessageForDisplay[0] = '\0';
    currentBgColorForDisplay = 0;
    if (m_utils) m_utils->displayShow();
}

void ModeMqtt::standbyMqtt() {
    if (m_matrix && m_utils) { 
        // Only draw standby if it's not already active.
        // isRequestDirectMessageDisplay is set by main.cpp when a new message arrives.
        // If it's true, it means a message display is pending, so standby should not override it
        // unless this call is from the timeout logic itself.
        if (!isStandbyScreenActive) { // Simplified: only enter if not already in standby.
            m_matrix->fillScreen(0); // Clear entire screen for standby
            m_matrix->setTextSize(1);
            m_matrix->setFont();
            m_matrix->setTextColor(m_utils->hexToRgb565(0x808080));
            char modeTitle[32] = "STANDBY";
            int x1 = m_utils->calculateTextCenterX(modeTitle, MATRIX_WIDTH);
            m_utils->setCursorTopBased(x1, 4, false);
            m_matrix->print(modeTitle);
            m_matrix->setTextWrap(false);
            
            m_matrix->setTextColor(m_utils->hexToRgb565(0xFFFFFF));
            m_matrix->setTextSize(1);
            m_matrix->setFont(FontManager::getFont(FontType::FONT_ROBOTO_BK_24)); 
            int x2 = m_utils->calculateTextCenterX(g_deviceNo, MATRIX_WIDTH); 
            m_utils->setCursorTopBased(x2, 28, true);
            m_matrix->print(g_deviceNo); 
            
            m_utils->displayShow();
            isStandbyScreenActive = true; 
            isInitialDisplayDone = true; 
            isRequestDirectMessageDisplay = false; // Standby is now active, clear any direct display request.

            // Clear stored message details when going to standby
            currentStageForDisplay[0] = '\0';
            currentStageMarkForDisplay = ' ';
            currentCodeForDisplay[0] = '\0';
            currentMessageForDisplay[0] = '\0';
            currentBgColorForDisplay = 0;
        }
    }
}

void ModeMqtt::displayMqttMessage(const char* stage_str, const char* code_str, const char* message_str) {
    if (!m_matrix || !m_utils) {
        Serial.println("displayMqttMessage: m_matrix or m_utils is null!");
        return;
    }

    if (!stage_str) { 
        Serial.println("displayMqttMessage: stage_str is null.");
        return;
    }
    
    m_utils->stopTextScroll(); // Changed from stopScrolling
    // isRequestDirectMessageDisplay is set by the caller (main.cpp) if this is a new message.
    // It will be cleared at the end of this function.

    m_matrix->fillScreen(0); // Clear entire screen before displaying new message

    char stageMark = getStageMarkChar(stage_str);
    // Store current message details for potential redraw during scrolling
    strncpy(currentStageForDisplay, stage_str, sizeof(currentStageForDisplay) - 1);
    currentStageForDisplay[sizeof(currentStageForDisplay)-1] = '\0';
    for (int i = 0; currentStageForDisplay[i]; i++) { // Convert to uppercase for display consistency
        currentStageForDisplay[i] = toupper(currentStageForDisplay[i]);
    }
    currentStageMarkForDisplay = stageMark;

    if (code_str) {
         strncpy(currentCodeForDisplay, code_str, sizeof(currentCodeForDisplay) - 1);
         currentCodeForDisplay[sizeof(currentCodeForDisplay)-1] = '\0';
    } else {
        currentCodeForDisplay[0] = '\0';
    }
    if (message_str) {
        strncpy(currentMessageForDisplay, message_str, sizeof(currentMessageForDisplay) - 1);
        currentMessageForDisplay[sizeof(currentMessageForDisplay)-1] = '\0';
    } else {
        currentMessageForDisplay[0] = '\0';
    }

    Serial.printf("Display MQTT message (from parsed): Stage: '%s'('%c'), Code: '%s'\n", currentStageForDisplay, currentStageMarkForDisplay, currentCodeForDisplay[0] != '\0' ? currentCodeForDisplay : "N/A");
    
    uint16_t backgroundColor = 0x0000; 
    bool shouldBlink = false;
    
    if (stageMark == 'G') {
        backgroundColor = m_utils->hexToRgb565(0x006000); 
    } else if (stageMark == 'E') {
        backgroundColor = m_utils->hexToRgb565(0xBC0000); 
        shouldBlink = true;
    } else if (stageMark == 'W') {
        backgroundColor = m_utils->hexToRgb565(0xFFC000); 
    } else if (stageMark == 'M') {
        backgroundColor = m_utils->hexToRgb565(0x000000); 
    } else if (stageMark == 'X') {
        backgroundColor = m_utils->hexToRgb565(0x545454); 
    }
    currentBgColorForDisplay = backgroundColor;
    
    const char* msgToDraw = currentMessageForDisplay[0] != '\0' ? currentMessageForDisplay : "";

    if (shouldBlink) {
        for (int blink = 0; blink < 3; blink++) {
            displayMessageWithBackground(currentStageForDisplay, currentStageMarkForDisplay, currentCodeForDisplay, msgToDraw, currentBgColorForDisplay, true); 
            m_utils->playBuzzer(1600, 500);
            delay(250);
            displayMessageWithBackground(currentStageForDisplay, currentStageMarkForDisplay, currentCodeForDisplay, msgToDraw, 0x0000, true); 
            delay(750);
        }
        displayMessageWithBackground(currentStageForDisplay, currentStageMarkForDisplay, currentCodeForDisplay, msgToDraw, currentBgColorForDisplay, false); 
    } else {
        displayMessageWithBackground(currentStageForDisplay, currentStageMarkForDisplay, currentCodeForDisplay, msgToDraw, currentBgColorForDisplay, false);
    }
    isStandbyScreenActive = false; // A message is being displayed, so not in standby.
    isRequestDirectMessageDisplay = false; // The request to display this message has been handled/initiated.
                                           // Subsequent idle checks will rely on m_utils->isScrollingActive() or timeout.
    isInitialDisplayDone = true; 
}


// Helper function to display message with background
void ModeMqtt::displayMessageWithBackground(const char* stage, char stageMark, const char* code, const char* message,
                                            uint16_t backgroundColor, bool invertText) {
    if (!m_matrix || !m_utils) {
        Serial.println("displayMessageWithBackground: m_matrix or m_utils is null!");
        return;
    }
    
    // If not scrolling (i.e., initial display or non-M stage), clear the relevant areas.
    // If scrolling an 'M' message, `handleScrolling` clears the scroll area, and `run` clears/redraws the title.
    // displayMqttMessage already calls fillScreen(0) before this.
    if (!m_utils->isTextScrolling()) { // Changed from isScrollingActive
        // m_matrix->fillScreen(0x0000); // Already cleared by displayMqttMessage
    } else if (stageMark != 'M') {
        // If scrolling is active but it's NOT an 'M' message (shouldn't happen often, but defensively)
        m_matrix->fillScreen(0x0000);
    }
    // For scrolling 'M' messages, specific area clearing is handled by run() (title) and handleScrolling() (scroll area)

    // Draw background for message area (Y=16 onwards)
    if (backgroundColor != 0x0000) { // Only draw background if it's not black
        m_matrix->fillRect(0, 16, MATRIX_WIDTH, MATRIX_HEIGHT - 16, backgroundColor);
    }
    
    // Display Stage Title (common for all stageMarks)
    if (stage && stage[0] != '\0') {
        m_matrix->setFont(); 
        m_matrix->setTextSize(1);
        m_matrix->setTextColor(m_utils->hexToRgb565(0xFFFFFF)); 
        int stage_x = m_utils->calculateTextCenterX(stage, MATRIX_WIDTH);
        m_utils->setCursorTopBased(stage_x, 4, false);
        m_matrix->print(stage); // Assumes 'stage' is already uppercase from displayMqttMessage
    }

    if (stageMark == 'M') {
        m_matrix->setTextSize(1); 
        uint16_t messageColor = m_utils->hexToRgb565(0x029994);
        int startX = 1; 
        int startY = 16; 
        int maxWidth = MATRIX_WIDTH - (2 * startX); 
        int charSpacing = 0;
        int lineSpacing = 1; 
        bool isCustomFont = false; 
        int displayAreaHeightForScroll = MATRIX_HEIGHT - startY; 

        char scrollMessage[256]; 
        if (message && message[0] != '\0') {
            strncpy(scrollMessage, message, sizeof(scrollMessage) - 1);
            scrollMessage[sizeof(scrollMessage) - 1] = '\0';
            for (int i = 0; scrollMessage[i]; i++) {
                scrollMessage[i] = toupper(scrollMessage[i]);
            }
        } else {
            scrollMessage[0] = '\0'; 
        }
        
        // If scrolling is active, drawWrappedText will use its internal state.
        // If not scrolling, it will draw the text and call displayShow.
        m_utils->drawWrappedTextAndScroll(scrollMessage, startX, startY, maxWidth, messageColor, ALIGN_CENTER, charSpacing, lineSpacing, isCustomFont, displayAreaHeightForScroll); // Changed: Utils::ALIGN_CENTER -> ALIGN_CENTER
        // If drawWrappedText activated scrolling, its first frame was shown.
        // If it didn't activate scrolling, it also showed the frame.

    } else { // For G, E, W, X stages
        m_matrix->setFont(FontManager::getFont(FontType::FONT_ROBOTO_BK_32));
        m_matrix->setTextSize(1);
        
        uint16_t markTextColor;
        if (backgroundColor == 0x0000) {
             markTextColor = m_utils->hexToRgb565(0xFFFFFF); 
        } else {
             markTextColor = m_utils->hexToRgb565(0x000000); 
        }
        if (invertText) { 
            markTextColor = (markTextColor == m_utils->hexToRgb565(0xFFFFFF)) ? m_utils->hexToRgb565(0x000000) : m_utils->hexToRgb565(0xFFFFFF);
        }
        m_matrix->setTextColor(markTextColor);
        
        char markStr[2] = {stageMark, '\0'};
        int markX = m_utils->calculateTextCenterX(markStr, MATRIX_WIDTH);
        m_utils->setCursorTopBased(markX, 22, true);
        m_matrix->print(stageMark);
        
        if (code && code[0] != '\0') {
            m_matrix->setFont(); 
            m_matrix->setTextSize(1);
            m_matrix->setTextColor(markTextColor); 
            int codeX = m_utils->calculateTextCenterX(code, MATRIX_WIDTH);
            m_utils->setCursorTopBased(codeX, 54, false);
            m_matrix->print(code);
        }
        m_matrix->setTextWrap(false);
        // For non-M stages, displayShow is called here as they don't scroll.
        m_utils->displayShow(); 
    }
}

// Return mark character directly from stage string
char ModeMqtt::getStageMarkChar(const char* stage) {
    if (!stage) return 'X';
    
    String stageStr = String(stage);
    stageStr.toLowerCase();
    
    for (size_t i = 0; i < STAGE_MARK_MAP_SIZE; i++) {
        if (stageStr.equals(STAGE_MARK_MAP[i].keyword)) {
            return STAGE_MARK_MAP[i].mark;
        }
    }
    for (size_t i = 0; i < STAGE_MARK_MAP_SIZE; i++) {
        if (stageStr.indexOf(STAGE_MARK_MAP[i].keyword) >= 0) {
            return STAGE_MARK_MAP[i].mark;
        }
    }
    
    return 'X'; 
}
