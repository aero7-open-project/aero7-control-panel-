# Control Panel QA — 2026-09-30

This is a testing-branch audit, not release acceptance. It separates entry-point
checks from proof that a setting changes its system backend.

| Area | Checked | Result |
| --- | --- | --- |
| Search/catalog entries | 72 settings and 13 optional-feature entries | All 85 have exact, bundled AeroThemePlasma icon-pack assets; `settings-icons-test` enforces this. |
| Setting entry points | 70 in-app routes | Offscreen startup smoke: no crashes or application errors. The remaining two entries launch Window Color and Programs Center; their executables and the Window Color plugin are present on the host. |
| Native editors | 44 advanced settings | Every catalogued editor opens with at least one field; the generic placeholder was removed. A failed Apply now keeps the dialog open. |
| Personalization | Aero theme, background, Window Color, Screen Saver | Only one visually distinct Aero theme is shown. Background opens without scanning or decoding on the UI thread; requested icons were visually checked in a disposable Aero7 VM. |
| Navigation | Back, forward and history menu | Circular packed arrows and a functional location dropdown are present; the VM visual matches the Windows 7 reference closely. |
| Automated tests | 15 CTests | All pass. The background-open timing test passed five additional consecutive runs. |

## VM evidence

The installed Aero7 Beta 2 acceptance image was booted with QEMU's temporary
snapshot layer. The updated Control Panel binary was copied to `/tmp` inside
that guest and launched in its Plasma session. Personalization and Desktop
Background were opened and visually inspected. The gallery displayed nine
wallpapers. The guest application log contained only MESA-EGL software-rendering
warnings, not Control Panel errors. The guest's base disk was not changed.

## Remaining functional acceptance

The route/editor smoke checks do **not** prove that all 72 settings persist and
change the running Plasma/KWin services. Each advanced editor needs a
save/readback/backend-response check in a disposable VM, particularly hardware
and network controls. Optional-feature install/removal also needs separate
end-to-end coverage. No change here should be called a complete settings audit
until those checks pass.

The Plasma wallpaper command currently accepts Fill, Fit, Stretch and Center
(`pad`). It rejects `tile`, so the previously broken Tile option is not shown
until there is a supported backend implementation.
