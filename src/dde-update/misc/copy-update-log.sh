#!/bin/bash

set -euo pipefail

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | systemd-cat -t deepin-update-log-copy
}

error_exit() {
    log "ERROR: $1"
    exit 1
}

# 解析参数：格式为十六进制编码的 "源路径:目标路径"
ENCODED_PARAM="${1:-}"

if [ -z "$ENCODED_PARAM" ]; then
    error_exit "Usage: $0 hex_encoded_paths"
fi

log "Received encoded parameter: $ENCODED_PARAM"

# 解码十六进制参数
PATHS_PARAM=$(printf '%b' "$(echo "$ENCODED_PARAM" | sed 's/../\\x&/g')" 2>/dev/null || echo "")

if [ -z "$PATHS_PARAM" ]; then
    error_exit "Failed to decode hex parameter"
fi

log "Decoded paths parameter: $PATHS_PARAM"

# 使用冒号分隔符解析源路径和目标路径
IFS=':' read -r SOURCE_FILE TARGET_DIR <<< "$PATHS_PARAM"

if [ -z "$SOURCE_FILE" ] || [ -z "$TARGET_DIR" ]; then
    error_exit "Invalid parameter format. Expected 'source_path:target_path'"
fi

log "SOURCE_FILE: $SOURCE_FILE"
log "TARGET_DIR: $TARGET_DIR"

SOURCE_DIR=$(dirname "$SOURCE_FILE")
SOURCE_BASENAME=$(basename "$SOURCE_FILE")

# 检查目录前缀（固定使用 /tmp/deepin-update-ui 目录）
if [[ ! "$SOURCE_DIR" == "/tmp/deepin-update-ui" ]]; then
    error_exit "Invalid source directory, must be /tmp/deepin-update-ui, actual: $SOURCE_DIR"
fi

# 检查文件后缀
if [[ ! "$SOURCE_BASENAME" =~ \.txt$ ]]; then
    error_exit "Invalid source file suffix: $SOURCE_BASENAME"
fi

# 检查源文件是否存在
if [ ! -f "$SOURCE_FILE" ]; then
    error_exit "Source file does not exist: $SOURCE_FILE"
fi

# 检查源文件不是软连接
if [ -L "$SOURCE_FILE" ]; then
    error_exit "Source file cannot be a symbolic link: $SOURCE_FILE"
fi

# 仅可复制lightdm用户创建的文件
FILE_OWNER=$(stat -c %U "$SOURCE_FILE" 2>/dev/null || echo "")
if [ "$FILE_OWNER" != "lightdm" ]; then
    error_exit "Source file owner must be lightdm, actual owner: $FILE_OWNER"
fi

log "Starting to copy update log from: $SOURCE_FILE to: $TARGET_DIR"

# 检查目标目录是否存在
if [ ! -d "$TARGET_DIR" ]; then
    error_exit "Target directory not found: $TARGET_DIR"
fi

# 获取目标目录的所有者信息
TARGET_UID=$(stat -c %u "$TARGET_DIR" 2>/dev/null || echo "")
TARGET_GID=$(stat -c %g "$TARGET_DIR" 2>/dev/null || echo "")

if [ -z "$TARGET_UID" ] || [ -z "$TARGET_GID" ]; then
    error_exit "Cannot determine target directory ownership"
fi

log "Target directory owner: UID=$TARGET_UID GID=$TARGET_GID"

# 目标文件名
TARGET_FILE="$TARGET_DIR/$SOURCE_BASENAME"

log "Copying to: $TARGET_FILE"

if cp -f "$SOURCE_FILE" "$TARGET_FILE"; then
    # 设置正确的文件权限和所有者
    chmod 644 "$TARGET_FILE"
    chown "$TARGET_UID:$TARGET_GID" "$TARGET_FILE"
    
    # 清理源文件（使用固定目录，需要手动清理）
    rm -f "$SOURCE_FILE"

    # 如果临时目录为空，则删除目录
    if [ -d "/tmp/deepin-update-ui" ] && [ -z "$(ls -A /tmp/deepin-update-ui 2>/dev/null)" ]; then
        rmdir /tmp/deepin-update-ui 2>/dev/null || true
    fi
    
    log "Successfully copied update log to desktop: $TARGET_FILE"
else
    error_exit "Failed to copy file"
fi
