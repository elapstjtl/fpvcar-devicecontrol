#pragma once // 推荐使用 #pragma once 替代 include guards

#include <thread>
#include <chrono>
#include <atomic> // <--- 包含 atomic

#include "fpvcar-motor/fpvcar_controller.hpp"
#include "fpvcar_device_control/desired_state.hpp"
#include "fpvcar_device_control/watch_dog.hpp"
//这个类用于具体控制小车的运动，根据期望状态管理器中的期望状态，控制小车运动

namespace fpvcar::device_control {

    /**
    * @brief 控制循环，用于控制小车运动
    * @param desired_state_manager 期望状态管理器
    * @param car 小车控制器
    * @note 控制循环会定期检查期望状态管理器中的期望状态，并根据期望状态控制小车运动

    */
class ControlLoop {
public:
    ControlLoop(DesiredStateManager& desired_state_manager, control::FpvCarController& car);
    ~ControlLoop(); // <--- 添加析构函数

    // 禁止拷贝和赋值，因为我们管理着一个线程
    ControlLoop(const ControlLoop&) = delete;
    ControlLoop& operator=(const ControlLoop&) = delete;

    /**
    * @brief 启动控制循环
    * @note 启动控制循环会启动一个新线程，用于控制小车运动
    */
    void start();

    /**
    * @brief 停止控制循环
    * @note 停止控制循环会停止控制循环线程
    */
    void stop();

private:
    void run_loop(); // <--- 循环的私有实现

    DesiredStateManager& m_desired_state_manager;
    control::FpvCarController& m_car;
    DesiredState m_old_desired_state;
    SoftwareWatchdog m_watchdog; // 看门狗
    
    std::atomic<bool> m_is_running{false}; // <--- 使用 atomic 并默认为 false // 避免编译器优化导致线程不安全
    std::thread m_loop_thread; // <--- 用于运行循环的线程

    const std::chrono::milliseconds m_target_interval = std::chrono::milliseconds(10); // <--- 目标循环间隔时间
    std::chrono::time_point<std::chrono::steady_clock> m_next_loop_start_time;
};

} // namespace fpvcar::device_control