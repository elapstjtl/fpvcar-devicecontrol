非常好的 C++ API 文档！这个 `fpvcar::control::FpvCarController` 封装得很好，它就是你 `fpvcar-devicecontrol` 服务的核心。

基于这份文档，`fpvcar-gateway`（客户端）和 `fpvcar-devicecontrol`（服务端）之间的 IPC 消息格式可以设计得非常简洁。

### 核心设计思路

1.  **`fpvcar-devicecontrol` (服务端)**：它在启动时，就应该根据配置（来自 `fpvcar-gateway` 的启动指令或本地文件）**构造一个** `fpvcar::control::FpvCarController` 实例并**长期持有**。
2.  **`fpvcar-gateway` (客户端)**：它发送的 JSON 消息，本质上就是对这个 `FpvCarController` 实例的**方法调用**。
3.  **JSON 格式**：我们只需要一个字段 `action` 来指定调用哪个方法。

-----

### JSON 消息格式

这是一个“请求-响应”模式。`gateway` 发送“请求”，`devicecontrol` 返回“响应”。

#### 1\. 发送什么 (Request)

`fpvcar-gateway` -\> `fpvcar-devicecontrol`

我们使用一个统一的 JSON 结构，只改变 `action` 字段的值。

```json
{
  "action": "string"
}
```

`action` 字段的 **有效值** 严格对应 `FpvCarController` 的公共方法：

  * `"moveForward"`
  * `"moveBackward"`
  * `"turnLeft"`
  * `"turnRight"`
  * `"stopAll"`

#### 2\. 返回什么 (Response)

`fpvcar-devicecontrol` -\> `fpvcar-gateway`

响应也使用统一的 JSON 结构，包含 `status` 和可选的 `message` 字段。

```json
{
  "status": "ok" | "error",
  "error_code": "string", // 仅在 status == "error" 时出现
  "message": "string"     // 详细信息
}
```

-----

### 在什么情况下 (场景示例)

以下是 `fpvcar-devicecontrol` (服务端) 的处理逻辑和返回内容。

服务端的核心逻辑伪代码 (C++):

```cpp
// 在 fpvcar-devicecontrol 服务启动时，只执行一次
// 注意：如果这里构造失败，整个服务应该退出或报告严重错误
// 构造函数会抛出异常，必须捕获
try {
    fpvcar::config::FpvCarPinConfig pins = {...}; // 从配置加载
    FpvCarController car(config::GPIO_CHIP_NAME, pins, config::GPIO_CONSUMER_NAME);
} catch (const std::exception& e) {
    log("FATAL: Failed to initialize FpvCarController: %s", e.what());
    // 退出服务
}

// ...
// 循环监听来自 gateway 的 IPC 消息 (JSON 字符串)
while (auto request_json = ipc.listen()) {
    std::string response_json;
    try {
        // 1. 解析 JSON
        json data = json::parse(request_json);
        std::string action = data.at("action");

        // 2. 根据 action 调用对应 C++ API
        if (action == "moveForward") {
            car.moveForward();
            response_json = make_success_response("moveForward executed");
        } else if (action == "stopAll") {
            car.stopAll();
            response_json = make_success_response("stopAll executed");
        } 
        // ... (else if for moveBackward, turnLeft, turnRight)
        else {
            // 3. 无效 Action
            response_json = make_error_response("INVALID_ACTION", "Unknown action: " + action);
        }
    } catch (const json::parse_error& e) {
        // 3. 无效 JSON
        response_json = make_error_response("INVALID_JSON", "Failed to parse JSON: " + std::string(e.what()));
    } catch (const std::exception& e) {
        // 3. 硬件/API 异常 (来自 car.moveForward() 等)
        response_json = make_error_response("HARDWARE_ERROR", "Exception caught: " + std::string(e.what()));
    }
    
    // 4. 发回响应
    ipc.reply(response_json);
}
```

-----

### 场景详解

#### 场景 1：成功执行 (前进)

1.  **Gateway 发送**:
    ```json
    { "action": "moveForward" }
    ```
2.  **DeviceControl 处理**:
      * 解析 JSON 成功。
      * 找到 `action == "moveForward"`。
      * 调用 `car.moveForward()`。
      * `moveForward()` **未抛出异常**，执行成功。
3.  **DeviceControl 返回**:
    ```json
    {
      "status": "ok",
      "message": "moveForward executed"
    }
    ```

#### 场景 2：成功执行 (停止)

1.  **Gateway 发送**:
    ```json
    { "action": "stopAll" }
    ```
2.  **DeviceControl 处理**:
      * 调用 `car.stopAll()` 成功。
3.  **DeviceControl 返回**:
    ```json
    {
      "status": "ok",
      "message": "stopAll executed"
    }
    ```

#### 场景 3：失败 (无效的 Action)

1.  **Gateway 发送**:
    ```json
    { "action": "flyToTheMoon" }
    ```
2.  **DeviceControl 处理**:
      * 解析 JSON 成功。
      * 在 `if/else` 逻辑中**找不到**匹配的 `action`。
3.  **DeviceControl 返回**:
    ```json
    {
      "status": "error",
      "error_code": "INVALID_ACTION",
      "message": "Unknown action: flyToTheMoon"
    }
    ```

#### 场景 4：失败 (硬件/API 异常)

根据你的文档，API 可能会在引脚请求失败时抛出异常。

1.  **Gateway 发送**:
    ```json
    { "action": "turnLeft" }
    ```
2.  **DeviceControl 处理**:
      * 解析 JSON 成功。
      * 找到 `action == "turnLeft"`。
      * 调用 `car.turnLeft()`。
      * `turnLeft()` 内部的 `gpiod::line::set_value()` **抛出了 `std::exception`** (例如，假设此时引脚被其他进程占用)。
      * `try...catch (const std::exception& e)` 捕获了该异常。
3.  **DeviceControl 返回**:
    ```json
    {
      "status": "error",
      "error_code": "HARDWARE_ERROR",
      "message": "Exception caught: gpiod: error setting line value" 
    }
    ```
    *(这里的 `message` 内容取决于 `e.what()` 的具体内容)*

#### 场景 5：失败 (无效的 JSON 格式)

1.  **Gateway 发送** (一个损坏的 JSON):
    ```
    { "action": "moveForward" 
    ```
2.  **DeviceControl 处理**:
      * `json::parse()` 抛出 `json::parse_error` 异常。
      * `try...catch (const json::parse_error& e)` 捕获了该异常。
3.  **DeviceControl 返回**:
    ```json
    {
      "status": "error",
      "error_code": "INVALID_JSON",
      "message": "Failed to parse JSON: [detail about parsing error]"
    }
    ```

这个 JSON 格式足够简单且健壮，`gateway` 不需要知道任何 `gpiod` 的细节，只需要发送命令并检查 `status` 即可。