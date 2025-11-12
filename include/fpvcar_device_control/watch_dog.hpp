#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional> // 用于 std::function
#include "fpvcar-motor/fpvcar_controller.hpp"
/**
 * @brief 一个简单的C++软件看门狗类 会直接调用底层控制库停止运动
 */
namespace fpvcar::device_control {
    class SoftwareWatchdog {
public:
    /**
     * @param timeout 超时时间
     * @param car 小车控制器引用，超时后直接调用 car.stopAll() 停止所有电机
     * @note 看门狗作为独立的安全机制，不依赖 control_loop，即使 control_loop 卡死也能直接停止硬件
     */
    SoftwareWatchdog(std::chrono::milliseconds timeout, control::FpvCarController& car)
        : m_timeout(timeout),
          m_car(car),
          m_stop(false),
          m_kicked(false) {}

    ~SoftwareWatchdog() {
        stop();
    }

    /**
     * @brief 启动看门狗监控
     */
    void start();

    /**
     * @brief 停止看门狗
     */
    void stop();

    /**
     * @brief "喂狗" (由被监控的线程调用)
     */
    void feed();

private:
    /**
     * @brief 看门狗监控循环
    */
    void watchLoop();

    control::FpvCarController& m_car;
    std::chrono::milliseconds m_timeout; // 超时时间
    std::atomic<bool> m_stop; // 停止标志
    std::atomic<bool> m_kicked; // 被喂狗标志
    std::thread m_thread; // 看门狗线程
};

} // namespace fpvcar::device_control