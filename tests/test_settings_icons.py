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
