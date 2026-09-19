#include "process.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace power_timer {
namespace {
constexpr std::size_t output_limit = 8192;

struct Pipe {
    int read = -1;
    int write = -1;
    ~Pipe() {
        if (read >= 0) ::close(read);
        if (write >= 0) ::close(write);
    }
};

struct SpawnActions {
    posix_spawn_file_actions_t actions{};
    bool initialized = false;
    ~SpawnActions() {
        if (initialized) posix_spawn_file_actions_destroy(&actions);
    }
};

void drain(int fd, std::string &output) {
    std::array<char, 1024> buffer{};
    // Bound work per pass: even a noisy process cannot prevent timeout checks.
    for (int i = 0; i < 64; ++i) {
        const auto count = ::read(fd, buffer.data(), buffer.size());
        if (count > 0) {
            const auto room = output_limit - output.size();
            output.append(buffer.data(), std::min(room, static_cast<std::size_t>(count)));
        } else if (count < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
}
} // namespace

std::string system_executable(const char *name) {
    for (const auto *directory : {"/usr/bin/", "/bin/"}) {
        const std::string path = std::string(directory) + name;
        struct stat info{};
        if (::stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode) &&
            ::access(path.c_str(), X_OK) == 0)
            return path;
    }
    return {};
}

ProcessResult run_process(const std::string &executable, const std::vector<std::string> &arguments,
                          std::chrono::milliseconds timeout, Locale locale) {
    ProcessResult result;
    if (executable.empty() || executable.front() != '/' ||
        timeout <= std::chrono::milliseconds::zero()) {
        result.error = text(locale, Text::process_invalid_parameters);
        return result;
    }
    int descriptors[2]{};
    if (::pipe2(descriptors, O_CLOEXEC) != 0) {
        result.error = std::strerror(errno);
        return result;
    }
    Pipe pipe{descriptors[0], descriptors[1]};
    // POSIX file-action descriptors must not alias stdin/stdout/stderr. This also
    // works when the GUI was started with some standard streams already closed.
    for (int *fd : {&pipe.read, &pipe.write}) {
        if (*fd <= STDERR_FILENO) {
            const int copy = ::fcntl(*fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
            if (copy < 0) {
                result.error = std::strerror(errno);
                return result;
            }
            ::close(*fd);
            *fd = copy;
        }
    }
    if (::fcntl(pipe.read, F_SETFL, O_NONBLOCK) < 0) {
        result.error = std::strerror(errno);
        return result;
    }
    SpawnActions actions;
    int error = posix_spawn_file_actions_init(&actions.actions);
    if (error != 0) {
        result.error = std::strerror(error);
        return result;
    }
    actions.initialized = true;
    for (int target : {STDOUT_FILENO, STDERR_FILENO}) {
        error = posix_spawn_file_actions_adddup2(&actions.actions, pipe.write, target);
        if (error != 0) {
            result.error = std::strerror(error);
            return result;
        }
    }
    error = posix_spawn_file_actions_addclose(&actions.actions, pipe.read);
    if (error == 0) error = posix_spawn_file_actions_addclose(&actions.actions, pipe.write);
    if (error == 0)
        error = posix_spawn_file_actions_addopen(&actions.actions, STDIN_FILENO, "/dev/null",
                                                 O_RDONLY, 0);
    if (error != 0) {
        result.error = std::strerror(error);
        return result;
    }

    std::vector<char *> argv;
    argv.reserve(arguments.size() + 2);
    argv.push_back(const_cast<char *>(executable.c_str()));
    for (const auto &argument : arguments)
        argv.push_back(const_cast<char *>(argument.c_str()));
    argv.push_back(nullptr);

    pid_t pid = -1;
    error = posix_spawn(&pid, executable.c_str(), &actions.actions, nullptr, argv.data(), environ);
    if (error != 0) {
        result.error = std::strerror(error);
        return result;
    }
    ::close(pipe.write);
    pipe.write = -1;
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    while (true) {
        drain(pipe.read, result.output);
        const auto waited = ::waitpid(pid, &status, WNOHANG);
        if (waited == pid) break;
        if (waited < 0 && errno != EINTR) {
            result.error = std::strerror(errno);
            return result;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            result.timed_out = true;
            // Reap the one process we created; never kill by name or PID guessed
            // from another source. System power requests are not retried.
            ::kill(pid, SIGKILL);
            while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
            }
            break;
        }
        pollfd poll_descriptor{pipe.read, POLLIN, 0};
        if (::poll(&poll_descriptor, 1, 25) > 0 &&
            (poll_descriptor.revents & (POLLHUP | POLLERR)) != 0) {
            ::poll(nullptr, 0, 25);
        }
    }
    drain(pipe.read, result.output);
    if (WIFEXITED(status))
        result.exit_code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        result.exit_code = 128 + WTERMSIG(status);
    return result;
}
} // namespace power_timer
