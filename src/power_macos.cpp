#include "power.hpp"

#include <Carbon/Carbon.h>
#include <cstring>

namespace power_timer {
namespace {
class Descriptor {
  public:
    AEDesc value{typeNull, nullptr};
    ~Descriptor() { AEDisposeDesc(&value); }
    Descriptor() = default;
    Descriptor(const Descriptor &) = delete;
    Descriptor &operator=(const Descriptor &) = delete;
};

AEEventID event_id(Action action) { return action == Action::sleep ? kAESleep : kAEShutDown; }

OSStatus system_address(Descriptor &address) {
    // Public Apple Events API, addressed to the interactive loginwindow process.
    // No shell, sudo, privileged helper or private NSWorkspace selectors.
    const ProcessSerialNumber system_process{0, kSystemProcess};
    return AECreateDesc(typeProcessSerialNumber, &system_process, sizeof(system_process),
                        &address.value);
}

Result mac_error(OSStatus status, Locale locale) {
    if (status == errAEEventNotPermitted || status == errAEEventWouldRequireUserConsent) {
        return Result::failure(std::string(text(locale, Text::mac_permission)) +
                               text(locale, Text::mac_permission_help) +
                               text(locale, Text::mac_restart) +
                               std::string(text(locale, Text::mac_code)) + std::to_string(status));
    }
    return Result::failure(std::string(text(locale, Text::mac_rejected)) + std::to_string(status));
}
} // namespace

Result check_power_action(Action action, Locale locale) {
    Descriptor address;
    auto status = system_address(address);
    if (status != noErr) return mac_error(status, locale);
    // Request consent when arming the timer, never for the first time at expiry.
    status = AEDeterminePermissionToAutomateTarget(&address.value, kCoreEventClass,
                                                   event_id(action), true);
    return status == noErr ? Result::success() : mac_error(status, locale);
}

Result execute_power_action(Action action, Locale locale) {
    Descriptor address;
    auto status = system_address(address);
    if (status != noErr) return mac_error(status, locale);
    status = AEDeterminePermissionToAutomateTarget(&address.value, kCoreEventClass,
                                                   event_id(action), false);
    if (status != noErr) return mac_error(status, locale);
    Descriptor event;
    Descriptor reply;
    status = AECreateAppleEvent(kCoreEventClass, event_id(action), &address.value,
                                kAutoGenerateReturnID, kAnyTransactionID, &event.value);
    if (status != noErr) return mac_error(status, locale);
    status = AESendMessage(&event.value, &reply.value, kAEWaitReply | kAENeverInteract, 10 * 60);
    if (status == errAETimeout) {
        return Result::failure(std::string(text(locale, Text::mac_timeout)) +
                               text(locale, Text::mac_timeout_followup));
    }
    if (status != noErr) return mac_error(status, locale);
    // Delivery can succeed while the receiving application returns an error.
    SInt32 remote_error = 0;
    const auto extracted = AEGetParamPtr(&reply.value, keyErrorNumber, typeSInt32, nullptr,
                                         &remote_error, sizeof(remote_error), nullptr);
    if (extracted == noErr && remote_error != 0) return mac_error(remote_error, locale);
    return Result::success();
}
} // namespace power_timer
