# Dependencies and technical references

Power Timer is based in part on the work of the FLTK project (https://www.fltk.org).

## FLTK

Version: 1.4.5. FLTK is statically linked when building the application. No FLTK source or binaries are included in this source archive.

License: GNU Library General Public License, version 2, with FLTK's exceptions for static linking.

- License and exceptions: https://www.fltk.org/COPYING.php
- Release: https://github.com/fltk/fltk/releases/tag/release-1.4.5
- CMake instructions: https://github.com/fltk/fltk/blob/release-1.4.5/README.CMake.txt
- API documentation: https://www.fltk.org/doc-1.4/
- Upstream release metadata with asset SHA-256: https://api.github.com/repos/fltk/fltk/releases/tags/release-1.4.5

## Platform API documentation

Windows:

- https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-exitwindowsex
- https://learn.microsoft.com/en-us/windows/win32/api/powrprof/nf-powrprof-setsuspendstate
- https://learn.microsoft.com/en-us/windows/win32/api/securitybaseapi/nf-securitybaseapi-adjusttokenprivileges

macOS:

- Apple Technical Q&A QA1134, “Programmatically causing restart, shutdown and/or logout” (archived; not retrievable in this preparation environment): https://developer.apple.com/library/archive/qa/qa1134/_index.html
- Apple Events: https://developer.apple.com/documentation/applicationservices/apple_events
- Process activity: https://developer.apple.com/documentation/foundation/processinfo/activityoptions/userinitiatedallowingidlesystemsleep

Linux:

- systemctl source documentation: https://github.com/systemd/systemd/blob/main/man/systemctl.xml
- logind source documentation: https://github.com/systemd/systemd/blob/main/man/org.freedesktop.login1.xml
- Inhibitor locks: https://systemd.io/INHIBITOR_LOCKS/

The source code uses public operating-system interfaces. None of the non-destructive automated tests invokes a sleep or power-off operation.
