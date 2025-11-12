#include "fpvcar_device_control/desired_state.hpp"


namespace fpvcar::device_control {
DesiredStateManager::DesiredStateManager() {
    m_desired_state = DesiredState::STOPPING;
}

DesiredStateManager::~DesiredStateManager() {
}

void DesiredStateManager::set_desired_state(const DesiredState& desired_state) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_desired_state = desired_state;
}

DesiredState DesiredStateManager::get_desired_state() {
    std::shared_lock<std::shared_mutex> shared_lock(m_mutex);
    return m_desired_state;
}

}