#!/bin/bash
# IPC 通信测试脚本
# 使用方法: ./test_ipc.sh <action>
# 示例: ./test_ipc.sh moveForward

SOCKET_PATH="/tmp/fpvcar_control.sock"

if [ $# -eq 0 ]; then
    echo "用法: $0 <action>"
    echo "支持的 action: moveForward, moveBackward, turnLeft, turnRight, stopAll"
    exit 1
fi

ACTION=$1

# 检查 socat 是否安装
if ! command -v socat &> /dev/null; then
    echo "错误: 未找到 socat 命令"
    echo "请安装 socat: sudo apt-get install socat"
    exit 1
fi

# 检查套接字文件是否存在
if [ ! -S "$SOCKET_PATH" ]; then
    echo "错误: 套接字文件不存在: $SOCKET_PATH"
    echo "请确保 fpvcar-devicecontrol 服务正在运行"
    exit 1
fi

# 构造 JSON 请求
JSON_REQUEST="{\"action\":\"$ACTION\"}"

echo "发送请求: $JSON_REQUEST"
echo "---"

# 使用 socat 发送请求并接收响应
echo "$JSON_REQUEST" | socat - UNIX-CONNECT:"$SOCKET_PATH"

echo ""
