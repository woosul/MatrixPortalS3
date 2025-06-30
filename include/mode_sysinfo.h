#ifndef MODE_SYSINFO_H
#define MODE_SYSINFO_H

#include "config.h"
#include "utils.h"
#include "version.h"

// Forward declarations
class Utils;
class MatrixPanel_I2S_DMA;

class ModeSysinfo {
public:
    enum InfoModeType {
        INFO_SYSINFO = 0,
        INFO_NETWORK,
        INFO_MEMORY,
        INFO_MODE_COUNT  // 이 값이 자동으로 모드 개수가 됨
    };
    
    static const int MAX_INFO_MODES = INFO_MODE_COUNT; // 자동 계산
    
    void setup(Utils* utils_ptr, MatrixPanel_I2S_DMA* matrix_ptr);
    void run();
    void cleanup();
    
    // IR 명령어 처리
    void nextInfo();
    void prevInfo();
    
    // MQTT 서브모드 직접 설정 함수
    void setInfoMode(int mode);

private:
    Utils* m_utils;
    MatrixPanel_I2S_DMA* m_matrix;
    
    unsigned long lastUpdate;
    int currentInfoMode; // 0: SYSINFO, 1: NETWORK, 2: MEMORY
    
    void displayCurrentInfo();
    void displaySysInfo();      // SYSINFO
    void displayWiFiInfo();     // NETWORK  
    void displayMemoryInfo();   // MEMORY
};

extern ModeSysinfo modeSysinfo;

#endif