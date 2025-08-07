#include "time_manager.hpp"

#include <format>

TimeManager::TimeManager(const TimeControl &initial_tc, int timeout_buffer_ms)
    : tc(initial_tc), timeout_buffer_ms(timeout_buffer_ms) {}

void TimeManager::update(Color player_who_moved, long long elapsed_ms) {
    if (player_who_moved == Color::RED) {
        tc.wtime_ms -= elapsed_ms;
        tc.wtime_ms += tc.winc_ms;
    } else {
        tc.btime_ms -= elapsed_ms;
        tc.btime_ms += tc.binc_ms;
    }
}

bool TimeManager::is_out_of_time(Color player) const {
    // Apply timeout buffer to prevent premature timeouts
    if (player == Color::RED) {
        return tc.wtime_ms <= -timeout_buffer_ms;
    } else {
        return tc.btime_ms <= -timeout_buffer_ms;
    }
}

int TimeManager::get_time_ms(Color player) const {
    return player == Color::RED ? tc.wtime_ms : tc.btime_ms;
}

std::string TimeManager::get_go_command() const {
    return std::format("go wtime {} btime {} winc {} binc {}", tc.wtime_ms, tc.btime_ms, tc.winc_ms,
                       tc.binc_ms);
}

void TimeManager::set_timeout_buffer(int buffer_ms) {
    timeout_buffer_ms = buffer_ms;
}