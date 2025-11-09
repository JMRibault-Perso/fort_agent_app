#!/bin/bash

# Set these to match your setup
TARGET_USER=root
TARGET_HOST=192.168.3.87
TARGET_PATH=fort_agent
BINARY_NAME=fort_agent_app
LOCAL_BINARY=./build/aarch64/apps/$BINARY_NAME
LOCAL_CONFIG=./config/fort-agent.conf
LOCAL_OJCONF=./config/fort_agent_app.ojconf

# Copy the binary via scp
scp "$LOCAL_BINARY" "$TARGET_USER@$TARGET_HOST:$TARGET_PATH"
scp "$LOCAL_CONFIG" "$TARGET_USER@$TARGET_HOST:$TARGET_PATH"
scp "$LOCAL_OJCONF" "$TARGET_USER@$TARGET_HOST:$TARGET_PATH"

# Optional: SSH in and run it
# ssh "$TARGET_USER@$TARGET_HOST" "$TARGET_PATH/$BINARY_NAME --config $TARGET_PATH/fort-agent.conf"