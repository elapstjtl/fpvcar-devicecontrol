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
    m_stop = false;
    m_kicked = true; // 初始为true，防止启动时立即超时
    m_thread = std::thread(&SoftwareWatchdog::watchLoop, this);
}

void SoftwareWatchdog::stop() {
    m_stop = true;
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
            std::cerr << "!!! 软件看门狗超时 !!!" << std::endl;
            if (m_onTimeoutCallback) {
                m_onTimeoutCallback();
            }
            // 超时后，我们也停止看门狗线程
            m_stop = true; 
        }
    }
}

} // namespace fpvcar::device_control