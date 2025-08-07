#pragma once

#include <string>

#include "types.hpp"

// --- Time Management ---

// Default timeout buffer in milliseconds to prevent premature timeouts
constexpr int DEFAULT_TIMEOUT_BUFFER_MS = 5000;  // 5 seconds buffer

struct TimeControl {
    int wtime_ms = 0;
    int btime_ms = 0;
    int winc_ms = 0;
    int binc_ms = 0;
};

class TimeManager {
   private:
    TimeControl tc;
    int timeout_buffer_ms;

   public:
    TimeManager(const TimeControl &initial_tc, int timeout_buffer_ms = DEFAULT_TIMEOUT_BUFFER_MS);

    void update(Color player_who_moved, long long elapsed_ms);
    bool is_out_of_time(Color player) const;
    int get_time_ms(Color player) const;
    std::string get_go_command() const;

    // Setter for timeout buffer
    void set_timeout_buffer(int buffer_ms);
};
