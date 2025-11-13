#pragma once
#include <tl/expected.hpp>
#include <string>        
#include <atomic>        
#include <chrono>      
#include <mutex>   
#include <shared_mutex>   

// 这个文件主要提供共享状态模型的实现，用于管理期望状态
// 向control_loop提供读取共享状态的接口
//向ipc_server提供修改共享状态的接口

namespace fpvcar::device_control {
    /**
     * @brief 期望状态结构体
     * @param is_moving_forward 是否期望前进
     * @param is_moving_backward 是否期望后退
     * @param is_turning_left 是否期望左转
     * @param is_turning_right 是否期望右转
     * @param is_stopping 是否期望停止
     */
    enum class DesiredState {
        MOVING_FORWARD,
        MOVING_BACKWARD,
        TURNING_LEFT,
        TURNING_RIGHT,
        STOPPING
    };

    class DesiredStateManager {
    public:
        DesiredStateManager();
        ~DesiredStateManager();

        /**
        * @brief 设置期望状态
        * @param desired_state 期望状态
        * @return 返回 void
        * @note 设置期望状态会覆盖当前期望状态
        */
        void set_desired_state(const DesiredState& desired_state);

        /**
        * @brief 获取期望状态
        * @return 返回期望状态
        * @note 获取期望状态会返回当前期望状态
        */
        DesiredState get_desired_state();

    private:
        DesiredState m_desired_state; // 期望状态结构体
        std::shared_mutex m_mutex; // 共享锁
    };
}