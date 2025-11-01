## 接口文档（API）

本库对外提供头文件位于 `include/fpvcar-motor/` 目录，核心目标为 CMake 库 `fpvcar_motor`（CMake 别名 `fpvcar::motor`）。
代码命名空间分层：
- 配置：`fpvcar::config`
- 电机（底层）：`fpvcar::motor`
- 控制器（上层）：`fpvcar::control`

### 常量（均位于 `fpvcar::config`）
- `GPIO_CHIP_NAME: const char*` 默认 GPIO 芯片路径，示例：`"/dev/gpiochip0"`
- `GPIO_CONSUMER_NAME: const char*` 默认消费者名称，示例：`"fpvcar-motor"`

### 结构体（位于 `fpvcar::config`）
#### `fpvcar::config::FpvCarPinConfig`
四个电机（前左/前右/后左/后右）各两根控制引脚（H 桥 IN1/IN2）的编号（offset）。

字段：
- `int fl_pin_a`, `int fl_pin_b`
- `int fr_pin_a`, `int fr_pin_b`
- `int bl_pin_a`, `int bl_pin_b`
- `int br_pin_a`, `int br_pin_b`

说明：引脚为 libgpiod 的 offset，不是 BCM/GPIO 物理编号，请根据平台映射配置。

### 类
#### `class fpvcar::motor::Motor`
单个直流电机（2-pin）。通常无需直接在上层应用使用，由 `FpvCarController` 管理。

构造函数：
- `Motor(gpiod::chip& chip, int pinA, int pinB, const std::string& consumer)`

方法：
- `void forward()` 前进（A 高、B 低）
- `void reverse()` 后退（A 低、B 高）
- `void stop()` 停止（两线拉低）

注意：`Motor` 禁止拷贝和赋值；GPIO 资源通过 RAII 在析构时自动释放。

#### `class fpvcar::control::FpvCarController`
管理四个电机，提供高级动作接口。

构造函数：
- `FpvCarController(const std::string& chipName, const fpvcar::config::FpvCarPinConfig& config, const std::string& consumer)`

方法：
- `void moveForward()` 四轮前进
- `void moveBackward()` 四轮后退
- `void turnLeft()` 左转（右侧前进，左侧后退）
- `void turnRight()` 右转（左侧前进，右侧后退）
- `void stopAll()` 停止四轮

语义与资源管理：
- 构造时打开 `gpiod::chip` 并请求所需引脚；
- 析构时自动释放所有请求的 GPIO 资源；
- 禁止拷贝与赋值。

### 使用示例
#### 最小示例
```cpp
#include "fpvcar-motor/fpvcar_controller.hpp"
#include "fpvcar-motor/config.hpp"

int main() {
    fpvcar::config::FpvCarPinConfig pins{17,27, 22,23, 24,25, 5,6};
    fpvcar::control::FpvCarController car(fpvcar::config::GPIO_CHIP_NAME,
                                          pins,
                                          fpvcar::config::GPIO_CONSUMER_NAME);
    car.moveForward();
    car.stopAll();
}
```

#### 典型流程
1) 准备 `FpvCarPinConfig`（确保为有效 offset 且未被占用）
2) 以芯片名（或路径）与消费者名构造 `fpvcar::control::FpvCarController`
3) 调用动作方法；
4) 退出作用域自动释放资源。

### 依赖约束
- 需要 `libgpiod` v2 C++ API（`gpiod.hpp`）与 `tl::expected`；
- 运行时需要对目标 GPIO 芯片与引脚有足够权限；
- 在容器或非 root 环境下，请配置 udev/权限或以合适用户运行。

### 错误与异常
- 构造/动作失败将抛出标准异常（如打开芯片失败、引脚请求失败）；
- 建议在应用侧捕获 `std::exception` 并给出友好日志。

### 线程与时序
- 接口未内置并发保护；如多线程控制，请在上层进行序列化；
- 电机操作需留有硬件响应时间，避免高频切换方向导致硬件应力。

### 版本兼容
- 本文档基于项目 `1.0.0`；同 Major 版本内保证 API 向后兼容（破坏性变更将提升 Major）。


