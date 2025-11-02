# IPC 通信测试指南

本文档介绍如何通过终端测试 IPC 通信。

## 前提条件

1. **启动服务**: 确保 `fpvcar-devicecontrol` 服务正在运行
   ```bash
   # 服务启动后会监听套接字: /tmp/fpvcar_control.sock
   ```

2. **套接字路径**: `/tmp/fpvcar_control.sock`

3. **请求格式**: JSON 格式，必须包含 `action` 字段
   ```json
   {"action": "moveForward"}
   ```

4. **支持的 action**:
   - `moveForward` - 前进
   - `moveBackward` - 后退
   - `turnLeft` - 左转
   - `turnRight` - 右转
   - `stopAll` - 停止所有动作

## 测试方法

### 方法 1: 使用 Bash 脚本（推荐）

使用提供的 `test_ipc.sh` 脚本：

```bash
# 测试前进
./scripts/test_ipc.sh moveForward

# 测试后退
./scripts/test_ipc.sh moveBackward

# 测试左转
./scripts/test_ipc.sh turnLeft

# 测试右转
./scripts/test_ipc.sh turnRight

# 测试停止
./scripts/test_ipc.sh stopAll
```

**要求**: 需要安装 `socat`:
```bash
sudo apt-get install socat
```