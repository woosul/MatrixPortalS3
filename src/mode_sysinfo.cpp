#include "mode_sysinfo.h"
#include "common.h"
#include "utils.h"
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <WiFi.h>
#include "version.h"

void ModeSysinfo::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) {
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;
    currentInfoMode = INFO_SYSINFO; // Start with the first info screen
    lastUpdate = 0;
    Serial.println("System info mode setup complete");
}

void ModeSysinfo::run() {
    unsigned long currentTime = millis();
    
    // Update every 1 second
    if (currentTime - lastUpdate >= 1000) {
        displayCurrentInfo();
        lastUpdate = currentTime;
    }
}

void ModeSysinfo::cleanup() {
    Serial.println("System info mode cleanup");
    m_matrix->fillScreen(0);
    m_utils->displayShow();
}

void ModeSysinfo::nextInfo() {
    currentInfoMode = (currentInfoMode + 1) % MAX_INFO_MODES;
    displayCurrentInfo();
}

void ModeSysinfo::prevInfo() {
    currentInfoMode = (currentInfoMode - 1 + MAX_INFO_MODES) % MAX_INFO_MODES;
    displayCurrentInfo();
}

void ModeSysinfo::setInfoMode(int mode) {
    if (mode >= 0 && mode < MAX_INFO_MODES) {
        currentInfoMode = mode;
        displayCurrentInfo();
    }
}

void ModeSysinfo::displayCurrentInfo() {
    m_matrix->fillScreen(0);
    
    switch (currentInfoMode) {
        case INFO_SYSINFO:
            displaySysInfo();
            break;
        case INFO_NETWORK:
            displayWiFiInfo();
            break;
        case INFO_MEMORY:
            displayMemoryInfo();
            break;
    }
    
    m_utils->displayShow();
}

void ModeSysinfo::displaySysInfo() {
    // 상단 타이틀: cyan, center-align, y=4
    m_matrix->setFont();
    m_matrix->setTextSize(1);
    m_matrix->setTextColor(m_utils->hexToRgb565(0x00FFFF)); // Cyan
    
    const char* title = "SYSINFO";
    int title_x = m_utils->calculateTextCenterX(title, MATRIX_WIDTH);
    m_utils->setCursorTopBased(title_x, 4, false);
    m_matrix->print(title);
    
    // 본문: white, left-align-2px, start_y=20, 줄간격 12px
    m_matrix->setTextColor(m_utils->hexToRgb565(0xFFFFFF)); // White
    
    // Device No (N)
    m_utils->setCursorTopBased(2, 20, false);
    m_matrix->printf("N:%s", g_deviceNo);
    
    // Device ID (D) - 길이 제한
    m_utils->setCursorTopBased(2, 32, false);
    String deviceId = String(g_deviceId);
    if (deviceId.length() > 10) {
        deviceId = deviceId.substring(0, 10);
    }
    m_matrix->printf("D:%s", deviceId.c_str());
    
        // Firmware Version (F)
    m_utils->setCursorTopBased(2, 44, false);
    String fwVersion = Version::getFirmwareVersionShort();
    if (fwVersion.length() > 8) {
        fwVersion = fwVersion.substring(0, 8); // 길이 제한
    }
    m_matrix->printf("F:%s", fwVersion.c_str());
}

void ModeSysinfo::displayWiFiInfo() {
    // 상단 타이틀: cyan, center-align, y=4
    m_matrix->setFont();
    m_matrix->setTextSize(1);
    m_matrix->setTextColor(m_utils->hexToRgb565(0x00FFFF)); // Cyan
    
    const char* title = "NETWORK";
    int title_x = m_utils->calculateTextCenterX(title, MATRIX_WIDTH);
    m_utils->setCursorTopBased(title_x, 4, false);
    m_matrix->print(title);
    
    // 본문: white, left-align-2px, start_y=20, 줄간격 12px
    m_matrix->setTextColor(m_utils->hexToRgb565(0xFFFFFF)); // White
    
    if (WiFi.status() == WL_CONNECTED) {
        // SSID (S) - 길이 제한
        m_utils->setCursorTopBased(2, 20, false);
        String ssid = WiFi.SSID();
        if (ssid.length() > 10) {
            ssid = ssid.substring(0, 10);
        }
        m_matrix->printf("S:%s", ssid.c_str());
        
        // RSSI (R)
        m_utils->setCursorTopBased(2, 32, false);
        m_matrix->printf("R:%d", WiFi.RSSI());
        
        // IP (P) - C/D 클래스만
        m_utils->setCursorTopBased(2, 44, false);
        IPAddress ip = WiFi.localIP();
        m_matrix->printf("P:%d.%d", ip[2], ip[3]);
    } else {
        m_utils->setCursorTopBased(2, 20, false);
        m_matrix->print("S:No WiFi");
        m_utils->setCursorTopBased(2, 32, false);
        m_matrix->print("R:---");
        m_utils->setCursorTopBased(2, 44, false);
        m_matrix->print("P:---.---");
    }
}

void ModeSysinfo::displayMemoryInfo() {
    // 상단 타이틀: cyan, center-align, y=4
    m_matrix->setFont();
    m_matrix->setTextSize(1);
    m_matrix->setTextColor(m_utils->hexToRgb565(0x00FFFF)); // Cyan
    
    const char* title = "MEMORY";
    int title_x = m_utils->calculateTextCenterX(title, MATRIX_WIDTH);
    m_utils->setCursorTopBased(title_x, 4, false);
    m_matrix->print(title);
    
    // 본문: white, left-align-2px, start_y=20, 줄간격 12px
    m_matrix->setTextColor(m_utils->hexToRgb565(0xFFFFFF)); // White
    
    // Free (F) - KB 단위
    m_utils->setCursorTopBased(2, 20, false);
    m_matrix->printf("F:%dK", ESP.getFreeHeap() / 1024);
    
    // Allocated (A) - KB 단위
    m_utils->setCursorTopBased(2, 32, false);
    uint32_t allocated = ESP.getHeapSize() - ESP.getFreeHeap();
    m_matrix->printf("A:%dK", allocated / 1024);
    
    // Total (T) - KB 단위
    m_utils->setCursorTopBased(2, 44, false);
    m_matrix->printf("T:%dK", ESP.getHeapSize() / 1024);
}