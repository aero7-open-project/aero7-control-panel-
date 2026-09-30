# Control Panel QA — 2026-09-30

This is a testing-branch audit, not release acceptance. It separates entry-point
checks from proof that a setting changes its system backend.

| Area | Checked | Result |
| --- | --- | --- |
| Search/catalog entries | 72 settings and 13 optional-feature entries | All 85 have exact, bundled AeroThemePlasma icon-pack assets; `settings-icons-test` enforces this. All 45 All Control Panel Items icons also have exact bundled assets; Devices and Printers and Sync Center use more appropriate icons from the same pack. |
| Setting entry points | 70 in-app routes | Offscreen startup smoke: no crashes or application errors. The remaining two entries launch Window Color and Programs Center; their executables and the Window Color plugin are present on the host. |
| Advanced settings | 49 temporary KDE bridges | The misleading Aero7 property sheets and five category entries without their advertised control are no longer reachable. Each bridge checks for its real KDE module before launch, including search and Ease of Access links. All 49 mapped modules are present on the host and in the disposable Aero7 VM. |
| Personalization | Aero theme, background, Window Color, Screen Saver | Only one visually distinct Aero theme is shown. Background opens without scanning or decoding on the UI thread; requested icons were visually checked in a disposable Aero7 VM. Theme selection now waits for the color-scheme command to succeed before highlighting the swatch, and displays an error on failure. |
| Navigation | Back, forward and history menu | Circular packed arrows and a functional location dropdown are present; the VM visual matches the Windows 7 reference closely. |
| Automated tests | 19 CTests | All pass. The background-open timing test passed five additional consecutive runs. New tests cover theme-apply failure without a false selection, Folder Options save/indexer responses, Taskbar/Start Menu save/readback failures, and Window Snapping helper/readback failures. |

## VM evidence

The installed Aero7 Beta 2 acceptance image was booted with QEMU's temporary
snapshot layer. The updated Control Panel binary was copied to `/tmp` inside
that guest and launched in its Plasma session. Personalization and Desktop
Background were opened and visually inspected. The gallery displayed nine
wallpapers. The guest application log contained only MESA-EGL software-rendering
warnings, not Control Panel errors. The guest's base disk was not changed.

The rebuilt binary was also copied into a fresh snapshot of that VM. All 49
temporary KDE module IDs were compared with the guest's installed module list.
Launching `--setting network-connections` opened the working Wi-Fi & Networking
editor under the Aero7 window frame, titled **Change Adapter Settings**. Its
connection, IPv4 and IPv6 controls were visible. No network configuration was
changed. The VM printed MESA software-rendering warnings, but the editor
remained usable. The guest's snapshot was disposable.

After five additional misleading catalog entries were bridged, a second
snapshot confirmed their modules were present and `--setting file-search`
launched without a Control Panel error. The VM terminated before a screenshot
of that particular module could be captured, so visual verification of those
five module windows remains pending.

A fresh disposable VM snapshot also ran `taskbar-start-menu-apply-test` against
its real Plasma session, with the real `qdbus6` and Aero7 panel widgets. The
page loaded the installed taskbar/Start values, applied them without changing
the selected values, and read back all requested fields; the integration check
exited 0. The same check passed on the development host. This verifies a
no-op apply/readback path, not every individual visual behavior of the controls.

The same VM image does not contain `/usr/lib/aero7-desktop/aero7-snap-control`,
although the current Aero7 Desktop source installs that helper. Window Snapping
now shows an unavailable state and disables its slider when the helper cannot
load. After temporarily copying the current helper into the disposable guest,
the page changed sensitivity by one step, read it back, and restored the
original value successfully (`window-snapping-apply-test` exited 0). A future
installed-package/ISO verification must check that the helper is actually
included at its expected path; the temporary copy does not prove packaging.

## Remaining functional acceptance

The route smoke checks do **not** prove that all 72 settings persist and
change the running Plasma/KWin services. The 49 advanced or incomplete entries now open
KDE's working modules temporarily instead of the unverified Aero7 editors.
Each native Aero7 page still needs a save/readback/backend-response check in
a disposable VM, particularly hardware and network controls. Optional-feature
install/removal also needs separate end-to-end coverage. No change here should
be called a complete settings audit until those checks pass.

Folder Options now reports whether its configuration files were saved and
whether the Baloo indexer actually accepted the requested state. It no longer
reports blanket success when the helper is missing or exits with an error.
Taskbar and Start Menu now refuse to claim success when the Aero7 panel,
SevenTasks, or SevenStart component is absent, when Plasma rejects the command,
or when its readback does not match the requested values.
Window Snapping now refuses to claim success unless its helper rereads the
requested sensitivity. The installed VM image's missing helper remains a
packaging acceptance gap.

The Plasma wallpaper command currently accepts Fill, Fit, Stretch and Center
(`pad`). It rejects `tile`, so the previously broken Tile option is not shown
until there is a supported backend implementation.
