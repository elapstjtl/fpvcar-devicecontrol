#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional> // 用于 std::function
#include "fpvcar_device_control/watch_dog.hpp"

/**
 * @brief 一个简单的C++软件看门狗类
 */
namespace fpvcar::device_control {

void SoftwareWatchdog::start() {
    // 如果线程已经在运行，先停止它
    // 注意：这里直接设置 m_stop 并 join，不使用 stop() 避免可能的竞争
    if (m_thread.joinable()) {
        m_stop.store(true, std::memory_order_relaxed);
        m_thread.join();
    }
    
    // 重置状态并启动新线程
    m_stop.store(false, std::memory_order_relaxed);
    m_kicked.store(true, std::memory_order_relaxed); // 初始为true，防止启动时立即超时
    m_thread = std::thread(&SoftwareWatchdog::watchLoop, this);
}

void SoftwareWatchdog::stop() {
    // 原子地设置停止标志，如果已经是 true 则直接返回（避免重复停止）
    bool expected = false;
    if (!m_stop.compare_exchange_strong(expected, true)) {
        // 已经停止，直接返回
        return;
    }
    
    // 等待线程结束
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void SoftwareWatchdog::feed() {
    m_kicked.store(true, std::memory_order_relaxed);
}


void SoftwareWatchdog::watchLoop() {
    while (!m_stop.load(std::memory_order_relaxed)) {
        // 1. 等待一个超时周期
        std::this_thread::sleep_for(m_timeout);

        // 2. 检查是否被 "喂" 过
        //    我们使用 exchange 来原子地检查并重置标志
        bool wasKicked = m_kicked.exchange(false, std::memory_order_relaxed);

        if (!wasKicked) {
            // 3. 超时！
            std::cerr << "!!! 软件看门狗超时 停止所有电机 !!!" << std::endl;
            m_car.stopAll();

            m_kicked.store(true, std::memory_order_relaxed); // 重置标志，防止立即再次超时
        }
    }
}

} // namespace fpvcar::device_control