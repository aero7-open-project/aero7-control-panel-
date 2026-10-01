# Control Panel QA — 2026-09-30

This is a testing-branch audit, not release acceptance. It separates entry-point
checks from proof that a setting changes its system backend.

| Area | Checked | Result |
| --- | --- | --- |
| Search/catalog entries | 72 settings and 13 optional-feature entries | All 85 have exact, bundled AeroThemePlasma icon-pack assets; `settings-icons-test` enforces this. All 45 All Control Panel Items icons also have exact bundled assets; Devices and Printers and Sync Center use more appropriate icons from the same pack. The Internet Explorer card now uses the pack's matching browser icon rather than an absent icon name. |
| Setting entry points | 70 in-app routes | Offscreen startup smoke: no crashes or application errors. The remaining two entries launch Window Color and Programs Center; their executables and the Window Color plugin are present on the host. |
| Advanced settings | 49 temporary KDE bridges | The misleading Aero7 property sheets and five category entries without their advertised control are no longer reachable. Each bridge checks for its real KDE module before launch, including search and Ease of Access links. All 49 mapped modules are present on the host and in the disposable Aero7 VM. |
| Personalization | Aero theme, background, Window Color, Screen Saver | Only one visually distinct Aero theme is shown. Background opens without scanning or decoding on the UI thread; requested icons were visually checked in a disposable Aero7 VM. Theme selection now waits for the color-scheme command to succeed before highlighting the swatch, and displays an error on failure. |
| Navigation | Back, forward and history menu | Circular packed arrows and a functional location dropdown are present; the VM visual matches the Windows 7 reference closely. |
| Display / Screen Resolution | Layout drag, per-monitor edits, and backend rollback | Dragging a monitor enables Apply only when its position changes. Resolution, orientation, scale, and primary-display changes are retained per monitor. Rollback uses a separate pre-edit snapshot, and backend failures produce an error. Controlled tests cover drag, original-coordinate rollback, failure handling, and edits retained across monitor selection. A disposable graphical VM also passed real resolution reject, 15-second timeout, keep, and restore checks. Real multi-monitor arrangement, scaling, and rotation still need acceptance. |
| Automated tests | 24 CTests | All pass. The background-open timing test passed five additional consecutive runs. New tests cover theme-apply failure without a false selection, Folder Options save/indexer responses, Taskbar/Start Menu save/readback failures, Window Snapping helper/readback failures, Power Options' unavailable-service state and advanced-link routing, firewall-backend selection and module routing, Linux Update command failures, and User Accounts module routing. |

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

Power Options was checked against a VM with Balanced and Power Saver profiles.
The first SSH-launched test was correctly rejected by Polkit as an inactive
remote session; no profile changed. The same test then launched from the
graphical Aero7 session, switched to Power Saver, confirmed the service's
readback, restored Balanced, and exited 0. The updated Power Options page was
opened visually in the VM. Its advanced plan links now target the installed KDE
Power Management module, while the wake-password link targets Lock Screen.
The information link now explains the plans instead of doing nothing.

Default Programs was checked in the same VM. The base image had no compatible
browser, so Falkon was installed only in the separate QA overlay. The updated
page then saved Falkon as the Internet Explorer backend and the installed
launcher read back `org.kde.falkon.desktop`; the test exited 0. Controlled
launcher cases in that VM also verified that ignored writes are rejected,
malformed status disables Save, policy locks disable edits, and the bundled
Internet Explorer icon loads without a missing-icon warning. The test log had
no Qt warnings after supplying the normal sidebar scaffold. Switching the
actual system-wide default associations off and back on remains a separate
acceptance check; the VM run kept the pre-existing default-association state.

The fresh-install VM has firewalld but not UFW. The old Firewall and both
Action Center views read only `/etc/ufw/ufw.conf`, so they reported the wrong
backend or an off state. The shared backend detector now selects firewalld on
that VM and preserves an active UFW installation when present. The new backend
test passed both on the host and in the disposable VM (5 Qt assertions/groups,
0 failures); the updated Firewall page started offscreen in the VM without an
application error. The VM's firewalld service was temporarily stopped during
SSH-based QA, so these checks do **not** prove an actual authenticated on/off
click. That GUI/Polkit toggle and its service readback still need an active
desktop-session acceptance run. Firewalld-specific zone rule editing, logging,
and restore-defaults controls are visibly unavailable rather than issuing
UFW commands against the wrong backend. The tray Action Center now uses the
same detector as the Control Panel page.

Linux Update was audited with controlled fake `checkupdates`, `yay`, and
`pkexec` commands on the Aero7 development host. Missing tools, repository
errors, AUR errors, and installer start/exit failures now leave an error state
with details instead of claiming the computer is up to date or permanently
showing Downloading. Only a successful complete check records a successful
check timestamp. The test also confirmed that selecting only part of the repository
updates is stopped before pacman; Arch requires a full repository upgrade.
The unsafe fallback `pacman -Sy` check and automatic recursive deletion of
selected yay cache directories were removed. No real packages were installed
in this QA run, so full pacman and AUR installation still require VM
acceptance with disposable package sources and a graphical Polkit session.

User Accounts previously exposed native account-edit commands that had not
been verified end to end, including a create-account path that could claim a
password window opened when launch failed. All five account-edit links now
open the installed KDE Users module (`kcm_users`) temporarily; the Aero7
overview still reads the local account and avatar. A controlled fake-module
test verified the five links and the unavailable-module sidebar state without
creating, deleting, or modifying any real account. The real module is present
on the development host. The rebuilt Control Panel page and the real Users
module both remained running for five seconds offscreen with no error output.
Graphical appearance and account editing were still pending at that stage;
the October 1 checks below provide further coverage.

On October 1, the rebuilt Display page was tested against the real KScreen
service in a new disposable VM layer. `AERO7_DISPLAY_REAL_BACKEND=1
./display-page-test` changed Virtual-1 from 1920 × 1080 (mode 1) to
1280 × 768 (mode 22), verified the changed mode during the confirmation,
rejected it, and verified the original mode was restored. It then verified
the automatic 15-second rollback, a kept change, and a kept restoration to
the original mode. All four transitions passed and the test exited 0. The
actual Control Panel Screen Resolution page was visually inspected at
1920 × 1080. The VM exposed only one connected output, so this does not
prove the multi-monitor arrangement, scaling, or rotation paths.

The resolution changes exposed a SevenTasks `task.model` TypeError during
delegate reconstruction. The theme's geometry publisher now skips delegates
whose item or model is absent. After installing that same guard in the
disposable VM, all four resolution transitions passed again, and the journal
for that run contained no TypeError or ReferenceError. The desktop and
taskbar rendered normally after restoration. Display blanking during the
initial long setup was traced to the guest's idle DPMS/lock settings;
waking and unlocking the guest restored the visible desktop.

The User Accounts overview and the real KDE Users module were also opened
and visually inspected in that guest. AccountsService was active, and the
controlled account-routing test passed all four Qt test groups in the VM.
The module log contained MESA software-rendering warnings. After restarting
the same disposable layer, the real Users editor saved the current user's
display name as `Aero7 QA`; both `/etc/passwd` and the AccountsService
`RealName` property confirmed it. The daemon journal identified the request
as coming from `kcmshell6`.

Clearing that name and applying it in the KDE editor showed an empty name
in the UI but did not send a corresponding AccountsService update. The
backend still held `Aero7 QA`. Calling AccountsService's `SetRealName` with
an empty string restored the guest's original value, and both readbacks
confirmed the restoration. **Clearing a display name through the temporary
KDE Users editor remains a known defect.** Account creation/removal,
password changes, and privileged account-type changes also remain VM
acceptance items.

## Remaining functional acceptance

### October 1 firewall follow-up

Installed `plasma-firewall` 6.7.5-1 only in the disposable VM overlay. Its
`kcm_firewall` module was detected and opened under Aero7's window decoration.
Launching it through SSH initially produced authorization errors and an
incorrect disabled display; launching from the active desktop terminal read
the active firewalld backend, Reject/Allow policies, and existing rules.
KDE's editor emitted QML ReferenceError/binding warnings and protocol warnings;
this temporary bridge must not be described as error-free or a native Aero7
rule editor. Rule creation/removal and log-view acceptance remain pending.

The rebuilt native page was then opened from that same graphical session.
Its firewalld rule/log buttons were enabled, Advanced settings was reachable,
and Restore defaults stayed unavailable. The new controlled routing test
passed on the host and in the VM (four Qt test groups). All 24 CTests passed
on the host. Missing-module cases keep those editor actions disabled with a
`plasma-firewall` explanation; the native service toggle remains independent.

The actual native firewall toggle was clicked in the VM. Cancelling UAC
produced the expected cancellation warning and left the service active.
An authenticated off action showed success and independently read back
`inactive`/`disabled` from systemctl. After acknowledging the dialog, the
page refreshed to Off. An authenticated on action showed success following
the page's service-state readback, restoring the original enabled firewall.
After QA SSH access was restored, independent `systemctl is-active` and
`systemctl is-enabled` checks confirmed `active` and `enabled`. The daemon
journal recorded both graphical-session pkexec requests and the stop/start.
The first password attempts occurred before focus settled and triggered the
test account's PAM lock; clearing only that disposable account's tally and
waiting for the prompt made authentication succeed. No host account was
changed. Restarting firewalld discarded the overlay's runtime-only SSH
allowance, so QA access required restoring that allowance separately.

### Remaining checks

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
Power Options now waits for the power service's response and confirms the
active profile before claiming success. An unavailable service leaves the
fallback plan disabled rather than offering a no-op control.
Default Programs now rereads browser and default-association status after Save
and refuses to report success if the launcher did not retain either choice.
Firewall status now follows the installed active backend. The firewalld
on/off action waits for `systemctl` and verifies the resulting service state
before claiming success; the October 1 graphical VM follow-up verified
cancellation, authenticated off, and authenticated on. The existing UFW toggle
likewise rereads `ufw.conf` before claiming success, but its actual privileged
toggle still needs VM acceptance. Firewalld rules/logs now use the checked KDE
Firewall module temporarily; actual rule changes and log views remain pending.
Linux Update now requires `checkupdates` from pacman-contrib for a safe
repository check. If it is absent or fails, the page reports an error rather
than presenting stale package data as a successful check. Selective repository
updates are blocked to avoid an unsupported partial Arch upgrade; selected
AUR updates remain optional. The Change settings sidebar entry and actual
package-install path remain incomplete acceptance items.
User Accounts currently hands edits to the KDE Users module as the approved
temporary backend. Its native replacement cannot be called complete until
privileged account changes, errors, and readback are safely tested in a VM.

The Plasma wallpaper command currently accepts Fill, Fit, Stretch and Center
(`pad`). It rejects `tile`, so the previously broken Tile option is not shown
until there is a supported backend implementation.
