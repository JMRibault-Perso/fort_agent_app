#!/bin/bash

# Set these to match your setup
TARGET_USER=root
TARGET_HOST=192.168.86.29
TARGET_PATH=fort_agent
LOCAL_BINARY=./build/aarch64/apps/fort_agent_app

# Copy the binary via scp
scp "$LOCAL_BINARY" "$TARGET_USER@$TARGET_HOST:$TARGET_PATH"

# Optional: SSH in and run it
# ssh "$TARGET_USER@$TARGET_HOST" "$TARGET_PATH/your_binary_name"