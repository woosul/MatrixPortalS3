#!/usr/bin/env python3
"""
Git 정보를 자동으로 version.h에 업데이트하는 스크립트
PlatformIO 빌드 전에 실행됩니다.
"""

import subprocess
import os
import re
from datetime import datetime

def get_git_info():
    """Git 정보를 가져옵니다."""
    try:
        # Git hash (short)
        git_hash = subprocess.check_output(['git', 'rev-parse', '--short', 'HEAD'], 
                                         stderr=subprocess.DEVNULL).decode('utf-8').strip()
        
        # Git branch
        git_branch = subprocess.check_output(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], 
                                           stderr=subprocess.DEVNULL).decode('utf-8').strip()
        
        # Check if working directory is clean
        git_status = subprocess.check_output(['git', 'status', '--porcelain'], 
                                           stderr=subprocess.DEVNULL).decode('utf-8').strip()
        
        if git_status:
            git_hash += "-dirty"
            
        return git_hash, git_branch
        
    except (subprocess.CalledProcessError, FileNotFoundError):
        # Git이 없거나 Git 저장소가 아닌 경우
        return "unknown", "unknown"

def update_version_header():
    """version.h 파일을 업데이트합니다."""
    
    # 현재 스크립트의 디렉토리 기준으로 프로젝트 루트 찾기
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    version_file = os.path.join(project_root, 'include', 'version.h')
    
    if not os.path.exists(version_file):
        print(f"Warning: {version_file} not found")
        return
    
    # Git 정보 가져오기
    git_hash, git_branch = get_git_info()
    
    # 현재 시간
    build_time = datetime.now().strftime("%Y%m%d-%H%M%S")
    
    # version.h 읽기
    with open(version_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Git hash 업데이트
    content = re.sub(r'#define FW_GIT_HASH\s+"[^"]*"', 
                     f'#define FW_GIT_HASH "{git_hash}"', content)
    
    # Git branch 업데이트
    content = re.sub(r'#define FW_GIT_BRANCH\s+"[^"]*"', 
                     f'#define FW_GIT_BRANCH "{git_branch}"', content)
    
    # Build number 업데이트 (선택사항)
    content = re.sub(r'#define FW_VERSION_BUILD\s+\d+', 
                     f'#define FW_VERSION_BUILD {build_time}', content)
    
    # 파일에 쓰기
    with open(version_file, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Updated version.h:")
    print(f"  Git Hash: {git_hash}")
    print(f"  Git Branch: {git_branch}")
    print(f"  Build Time: {build_time}")

if __name__ == "__main__":
    update_version_header()
