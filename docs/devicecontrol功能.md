#### 2. `fpvcar-devicecontrol` (设备控制服务)
- **核心职责**：小车的“大脑”和“四肢”的执行者。
- **角色**：扮演一个 **IPC 服务端**。
    
- **功能**：
    - 接收 `fpvcar-gateway` 通过 IPC 发来的控制指令 (如 `MoveForward`, `TurnLeft`)。
    - **调用** `fpvcar-motor` 库的 C++ 接口，实际驱动电机。
    - (可选) 管理其他硬件，如灯光、舵机等。