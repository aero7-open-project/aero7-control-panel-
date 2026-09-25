# Aero7 Optional Features

Open **Start > Turn Aero7 features on or off**, or open **Control Panel >
Programs > Programs and Features > Turn Aero7 features on or off**. A checked
box means the feature's real packages and required services are installed.
Changes require administrator approval. Removing a feature does not delete its
user-created files or settings.

The **Show KDE System Settings application** switch is off by default. Turn it
on here if you need the full KDE settings window in application menus and
search. Aero7 Control Panel remains available either way.

## Feature reference

| Feature | What it does | Components and important behavior |
| --- | --- | --- |
| Aero7 Desktop Core | Provides the Aero7 session, shell, Control Panel and File Explorer. | Required on every Aero7 installation. It cannot be disabled from Optional Features. |
| Programs Center Beta | Adds Aero7's graphical application browser for installing, removing and updating software. | Optional beta software. Fresh installations keep a checksum-verified package in the local optional-package cache, so it can be enabled without internet. It can be removed again without removing Aero7. |
| Parental Controls | Restricts which applications managed local accounts can use. | Installs `malcontent`. Sign out after enabling it so the account restrictions are loaded by a new session. |
| Backup and Restore | Creates scheduled backups and restores personal files. | Installs Déjà Dup. Backup archives and user configuration remain when the feature is removed. |
| System Recovery | Creates and manages filesystem snapshots. | Installs Snapper and Btrfs Assistant and is available only when the system root uses Btrfs. Existing snapshots remain after removal. |
| Advanced Accessibility Services | Adds spoken desktop feedback and screen reading. | Installs Orca with Speech Dispatcher and eSpeak NG. Sign out after enabling it. This is speech output, not speech recognition. |
| Speech Recognition | Would provide voice control and dictation. | Currently unavailable because Aero7 has no supported recognition backend in its configured repositories. No substitute package is falsely enabled. |
| Sync Center | Synchronizes selected folders between trusted computers. | Installs Syncthing, starts its user service and opens the local web interface at `127.0.0.1:8384`. Synced data and configuration remain after removal. |
| Aero7 Defender | Provides on-demand malware scans and signature updates shown through Action Center. | Installs ClamAV and enables its signature-update service. Signature data remains after removal. |
| Windows CardSpace | Historical identity-card technology from Windows. | Hidden and unavailable. CardSpace was discontinued and Aero7 does not provide a fake replacement. |
| Remote Desktop Connections | Connects to computers that offer Remote Desktop Protocol access. | Installs FreeRDP. This is a client only; enabling it does not allow remote logins to this computer. |
| File and Printer Sharing | Shares deliberately selected files and printers with SMB-compatible computers. | Installs Samba and enables SMB services. Aero7 never creates a share automatically, and existing share configuration remains after removal. |
| Mobile Broadband and Modem Support | Manages compatible cellular modems and mobile-data connections. | Installs ModemManager and enables its service. The entry reports **Hardware not present** when no compatible modem is detected. |
| Color Management | Manages device ICC color profiles and color-corrected workflows. | Installs colord. Sign out after enabling it; installed ICC profiles remain when it is removed. |

## Statuses

- **Checked** — all required packages are installed and services are active.
- **Unchecked** — the feature is available but not installed.
- **Partially installed** — a package or service is missing; select it to repair.
- **Hardware not present** — the feature needs hardware Aero7 cannot detect.
- **Unavailable** — a safe backend or its verified package is not available.
- **Restart required** — installation succeeded and must be completed by a restart.

Repository features need a working network connection when they are first
enabled. Programs Center Beta is the exception on Beta 2 media: its verified
package is retained locally specifically so the feature remains installable
offline.
