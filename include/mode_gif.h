#ifndef MODE_GIF_H
#define MODE_GIF_H

#include "config.h"
#include "utils.h"
#include "gif_player.h"

// GIF Content enumeration for direct MQTT access
enum class GifContentType {
    GIF_01_HOMER = 0,       // Built-in GIF (01_homer)
    GIF_02_MOOSE,           // /gifs/02_moose.gif - static image with QR code
    GIF_03_SMOKE,           // /gifs/03_smoke.gif
    GIF_04_VINCENT,         // /gifs/04_vincent.gif
    GIF_05_MONSTER,         // /gifs/05_monster.gif
    GIF_06_BIRD,            // /gifs/06_bird.gif
    GIF_07_WOOSUL,          // /gifs/07_woosulQR.gif
    GIF_CONTENT_COUNT       // Must be last - used for counting contents
};

// Constants for GIF content management
#define MAX_GIF_CONTENTS static_cast<int>(GifContentType::GIF_CONTENT_COUNT)

class ModeGIF {
private:
    Utils* m_utils;
    MatrixPanel_I2S_DMA* m_matrix;
    
    bool autoMode;
    unsigned long lastAutoSwitch;
    unsigned long autoSwitchInterval;
    bool isInitialized;
    
    void displayInfo();
    
public:
    ModeGIF();
    ~ModeGIF();

    // Basic mode structure functions
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr);
    void activate();
    void run();
    void cleanup();
    
    void handleIRCommand(uint32_t command);
    String getModeName() const { return "GIF Player"; }

    void nextGif();
    void prevGif();
    
    // MQTT direct access functions
    void setGifContent(GifContentType contentType);
    GifContentType getCurrentGifContent() const;
    String getGifContentName(GifContentType contentType) const;
};

#endif