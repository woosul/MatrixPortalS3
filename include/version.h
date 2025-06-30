#ifndef VERSION_H
#define VERSION_H

#include <Arduino.h>

// Version information
#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 0  
#define FW_VERSION_PATCH 2
#define FW_VERSION_BUILD "20250628-010948"

// Git information
#define FW_GIT_HASH "unknown"
#define FW_GIT_BRANCH "unknown"

// Device information
#define DEVICE_TYPE "MatrixPortal_S3"
#define HARDWARE_VERSION "v1.0"


class Version {
public:
    // 외부에서 사용하는 주요 함수들
    static String getFirmwareVersion();          // getBuildInfo()에서 사용
    static String getFirmwareVersionShort();     // mode_sysinfo에서 사용
    static String getBuildInfo();                 // 전체 빌드 정보
    static String getDeviceInfo();                // 디바이스 정보

private:
    // 내부 헬퍼 함수들 (현재 사용되지 않음)
    static String getBuildDate();                 // 빌드 날짜만
    static String getBuildTime();                 // 빌드 시간만
    static String getGitInfo();                   // Git 정보만
};

#endif