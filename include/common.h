/**
 * @file common.h
 * @brief Common definitions and forward declarations for ESP32 Matrix Portal S3
 * 
 * Contains shared type definitions, forward declarations, and global variable
 * extern declarations used across multiple modules in the application.
 */

#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declarations for mode classes - prevents circular dependencies
class ModeClock;
class ModeMqtt;
class ModeCountdown;
class ModePattern;      // Pattern animation display mode
class ModeImage;        // PNG image display mode
class ModeGIF;          // GIF animation display mode
class ModeFont;         // Font preview and testing mode
class ModeSysinfo;      // System information display mode
class ModeIRScan;       // IR remote control scanner mode
class Utils;            // Utility functions and hardware management

// Global system state variables - extern declarations
extern DisplayMode currentMode;        // Currently active display mode
extern char mqtt_message[256];         // MQTT message buffer for processing
extern unsigned long lastMqttActivity; // Timestamp of last MQTT activity

// Network communication objects - extern declarations
extern WiFiClient wifiClient;          // WiFi client for network connectivity
extern PubSubClient mqttClient;        // MQTT client for message handling

// Display mode instances - extern declarations
extern ModeClock modeClock;            // Clock display mode instance
extern ModeMqtt modeMqtt;              // MQTT message display mode instance
extern ModeCountdown modeCountdown;    // Countdown timer mode instance
extern ModePattern modePattern;        // Pattern animation mode instance
extern ModeImage modeImage;            // Image display mode instance
extern ModeGIF modeGif;                // GIF animation mode instance
extern ModeFont modeFont;              // Font preview mode instance
extern ModeSysinfo modeSysinfo;        // System information mode instance
extern ModeIRScan modeIRScan;          // IR scanner mode instance

#endif