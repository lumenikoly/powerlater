#include "activity.hpp"
#import <Foundation/Foundation.h>

namespace power_timer {
void *begin_timer_activity() {
    id token = [[NSProcessInfo processInfo]
        beginActivityWithOptions:NSActivityUserInitiatedAllowingIdleSystemSleep
                          reason:@"Countdown explicitly started by the user"];
    return (__bridge_retained void *)token;
}
void end_timer_activity(void *pointer) {
    id token = (__bridge_transfer id)pointer;
    [[NSProcessInfo processInfo] endActivity:token];
}
} // namespace power_timer
