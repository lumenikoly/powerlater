#include "power.hpp"

#include <powrprof.h>
#include <reason.h>
#include <windows.h>

namespace power_timer {
namespace {
std::string windows_error(DWORD code, Locale locale) {
    wchar_t *message = nullptr;
    const DWORD count = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<wchar_t *>(&message), 0, nullptr);
    if (count == 0) return std::string(text(locale, Text::windows_code)) + std::to_string(code);
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, message, static_cast<int>(count), nullptr, 0,
                                          nullptr, nullptr);
    std::string text(static_cast<std::size_t>(bytes), '\0');
    if (bytes > 0)
        WideCharToMultiByte(CP_UTF8, 0, message, static_cast<int>(count), text.data(), bytes,
                            nullptr, nullptr);
    LocalFree(message);
    return text + " (" + std::to_string(code) + ")";
}

class ShutdownPrivilege {
  public:
    ShutdownPrivilege() {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                              &token_)) {
            error_ = GetLastError();
            return;
        }
        TOKEN_PRIVILEGES requested{};
        requested.PrivilegeCount = 1;
        if (!LookupPrivilegeValueW(nullptr, SE_SHUTDOWN_NAME, &requested.Privileges[0].Luid)) {
            error_ = GetLastError();
            return;
        }
        requested.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        DWORD previous_size = sizeof(previous_);
        SetLastError(ERROR_SUCCESS);
        const BOOL adjusted = AdjustTokenPrivileges(token_, FALSE, &requested, sizeof(previous_),
                                                    &previous_, &previous_size);
        error_ = GetLastError(); // Success return can still mean NOT_ALL_ASSIGNED.
        enabled_ = adjusted && error_ == ERROR_SUCCESS;
        if (!adjusted && error_ == ERROR_SUCCESS) error_ = ERROR_PRIVILEGE_NOT_HELD;
    }
    ~ShutdownPrivilege() {
        if (enabled_) AdjustTokenPrivileges(token_, FALSE, &previous_, 0, nullptr, nullptr);
        if (token_) CloseHandle(token_);
    }
    ShutdownPrivilege(const ShutdownPrivilege &) = delete;
    ShutdownPrivilege &operator=(const ShutdownPrivilege &) = delete;
    bool enabled() const { return enabled_; }
    DWORD error() const { return error_; }

  private:
    HANDLE token_ = nullptr;
    TOKEN_PRIVILEGES previous_{};
    DWORD error_ = ERROR_SUCCESS;
    bool enabled_ = false;
};
} // namespace

Result check_power_action(Action action, Locale locale) {
    ShutdownPrivilege privilege;
    if (!privilege.enabled()) {
        return Result::failure(std::string(text(locale, Text::permission_error)) +
                               windows_error(privilege.error(), locale));
    }
    if (action == Action::sleep) {
        SYSTEM_POWER_CAPABILITIES capabilities{};
        if (!GetPwrCapabilities(&capabilities))
            return Result::failure(windows_error(GetLastError(), locale));
        if (!capabilities.SystemS1 && !capabilities.SystemS2 && !capabilities.SystemS3 &&
            !capabilities.AoAc) {
            return Result::failure(text(locale, Text::sleep_unavailable));
        }
    }
    return Result::success();
}

Result execute_power_action(Action action, Locale locale) {
    ShutdownPrivilege privilege;
    if (!privilege.enabled()) return Result::failure(windows_error(privilege.error(), locale));
    if (action == Action::sleep) {
        // FALSE means sleep, not hibernation. Keep normal wake sources enabled.
        if (!SetSuspendState(FALSE, FALSE, FALSE))
            return Result::failure(windows_error(GetLastError(), locale));
    } else {
        // Never add EWX_FORCE or EWX_FORCEIFHUNG: allow applications to object.
        if (!ExitWindowsEx(EWX_POWEROFF, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_OTHER |
                                             SHTDN_REASON_FLAG_PLANNED))
            return Result::failure(windows_error(GetLastError(), locale));
    }
    return Result::success(); // Accepted, not proof that power state changed.
}
} // namespace power_timer
