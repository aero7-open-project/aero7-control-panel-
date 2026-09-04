#!/usr/bin/env python3
"""Allow theme icons only for external application identities."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ALLOWLIST = {
    "src/ui/pages/ProgramsFeaturesPage.cpp": ("p.iconName", "p.name"),
    "src/ui/pages/DefaultProgramsPage.cpp": ("backend.value",),
}


def main() -> int:
    errors: list[str] = []
    for path in (ROOT / "src").rglob("*"):
        if path.suffix not in {".cpp", ".h", ".qml", ".ui"}:
            continue
        rel = path.relative_to(ROOT).as_posix()
        allowed = ALLOWLIST.get(rel, ())
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if "QIcon::fromTheme" in line and not any(token in line for token in allowed):
                errors.append(f"{rel}:{number}: unapproved QIcon::fromTheme")
    for desktop in ROOT.glob("*.desktop"):
        for number, line in enumerate(desktop.read_text(encoding="utf-8").splitlines(), 1):
            if line.startswith("Icon=") and not line.startswith("Icon=aero7-"):
                errors.append(f"{desktop.name}:{number}: launcher icon must use aero7- namespace")
    if errors:
        raise SystemExit("\n".join(errors))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
