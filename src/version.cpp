#include "version.h"

// Get firmware version string (e.g., "1.0.2")
String Version::getFirmwareVersion() {
    return String(FW_VERSION_MAJOR) + "." + 
           String(FW_VERSION_MINOR) + "." + 
           String(FW_VERSION_PATCH);
}

// Get short firmware version for display (e.g., "v1.0.2")
String Version::getFirmwareVersionShort() {
    return "v" + String(FW_VERSION_MAJOR) + "." + 
           String(FW_VERSION_MINOR) + "." + 
           String(FW_VERSION_PATCH);
}

// Get detailed build information as a multi-line string
String Version::getBuildInfo() {
    String buildInfo = "";
    buildInfo += "Firmware: " + getFirmwareVersion() + " (Build: " + String(FW_VERSION_BUILD) + ")\n";
    buildInfo += "Device: " + String(DEVICE_TYPE) + "\n";
    buildInfo += "Hardware: " + String(HARDWARE_VERSION) + "\n";
    buildInfo += "Git: " + String(FW_GIT_HASH) + " (" + String(FW_GIT_BRANCH) + ")\n";
    buildInfo += "ESP32 Core: " + String(ESP.getSdkVersion());
    return buildInfo;
}

// Get build date from the FW_VERSION_BUILD macro
String Version::getBuildDate() {
    // FW_VERSION_BUILD is "YYYYMMDD-HHMMSS"
    return String(FW_VERSION_BUILD).substring(0, 8);
}

// Get build time from the FW_VERSION_BUILD macro
String Version::getBuildTime() {
    String buildStr = FW_VERSION_BUILD;
    int hyphenPos = buildStr.indexOf('-');
    if (hyphenPos != -1) {
        return buildStr.substring(hyphenPos + 1);
    }
    return "unknown";
}

// Get device information as a multi-line string
String Version::getDeviceInfo() {
    String deviceInfo = "";
    deviceInfo += String(DEVICE_TYPE) + "\n";
    deviceInfo += "HW: " + String(HARDWARE_VERSION) + "\n";
    deviceInfo += "FW: " + getFirmwareVersionShort() + "\n"; // Uses "vM.m.p"
    deviceInfo += "ESP32-S3 " + String(ESP.getCpuFreqMHz()) + "MHz";
    return deviceInfo;
}

// Get Git information
String Version::getGitInfo() {
    String gitInfo = "";
    gitInfo += "Branch: " + String(FW_GIT_BRANCH) + "\n";
    gitInfo += "Commit: " + String(FW_GIT_HASH);
    return gitInfo;
}