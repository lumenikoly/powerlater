use crate::localization::Locale;
use crate::timer::Action;
use std::ffi::c_void;

type OSStatus = i32;
type AEEventClass = u32;
type AEEventID = u32;
#[repr(C)]
struct AEDesc {
    descriptor_type: u32,
    data_handle: *mut c_void,
}
#[repr(C)]
struct ProcessSerialNumber {
    high: u32,
    low: u32,
}

const TYPE_PROCESS_SERIAL_NUMBER: u32 = u32::from_be_bytes(*b"psn ");
const CORE_EVENT_CLASS: u32 = u32::from_be_bytes(*b"aevt");
const AE_SLEEP: u32 = u32::from_be_bytes(*b"slep");
const AE_SHUT_DOWN: u32 = u32::from_be_bytes(*b"shut");
const SYSTEM_PROCESS: u32 = 1;

unsafe extern "C" {
    fn AECreateDesc(kind: u32, data: *const c_void, size: isize, result: *mut AEDesc) -> OSStatus;
    fn AEDisposeDesc(desc: *mut AEDesc) -> OSStatus;
    fn AEDeterminePermissionToAutomateTarget(
        target: *const AEDesc,
        event_class: AEEventClass,
        event_id: AEEventID,
        ask_user: bool,
    ) -> OSStatus;
    fn AECreateAppleEvent(
        event_class: AEEventClass,
        event_id: AEEventID,
        target: *const AEDesc,
        return_id: i16,
        transaction_id: i32,
        result: *mut AEDesc,
    ) -> OSStatus;
    fn AESendMessage(event: *const AEDesc, reply: *mut AEDesc, mode: i32, timeout: i32)
    -> OSStatus;
}

fn event(action: Action) -> u32 {
    if action == Action::Sleep {
        AE_SLEEP
    } else {
        AE_SHUT_DOWN
    }
}
fn error(status: OSStatus, locale: Locale) -> String {
    match locale {
        Locale::En => format!("macOS rejected the request. Code: {status}"),
        Locale::Ru => format!("macOS отклонила запрос. Код: {status}"),
    }
}
fn address() -> Result<AEDesc, OSStatus> {
    let psn = ProcessSerialNumber {
        high: 0,
        low: SYSTEM_PROCESS,
    };
    let mut desc = AEDesc {
        descriptor_type: 0,
        data_handle: std::ptr::null_mut(),
    };
    let status = unsafe {
        AECreateDesc(
            TYPE_PROCESS_SERIAL_NUMBER,
            (&psn as *const ProcessSerialNumber).cast(),
            std::mem::size_of_val(&psn) as isize,
            &mut desc,
        )
    };
    if status == 0 { Ok(desc) } else { Err(status) }
}
pub fn check(action: Action, locale: Locale) -> Result<(), String> {
    let mut target = address().map_err(|status| error(status, locale))?;
    let status = unsafe {
        AEDeterminePermissionToAutomateTarget(&target, CORE_EVENT_CLASS, event(action), true)
    };
    unsafe {
        AEDisposeDesc(&mut target);
    }
    if status == 0 {
        Ok(())
    } else {
        Err(error(status, locale))
    }
}
pub fn execute(action: Action, locale: Locale) -> Result<(), String> {
    let mut target = address().map_err(|status| error(status, locale))?;
    let mut event_desc = AEDesc {
        descriptor_type: 0,
        data_handle: std::ptr::null_mut(),
    };
    let mut reply = AEDesc {
        descriptor_type: 0,
        data_handle: std::ptr::null_mut(),
    };
    let mut status = unsafe {
        AEDeterminePermissionToAutomateTarget(&target, CORE_EVENT_CLASS, event(action), false)
    };
    if status == 0 {
        status = unsafe {
            AECreateAppleEvent(
                CORE_EVENT_CLASS,
                event(action),
                &target,
                -1,
                0,
                &mut event_desc,
            )
        };
    }
    if status == 0 {
        status = unsafe { AESendMessage(&event_desc, &mut reply, 0x00000003 | 0x00000010, 600) };
    }
    unsafe {
        AEDisposeDesc(&mut reply);
        AEDisposeDesc(&mut event_desc);
        AEDisposeDesc(&mut target);
    }
    if status == 0 {
        Ok(())
    } else {
        Err(error(status, locale))
    }
}
