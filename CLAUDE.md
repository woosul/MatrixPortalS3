# MatrixPortalS3 Terminal Setup Commands

## Build Commands
```bash
# Build the project for macOS (default environment)
pio run

# Build for Windows environment
pio run -e matrixportal_s3_win

# Clean build (remove compiled files)
pio run --target clean

# Build and upload to device
pio run --target upload

# Build, upload and start serial monitor
pio run --target upload --target monitor
```

## Development Commands
```bash
# Serial monitor (debugging)
pio device monitor --baud 115200

# List connected devices
pio device list

# Check code syntax without building
pio check

# Run static analysis
pio check --flags="--enable=all"

# Format code (if clang-format available)
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

## File System Commands
```bash
# Upload filesystem (data folder to device)
pio run --target uploadfs

# Build filesystem image
pio run --target buildfs

# View filesystem contents
ls -la data/
```

## Library Management
```bash
# Update all libraries
pio lib update

# Install specific library
pio lib install "library_name"

# List installed libraries
pio lib list

# Search for libraries
pio lib search "keyword"
```

## Project Information
```bash
# Show project information
pio project config

# Show board information
pio boards adafruit_matrixportal_esp32s3

# Show available environments
pio project config --json-output | grep -A 1 "env:"
```

## Quick Development Workflow
```bash
# Full development cycle
alias matrix-build="pio run"
alias matrix-upload="pio run --target upload"
alias matrix-monitor="pio device monitor --baud 115200"
alias matrix-full="pio run --target upload --target monitor"
alias matrix-clean="pio run --target clean"
alias matrix-fs="pio run --target uploadfs"

# Check project health
alias matrix-check="pio check && echo 'Code analysis complete'"

# Quick git status for this project
alias matrix-status="git status --porcelain"
```

## Current Project Status
- **Platform**: ESP32-S3 with PSRAM
- **Framework**: Arduino
- **Board**: Adafruit Matrix Portal ESP32-S3
- **Default Environment**: matrixportal_s3_mac
- **Upload Speed**: 460800 baud
- **Monitor Speed**: 115200 baud
- **File System**: LittleFS

## Modified Files (Current)
- Notes.md
- include/ir_manager.h
- include/mode_ir_scan.h
- src/ir_manager.cpp
- src/main.cpp
- src/mode_ir_scan.cpp

## Quick Start
1. `pio run` - Build project
2. `pio run --target upload` - Upload to device
3. `pio device monitor --baud 115200` - View serial output
4. `pio run --target uploadfs` - Upload configuration files

## Debugging Tips
- Use `Serial.printf()` for debugging output
- Check serial monitor for boot messages
- LED panel brightness: controlled by g_panelBrightness
- IR codes: check IR_CODE_MAP in config.h
- MQTT: check g_mqttServer and g_mqttTopic in config