#!/usr/bin/env python3
"""Every Control Panel setting must use an icon bundled from the Aero7 pack."""

import json
from pathlib import Path
import subprocess
import sys


def main() -> int:
    control, pack = Path(sys.argv[1]), Path(sys.argv[2])
    result = subprocess.run(
        [str(control), "--list-settings-json"],
        check=True,
        capture_output=True,
        text=True,
    )
    settings = json.loads(result.stdout)
    catalog = [entry for entry in settings if "optionalFeature" not in entry]
    bridges = [entry for entry in catalog
               if entry.get("backend") == "KDE settings module (temporary)"]
    if len(catalog) != 72 or len(bridges) != 44:
        print(f"Unexpected catalog or bridge count: {len(catalog)} / {len(bridges)}")
        return 2
    if any(entry.get("status") != "KDE compatibility backend"
           for entry in bridges):
        print("A KDE bridge is incorrectly labeled as an Aero7 native editor")
        return 3
    missing = [
        f"{setting['key']}: {setting['icon']}"
        for setting in settings
        if not any(pack.glob(f"*/{setting['icon']}.png"))
    ]
    if missing:
        print("Settings without exact bundled pack icons:\n" + "\n".join(missing))
        return 1
    print(f"Checked {len(settings)} search entries and their bundled icons")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
