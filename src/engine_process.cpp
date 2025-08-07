#include "engine_process.hpp"

#include <iostream>
#include <vector>

EngineProcess::EngineProcess() = default;

EngineProcess::~EngineProcess() {
    stop();
}

bool EngineProcess::start(const std::string &command) {
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&h_child_stdout_read_, &h_child_stdout_write_, &sa, 0)) return false;
    if (!SetHandleInformation(h_child_stdout_read_, HANDLE_FLAG_INHERIT, 0)) return false;
    if (!CreatePipe(&h_child_stdin_read_, &h_child_stdin_write_, &sa, 0)) return false;
    if (!SetHandleInformation(h_child_stdin_write_, HANDLE_FLAG_INHERIT, 0)) return false;

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(STARTUPINFOA));
    si.cb = sizeof(STARTUPINFOA);
    si.hStdError = h_child_stdout_write_;
    si.hStdOutput = h_child_stdout_write_;
    si.hStdInput = h_child_stdin_read_;
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::vector<char> cmd_vec(command.begin(), command.end());
    cmd_vec.push_back('\0');

    if (!CreateProcessA(NULL, cmd_vec.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi_)) {
        return false;
    }
    CloseHandle(h_child_stdout_write_);
    CloseHandle(h_child_stdin_read_);
    return true;
#else
    int parent_to_child[2];
    int child_to_parent[2];

    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        std::cerr << "Pipe creation failed.\n";
        return false;
    }

    pid_ = fork();
    if (pid_ == -1) {
        std::cerr << "Fork failed.\n";
        return false;
    }

    if (pid_ == 0) {  // Child process
        close(parent_to_child[1]);
        dup2(parent_to_child[0], STDIN_FILENO);
        close(parent_to_child[0]);

        close(child_to_parent[0]);
        dup2(child_to_parent[1], STDOUT_FILENO);
        close(child_to_parent[1]);

        execl("/bin/sh", "sh", "-c", command.c_str(), (char *)NULL);
        exit(127);
    } else {  // Parent process
        close(parent_to_child[0]);
        engine_pipe_write_ = fdopen(parent_to_child[1], "w");

        close(child_to_parent[1]);
        engine_pipe_read_ = fdopen(child_to_parent[0], "r");

        if (!engine_pipe_write_ || !engine_pipe_read_) return false;

        setvbuf(engine_pipe_write_, NULL, _IOLBF, 0);
    }
    return true;
#endif
}

void EngineProcess::stop() {
#ifdef _WIN32
    if (pi_.hProcess) {
        TerminateProcess(pi_.hProcess, 0);
        CloseHandle(pi_.hProcess);
        CloseHandle(pi_.hThread);
        pi_.hProcess = NULL;
    }
    if (h_child_stdin_write_) {
        CloseHandle(h_child_stdin_write_);
        h_child_stdin_write_ = NULL;
    }
    if (h_child_stdout_read_) {
        CloseHandle(h_child_stdout_read_);
        h_child_stdout_read_ = NULL;
    }
#else
    if (pid_ > 0) {
        kill(pid_, SIGKILL);
        waitpid(pid_, NULL, 0);
        pid_ = -1;
    }
    if (engine_pipe_read_) {
        fclose(engine_pipe_read_);
        engine_pipe_read_ = nullptr;
    }
    if (engine_pipe_write_) {
        fclose(engine_pipe_write_);
        engine_pipe_write_ = nullptr;
    }
#endif
}

void EngineProcess::write_line(const std::string &line) {
    if (!is_running()) return;
#ifdef _WIN32
    DWORD bytes_written;
    std::string full_line = line + "\n";
    WriteFile(h_child_stdin_write_, full_line.c_str(), full_line.length(), &bytes_written, NULL);
#else
    fprintf(engine_pipe_write_, "%s\n", line.c_str());
    fflush(engine_pipe_write_);
#endif
}

std::string EngineProcess::read_line() {
    if (!is_running()) return "";
#ifdef _WIN32
    while (true) {
        size_t newline_pos = windows_read_buffer.find('\n');
        if (newline_pos != std::string::npos) {
            std::string line = windows_read_buffer.substr(0, newline_pos);
            windows_read_buffer.erase(0, newline_pos + 1);
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }

        char buffer[4096];
        DWORD bytes_read;
        if (ReadFile(h_child_stdout_read_, buffer, sizeof(buffer), &bytes_read, NULL) &&
            bytes_read > 0) {
            windows_read_buffer.append(buffer, bytes_read);
        } else {
            // Pipe closed or error
            if (!windows_read_buffer.empty()) {
                std::string line = windows_read_buffer;
                windows_read_buffer.clear();
                return line;
            }
            return "";
        }
    }
#else
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), engine_pipe_read_)) {
        std::string line = buffer;
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
        return line;
    }
    return "";
#endif
}

bool EngineProcess::is_running() const {
#ifdef _WIN32
    return pi_.hProcess != NULL;
#else
    return pid_ != -1;
#endif
}