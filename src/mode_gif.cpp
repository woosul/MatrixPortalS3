#include "mode_gif.h"
#include "common.h"

ModeGIF::ModeGIF() {
    m_utils = nullptr;
    m_matrix = nullptr;

    autoMode = false;
    lastAutoSwitch = 0;
    autoSwitchInterval = 10000; // Change to other gif file every 10 seconds
    isInitialized = false;
}

ModeGIF::~ModeGIF() {
    cleanup();
}

/**
 * @brief Set up the GIF mode with required resources
 * 
 * Initializes the GIF player with utility functions and matrix display.
 * Must be called before using any other mode functions.
 * 
 * @param utils_ptr Pointer to utility functions
 * @param matrix_ptr Pointer to the LED matrix display
 */
void ModeGIF::setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr) {
    // Serial.println("Setting up GIF Mode");  // DEBUG
    
    m_utils = utils_ptr;
    m_matrix = matrix_ptr;
    
    if (!m_utils || !m_matrix) {
        // Serial.println("GIF Mode: Invalid parameters");  // DEBUG
        isInitialized = false;
        return;
    }

    // GifPlayer initialization (pass utils and matrix)
    if (!gifPlayer.begin(m_utils, m_matrix)) {
        // Serial.println("Failed to initialize GIF Player");  // DEBUG
        isInitialized = false;
        return;
    }
    
    isInitialized = true;
    // Serial.println("GIF Mode setup complete");  // DEBUG
}

/**
 * @brief Activate GIF mode and start playback
 * 
 * Begins GIF content playback in loop mode. Must be called after setup()
 * and when switching to this mode from other display modes.
 */
void ModeGIF::activate() {
    // Serial.println("Activating GIF Mode");  // DEBUG
    
    if (!isInitialized) {
        Serial.println("GIF Mode not initialized");  // DEBUG
        return;
    }
    
    // Start playing the first content (with fade-in transition)
    gifPlayer.setPlayMode(GIF_LOOP);
    gifPlayer.play();         // Only load the GIF
    // The first frame will naturally overlay the previous screen
    
    // Initialize auto mode
    autoMode = false;
    lastAutoSwitch = millis();
    
    displayInfo();
}

/**
 * @brief Main update loop for GIF mode
 * 
 * Handles GIF playback updates and automatic GIF switching when auto mode is enabled.
 * Should be called continuously in the main loop when GIF mode is active.
 */
void ModeGIF::run() {
    if (!isInitialized) {
        return;
    }

    // GIF playback update
    gifPlayer.update();       // The first frame will be rendered in the back buffer
                            // Naturally displayed with displayShow()

    // Auto mode handling
    if (autoMode) {
        unsigned long now = millis();
        if (now - lastAutoSwitch > autoSwitchInterval) {
            // Serial.println("Auto switching to next GIF");  // DEBUG
            gifPlayer.nextContent();
            lastAutoSwitch = now;
            displayInfo();
        }
    }
}

/**
 * @brief Clean up GIF mode resources
 * 
 * Stops GIF playback, releases resources, and clears the display.
 * Called when switching away from GIF mode or during destruction.
 */
void ModeGIF::cleanup() {
    // Serial.println("Cleaning up GIF Mode");  // DEBUG
    
    if (isInitialized) {
        gifPlayer.end();
        isInitialized = false;
    }

    if (m_utils && m_matrix) {
        // m_matrix->clearScreen();
        m_utils->displayClear();
    }
}

// Not used in this mode, but will be called from main.cpp
void ModeGIF::handleIRCommand(uint32_t command) {
    if (!isInitialized) {
        return;
    }
    
    switch (command) {
        case IR_LEFT:
            // Serial.println("IR: Previous GIF");  // DEBUG
            autoMode = false;
            gifPlayer.prevContent();
            displayInfo();
            break;
            
        case IR_RIGHT:
            // Serial.println("IR: Next GIF");  // DEBUG
            autoMode = false;
            gifPlayer.nextContent();
            displayInfo();
            break;
            
        case IR_UP:
            // Serial.println("IR: Toggle play mode");  // DEBUG
            // Toggle play mode (loop <-> once)
            if (gifPlayer.getPlayMode() == GIF_LOOP) {
                gifPlayer.setPlayMode(GIF_ONCE);
            } else {
                gifPlayer.setPlayMode(GIF_LOOP);
            }
            displayInfo();
            break;
            
        case IR_DOWN:
            // Serial.println("IR: Toggle auto mode");  // DEBUG
            // Toggle auto mode
            autoMode = !autoMode;
            if (autoMode) {
                lastAutoSwitch = millis();
            }
            displayInfo();
            break;
            
        case IR_LIGHT:
            // Serial.println("IR: Play/Pause toggle");  // DEBUG
            // Toggle play/pause
            if (gifPlayer.isGifPlaying()) {
                if (gifPlayer.isGifPaused()) {
                    gifPlayer.resume();
                } else {
                    gifPlayer.pause();
                }
            } else {
                gifPlayer.play();
            }
            displayInfo();
            break;
            
        default:
            // Other IR commands (mode switching, etc.) are handled in main
            break;
    }
}

/**
 * @brief Display current GIF player status information
 * 
 * Outputs the current GIF name, play mode, auto mode status, and playback state
 * for debugging and monitoring purposes.
 */
void ModeGIF::displayInfo() {
    // Display status information via serial
    String info = "GIF: " + gifPlayer.getCurrentContentName();
    
    if (gifPlayer.getPlayMode() == GIF_LOOP) {
        info += " [LOOP]";
    } else {
        info += " [ONCE]";
    }
    
    if (autoMode) {
        info += " [AUTO]";
    } else {
        info += " [MANUAL]";
    }
    
    if (gifPlayer.isGifPaused()) {
        info += " [PAUSED]";
    } else if (gifPlayer.isGifPlaying()) {
        info += " [PLAYING]";
    } else {
        info += " [STOPPED]";
    }
    
    // Serial.println(info);  // DEBUG: Comment out for production
}

/**
 * @brief Switch to the next GIF in the sequence
 * 
 * Advances to the next available GIF content and displays status information.
 * Does nothing if the mode is not properly initialized.
 */
void ModeGIF::nextGif() {
    if (!isInitialized) {
        return;
    }

    gifPlayer.nextContent();
    displayInfo();
}

/**
 * @brief Switch to the previous GIF in the sequence
 * 
 * Goes back to the previous available GIF content and displays status information.
 * Does nothing if the mode is not properly initialized.
 */
void ModeGIF::prevGif() {
    if (!isInitialized) {
        return;
    }

    gifPlayer.prevContent();
    displayInfo();
}

/**
 * @brief Set specific GIF content for MQTT control
 * 
 * Switches to the specified GIF content type and starts playback.
 * Ignores request if the same content is already playing.
 * 
 * @param contentType The GIF content type to switch to
 */
void ModeGIF::setGifContent(GifContentType contentType) {
    // Serial.printf("DEBUG: setGifContent() called with type: %d\n", (int)contentType);

    // Ignore if the currently playing content is the same
    GifContentType currentContent = getCurrentGifContent();
    if (currentContent == contentType && gifPlayer.isGifPlaying()) {
        // Serial.printf("DEBUG: Same content (%s) already playing - no change needed\n", 
        //              getGifContentName(contentType).c_str());
        return;
    }

    // Stop GIF playback before changing content
    gifPlayer.stop();

    // Set content
    switch(contentType) {
        case GifContentType::GIF_01_HOMER:
            gifPlayer.setContentByType(GIF_BUILTIN, 0);
            break;
        case GifContentType::GIF_02_MOOSE:
            gifPlayer.setContentByType(GIF_FILE, 0);
            break;
        case GifContentType::GIF_03_SMOKE:
            gifPlayer.setContentByType(GIF_FILE, 1);
            break;
        case GifContentType::GIF_04_VINCENT:
            gifPlayer.setContentByType(GIF_FILE, 2);
            break;
        case GifContentType::GIF_05_MONSTER:
            gifPlayer.setContentByType(GIF_FILE, 3);
            break;
        case GifContentType::GIF_06_BIRD:
            gifPlayer.setContentByType(GIF_FILE, 4);
            break;
        case GifContentType::GIF_07_WOOSUL:
            gifPlayer.setContentByType(GIF_FILE, 5);
            break;
            default:
            // Serial.printf("Unknown GIF content type: %d\n", (int)contentType);  // DEBUG
            return;
    }

    // Start playing the GIF content (with fade-in transition)
    if (gifPlayer.play()) {
        // Serial.printf("GIF content switched to: %s\n", getGifContentName(contentType).c_str());  // DEBUG
    } else {
        // Serial.printf("Failed to start GIF content: %s\n", getGifContentName(contentType).c_str());  // DEBUG
    }
}

/**
 * @brief Get the currently active GIF content type
 * 
 * Determines which GIF content is currently being displayed based on the
 * source type (built-in vs file) and file index for file-based GIFs.
 * 
 * @return GifContentType The current GIF content type
 */
GifContentType ModeGIF::getCurrentGifContent() const {
    if (!isInitialized) {
        return GifContentType::GIF_01_HOMER;
    }
    
    if (gifPlayer.getCurrentSource() == GIF_BUILTIN) {
        return GifContentType::GIF_01_HOMER;
    } else {
        // File GIF
        int fileIndex = gifPlayer.getCurrentFileIndex();
        switch (fileIndex) {
            case 0: return GifContentType::GIF_02_MOOSE;
            case 1: return GifContentType::GIF_03_SMOKE;
            case 2: return GifContentType::GIF_04_VINCENT;
            case 3: return GifContentType::GIF_05_MONSTER;
            case 4: return GifContentType::GIF_06_BIRD;
            case 5: return GifContentType::GIF_07_WOOSUL;
            default: return GifContentType::GIF_02_MOOSE; // Default to first content if index is invalid
        }
    }
}

/**
 * @brief Get the human-readable name for a GIF content type
 * 
 * Converts the GIF content type enum to a readable string name
 * that matches the actual GIF file names or content identifiers.
 * 
 * @param contentType The GIF content type to get the name for
 * @return String The human-readable name of the GIF content
 */
String ModeGIF::getGifContentName(GifContentType contentType) const {
    switch (contentType) {
        case GifContentType::GIF_01_HOMER: return "homer";
        case GifContentType::GIF_02_MOOSE: return "moose";
        case GifContentType::GIF_03_SMOKE: return "smoke";
        case GifContentType::GIF_04_VINCENT: return "vincent";
        case GifContentType::GIF_05_MONSTER: return "monster";
        case GifContentType::GIF_06_BIRD: return "bird";
        case GifContentType::GIF_07_WOOSUL: return "woosul";    
        default: return "unknown";
    }
}

