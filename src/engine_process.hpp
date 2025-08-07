#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#endif

// --- Engine Process Management ---

class EngineProcess {
   private:
#ifdef _WIN32
    PROCESS_INFORMATION pi_{};
    HANDLE h_child_stdin_read_ = NULL;
    HANDLE h_child_stdin_write_ = NULL;
    HANDLE h_child_stdout_read_ = NULL;
    HANDLE h_child_stdout_write_ = NULL;
    // Buffer for Windows to handle multiple lines in one read
    std::string windows_read_buffer;
#else
    FILE *engine_pipe_read_ = nullptr;
    FILE *engine_pipe_write_ = nullptr;
    pid_t pid_ = -1;
#endif

   public:
    EngineProcess();
    ~EngineProcess();

    bool start(const std::string &command);
    void stop();
    void write_line(const std::string &line);
    std::string read_line();
    bool is_running() const;
};