# ESP32-S3 Matrix Portal Display System


[![PlatformIO CI](https://img.shields.io/badge/PlatformIO-CI-orange)](https://platformio.org/)
[![ESP32-S3](https://img.shields.io/badge/ESP32--S3-compatible-green)](https://www.espressif.com/en/products/socs/esp32-s3)
![ESP32-S3 Matrix Portal](https://img.shields.io/badge/ESP32--S3%20Matrix%20Portal-Display-blue)
[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-brightgreen)](src/version.h)

## MQTT Station : Square Center

A comprehensive multi-mode LED matrix display system built for the Adafruit Matrix Portal ESP32-S3, featuring real-time MQTT communication, animated GIF playback, digital clock, countdown timers, and interactive control via IR remote.

## Features

### Display Modes
- **Clock Mode**: Real-time digital clock with NTP synchronization
- **MQTT Mode**: Real-time message display with MQTT communication
- **Countdown Mode**: Interactive countdown timer with physics simulation
- **Pattern Mode**: Dynamic pattern animations and visual effects
- **Image Mode**: PNG image slideshow display
- **GIF Mode**: Animated GIF playback with smooth rendering
- **Font Mode**: Font preview and testing capabilities
- **SysInfo Mode**: System information monitoring dashboard
- **IR Scanner**: Infrared remote control code scanner

### Control Options
- **IR Remote Control**: Full navigation using NEC protocol IR remote
- **Physical Buttons**: On-board UP/DOWN buttons for mode switching
- **MQTT Commands**: Remote control via MQTT messages
- **Auto-timeout**: Intelligent mode switching based on activity

### Technical Highlights
- **64x64 RGB LED Matrix**: High-resolution P2 panel support
- **ESP32-S3 with PSRAM**: Advanced processing power for complex animations
- **LittleFS Storage**: Reliable configuration and asset storage
- **WiFi & MQTT**: Network connectivity with automatic reconnection
- **Real-time Updates**: NTP time synchronization and live data display

## Hardware Requirements

### Primary Components
- **Adafruit Matrix Portal ESP32-S3** ([Product Page](https://www.adafruit.com/product/5778))
- **64x64 RGB LED Matrix Panel** (P2 or P3 spacing recommended)
- **5V Power Supply** (4-6A recommended for full brightness)
- **IR Remote Control** (NEC protocol compatible)

### Optional Components
- MicroSD card for additional storage
- External buttons for manual control
- Temperature/humidity sensors for enhanced display

## Quick Start

### 1. Hardware Setup
```
ESP32-S3 Matrix Portal → 64x64 LED Panel
├── HUB75 Connector → LED Panel Input
├── 5V Power → Panel Power Input
└── IR Receiver → GPIO Pin (configured in config.h)
```

### 2. Software Installation

#### Using PlatformIO (Recommended)
```bash
# Clone the repository
git clone https://github.com/woosul/MatrixPortalS3.git
cd MatrixPortalS3

# Install dependencies and build
pio run

# Upload to device
pio run --target upload

# Upload filesystem data
pio run --target uploadfs
```

#### Using Arduino IDE 
1. Install ESP32 board package
2. Install required libraries (see `platformio.ini`)
3. Configure board settings for ESP32-S3
4. Compile and upload

### 3. Configuration

Edit `data/config.json` for your environment:
```json
{
    "deviceId": "YOUR_DEVICE_ID",
    "deviceNo": "YOUR_DEVICE_NUMBER",
    "wifiSsid": "YOUR_WIFI_SSID",
    "wifiPass": "YOUR_WIFI_PASSWORD",
    "mqttServer": "YOUR_MQTT_BROKER",
    "mqttPort": 1883,
    "mqttTopic": "your/mqtt/topic",
    "panelBrightness": 128,
    "buzzerVolume": 40
}
```

## Usage

### IR Remote Control Codes
| Function | IR Code | Description |
|----------|---------|-------------|
| Mode Switch | `0xFF00FF` | Cycle through display modes |
| Brightness Up | `0xFF40BF` | Increase LED brightness |
| Brightness Down | `0xFF807F` | Decrease LED brightness |
| Volume Up | `0xFF20DF` | Increase buzzer volume |
| Volume Down | `0xFFA05F` | Decrease buzzer volume |

### MQTT Commands
Send JSON messages to your configured MQTT topic:
```json
{
    "mode": "2.0",
    "stage" : "mode",
    "message": "Hello World!",
    "duration": 5000,
    "color": "#FF0000"
}
```

### Display Modes
- **1.0**: Clock Mode - Digital time display
- **2.0**: MQTT Standby - Waiting for messages
- **2.1**: MQTT Message - Active message display
- **3.0**: Countdown Mode - Timer functionality
- **4.0**: Pattern Mode - Visual effects
- **5.0**: Image Mode - Static image display
- **6.0**: GIF Mode - Animated content
- **7.0**: Font Mode - Typography preview
- **8.0**: SysInfo Mode - System monitoring
- **9.0**: IR Scanner - Remote debugging

## Project Structure

```
├── src/                    # Main source code
│   ├── main.cpp           # Application entry point
│   ├── mode_*.cpp         # Display mode implementations
│   ├── utils.cpp          # Utility functions
│   └── ir_manager.cpp     # IR remote handling
├── include/               # Header files
│   ├── config.h           # Hardware and app configuration
│   ├── mode_*.h           # Mode class definitions
│   └── common.h           # Shared definitions
├── data/                  # Filesystem data
│   ├── config.json        # Runtime configuration
│   ├── images/            # PNG image assets
│   └── gifs/              # Animated GIF files
├── fonts/                 # Custom font definitions
├── lib/                   # External libraries
├── tools/                 # Development utilities
└── docs/                  # Documentation assets
```

## Development

### Building Custom Modes
1. Create new mode class inheriting from base mode interface
2. Implement required methods: `enter()`, `exit()`, `loop()`, `handleInput()`
3. Register mode in main.cpp mode switching logic
4. Add mode-specific configurations to config.h

### Adding Custom Fonts
1. Convert TTF fonts using included Python tools
2. Place generated .h files in `fonts/` directory
3. Register fonts in `font_manager.cpp`
4. Reference in display modes as needed

### MQTT Message Format
```json
{
    "mode": "string",          # Target display mode
    "message": "string",       # Text content to display
    "duration": number,        # Display duration in milliseconds
    "color": "string",         # Hex color code (e.g., "#FF0000")
    "animation": "string",     # Animation type (optional)
    "priority": number         # Message priority (optional)
}
```

## Dependencies

### Core Libraries
- **ESP32-HUB75-MatrixPanel-DMA**: LED matrix driver
- **PubSubClient**: MQTT communication
- **ArduinoJson**: JSON parsing and generation
- **AnimatedGIF**: GIF animation support
- **LittleFS**: File system operations

### Development Tools
- **PlatformIO**: Build system and package manager
- **Python 3.x**: Font conversion and utilities
- **Static Analysis**: cppcheck, clang-tidy integration

## Performance

### Memory Usage
- **Flash**: ~1.2MB (application + filesystem)
- **RAM**: ~180KB (with PSRAM for display buffers)
- **PSRAM**: ~512KB (for large image/GIF processing)

### Display Specifications
- **Resolution**: 64x64 pixels (4,096 total pixels)
- **Color Depth**: 6-bit per channel (262,144 colors)
- **Refresh Rate**: 120Hz (flicker-free display)
- **Brightness**: 0-255 levels (adjustable)

## Troubleshooting

### Common Issues

**WiFi Connection Fails**
- Verify SSID and password in config.json
- Check 2.4GHz network compatibility
- Ensure stable power supply

**MQTT Not Connecting**
- Verify broker address and port
- Check network firewall settings
- Validate topic subscription format

**Display Flickering**
- Increase power supply capacity
- Reduce brightness level
- Check HUB75 cable connections

**GIF Playback Issues**
- Verify GIF file format compatibility
- Check available PSRAM
- Reduce GIF file size if needed

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Development Guidelines
- Follow existing code style and formatting
- Add comprehensive comments for new features
- Test thoroughly on actual hardware
- Update documentation as needed

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

**Denny Kim** - *Feature Lead: ESP32 Matrix Portal S3 Team*
- Email: woosul@gmail.com
- GitHub: [@woosul](https://github.com/woosul)

## Acknowledgments

- [Adafruit](https://www.adafruit.com/) for the Matrix Portal ESP32-S3 hardware
- [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA) library contributors
- Open source community for various libraries and tools

## Roadmap

- [ ] Web-based configuration interface
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Advanced animation effects library
- [ ] Multi-panel cascade support
- [ ] Integration with popular IoT platforms
- [ ] Mobile companion application

---

*For detailed technical documentation, see the [docs/](docs/) directory.*

