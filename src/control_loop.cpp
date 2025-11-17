#include "fpvcar_device_control/control_loop.hpp"
#include <iostream> // 用于打印状态和错误

namespace fpvcar::device_control {

ControlLoop::ControlLoop(DesiredStateManager& desired_state_manager, control::FpvCarController& car)
    : m_desired_state_manager(desired_state_manager),
      m_car(car),
      m_old_desired_state(DesiredState::STOPPING), // <--- 正确初始化
      m_is_running(false), // <--- 构造时为 false
      m_watchdog(std::chrono::milliseconds(5000), car, desired_state_manager)
{}

ControlLoop::~ControlLoop() {
    stop(); // 确保在对象销毁时，线程被正确停止和 join
}

void ControlLoop::start() {
    // 使用 exchange 来原子性地检查并设置 m_is_running
    // 防止重复调用 start
    if (m_is_running.exchange(true)) {
        return; // 如果已经是 true, 说明已启动, 直接返回
    }
    // 启动看门狗
    m_watchdog.start();
    // 启动新线程
    m_loop_thread = std::thread(&ControlLoop::run_loop, this);
}

void ControlLoop::stop() {
    // 使用 exchange 来原子性地检查并设置
    if (!m_is_running.exchange(false)) {
        return; // 如果已经是 false, 说明已停止, 直接返回
    }
    m_watchdog.stop();

    // [安全措施] 立即停止车辆，而不是等待循环下一次迭代
    m_car.stopAll();

    // 等待循环线程结束
    if (m_loop_thread.joinable()) {
        m_loop_thread.join();
    }
    std::cout << "Control loop stopped." << std::endl;
}

void ControlLoop::feed_watchdog() {
    m_watchdog.feed();
}

void ControlLoop::run_loop() {
    // 重置循环的起始时间
    m_next_loop_start_time = std::chrono::steady_clock::now();

    while (m_is_running.load(std::memory_order_relaxed)) {
        try {
            // --- 1. 获取状态 (安全地) ---
            DesiredState desired_state = m_desired_state_manager.get_desired_state();

            // --- 2. 检查状态变更 ---
            if (desired_state != m_old_desired_state) {
                m_old_desired_state = desired_state;
                switch (desired_state) {
                    case DesiredState::MOVING_FORWARD:
                        std::cout << "Moving forward" << std::endl;
                        m_car.moveForward();
                        break;
                    case DesiredState::MOVING_BACKWARD:
                        std::cout << "Moving backward" << std::endl;
                        m_car.moveBackward();
                        break;
                    case DesiredState::TURNING_LEFT:
                        std::cout << "Turning left" << std::endl;
                        m_car.turnLeft();
                        break;
                    case DesiredState::TURNING_RIGHT:
                        std::cout << "Turning right" << std::endl;
                        m_car.turnRight();
                        break;
                    case DesiredState::MOVING_FORWARD_AND_TURN_LEFT:
                        std::cout << "Moving forward and turning left" << std::endl;
                        m_car.moveForwardAndTurnLeft();
                        break;
                    case DesiredState::MOVING_FORWARD_AND_TURN_RIGHT:
                        std::cout << "Moving forward and turning right" << std::endl;
                        m_car.moveForwardAndTurnRight();
                        break;
                    case DesiredState::MOVING_BACKWARD_AND_TURN_LEFT:
                        std::cout << "Moving backward and turning left" << std::endl;
                        m_car.moveBackwardAndTurnLeft();
                        break;
                    case DesiredState::MOVING_BACKWARD_AND_TURN_RIGHT:
                        std::cout << "Moving backward and turning right" << std::endl;
                        m_car.moveBackwardAndTurnRight();
                        break;
                    case DesiredState::STOPPING:
                        std::cout << "Stopping" << std::endl;
                        m_car.stopAll();
                        break;
                }
            }
            // --- 3. 固定周期休眠 ---
            m_next_loop_start_time += m_target_interval;

            // "掉帧"检测
            auto now = std::chrono::steady_clock::now();
            if (now > m_next_loop_start_time) {
                std::cerr << "Warning: Control loop is overloaded (missed interval)!" << std::endl;
                // 如果工作时间超过了间隔, 立即开始下一次循环
                // 并且把下次启动时间重置为 "当前时间 + 间隔"
                m_next_loop_start_time = now + m_target_interval;
            }

            std::this_thread::sleep_until(m_next_loop_start_time);
        } catch (const std::exception& e) {
            std::cerr << "Error in control loop: " << e.what() << std::endl;
        }
    }
}

}// namespace fpvcar::device_control