#pragma once
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional> // 用于 std::function

/**
 * @brief 一个简单的C++软件看门狗类
 */
namespace fpvcar::device_control {
    class SoftwareWatchdog {
public:
    /**
     * @param timeout 超时时间
     * @param onTimeoutCallback 超时后执行的回调函数
     */
    SoftwareWatchdog(std::chrono::milliseconds timeout, std::function<void()> onTimeoutCallback)
        : m_timeout(timeout),
          m_onTimeoutCallback(onTimeoutCallback),
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

    std::chrono::milliseconds m_timeout; // 超时时间
    std::function<void()> m_onTimeoutCallback; // 超时后执行的回调函数
    std::atomic<bool> m_stop; // 停止标志
    std::atomic<bool> m_kicked; // 被喂狗标志
    std::thread m_thread; // 看门狗线程
};

} // namespace fpvcar::device_control