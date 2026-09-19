use crate::localization::Locale;
use crate::timer::Action;
use windows::Win32::Foundation::{CloseHandle, GetLastError, HANDLE, SetLastError, WIN32_ERROR};
use windows::Win32::Security::{
    AdjustTokenPrivileges, LUID_AND_ATTRIBUTES, LookupPrivilegeValueW, SE_PRIVILEGE_ENABLED,
    TOKEN_ADJUST_PRIVILEGES, TOKEN_PRIVILEGES, TOKEN_QUERY,
};
use windows::Win32::System::Power::{
    GetPwrCapabilities, SYSTEM_POWER_CAPABILITIES, SetSuspendState,
};
use windows::Win32::System::Shutdown::{
    EWX_POWEROFF, ExitWindowsEx, SHTDN_REASON_FLAG_PLANNED, SHTDN_REASON_MAJOR_APPLICATION,
    SHTDN_REASON_MINOR_OTHER, SHUTDOWN_REASON,
};
use windows::Win32::System::Threading::{GetCurrentProcess, OpenProcessToken};
use windows::core::w;

struct Privilege(HANDLE, TOKEN_PRIVILEGES);

impl Privilege {
    fn acquire() -> Result<Self, String> {
        unsafe {
            let mut token = HANDLE::default();
            OpenProcessToken(
                GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &mut token,
            )
            .map_err(|error| error.to_string())?;
            let mut luid = Default::default();
            if let Err(error) = LookupPrivilegeValueW(None, w!("SeShutdownPrivilege"), &mut luid) {
                let _ = CloseHandle(token);
                return Err(error.to_string());
            }
            let requested = TOKEN_PRIVILEGES {
                PrivilegeCount: 1,
                Privileges: [LUID_AND_ATTRIBUTES {
                    Luid: luid,
                    Attributes: SE_PRIVILEGE_ENABLED,
                }],
            };
            let mut previous = TOKEN_PRIVILEGES::default();
            let mut size = std::mem::size_of::<TOKEN_PRIVILEGES>() as u32;
            SetLastError(WIN32_ERROR(0));
            if let Err(error) = AdjustTokenPrivileges(
                token,
                false,
                Some(&requested),
                size,
                Some(&mut previous),
                Some(&mut size),
            ) {
                let _ = CloseHandle(token);
                return Err(error.to_string());
            }
            let last_error: WIN32_ERROR = GetLastError();
            if last_error.0 != 0 {
                let _ = CloseHandle(token);
                return Err(format!("Windows error code: {}", last_error.0));
            }
            Ok(Self(token, previous))
        }
    }
}

impl Drop for Privilege {
    fn drop(&mut self) {
        unsafe {
            let _ = AdjustTokenPrivileges(self.0, false, Some(&self.1), 0, None, None);
            let _ = CloseHandle(self.0);
        }
    }
}

pub fn check(action: Action, _locale: Locale) -> Result<(), String> {
    let _privilege = Privilege::acquire()?;
    if action == Action::Sleep {
        let mut capabilities = SYSTEM_POWER_CAPABILITIES::default();
        if !unsafe { GetPwrCapabilities(&mut capabilities) } {
            return Err(windows::core::Error::from_thread().to_string());
        }
        if !capabilities.SystemS1
            && !capabilities.SystemS2
            && !capabilities.SystemS3
            && !capabilities.AoAc
        {
            return Err("Sleep is not available on this computer.".into());
        }
    }
    Ok(())
}

pub fn execute(action: Action, locale: Locale) -> Result<(), String> {
    let _privilege = Privilege::acquire()?;
    unsafe {
        if action == Action::Sleep {
            if !SetSuspendState(false, false, false) {
                return Err(windows::core::Error::from_thread().to_string());
            }
        } else {
            let reason = SHUTDOWN_REASON(
                SHTDN_REASON_MAJOR_APPLICATION.0
                    | SHTDN_REASON_MINOR_OTHER.0
                    | SHTDN_REASON_FLAG_PLANNED.0,
            );
            ExitWindowsEx(EWX_POWEROFF, reason).map_err(|error| error.to_string())?;
        }
    }
    let _ = locale;
    Ok(())
}
