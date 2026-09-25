#!/usr/bin/python3

import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "helpers/aero7-system-settings-visibility"


class SystemSettingsVisibilityTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.config = self.root / "config"
        self.applications = self.root / "data/applications"
        self.env = os.environ.copy()
        self.env.update(XDG_CONFIG_HOME=str(self.config),
                        XDG_DATA_HOME=str(self.root / "data"),
                        AERO7_KBUILDSYCOCA=str(self.root / "missing-kbuildsycoca"))

    def run_action(self, action):
        return subprocess.run([str(SCRIPT), action], env=self.env,
                              text=True, capture_output=True, check=False)

    def test_default_hidden_and_feature_restores_both_launchers(self):
        self.assertEqual(self.run_action("status").stdout.strip(), "disabled")
        self.assertEqual(self.run_action("ensure").returncode, 0)
        for name in ("systemsettings.desktop", "kdesystemsettings.desktop"):
            contents = (self.applications / name).read_text()
            self.assertIn("NoDisplay=true", contents)
            self.assertIn("NotShowIn=KDE;", contents)
            self.assertNotIn("Hidden=true", contents)
        self.assertEqual(self.run_action("enable").returncode, 0)
        self.assertEqual(self.run_action("status").stdout.strip(), "enabled")
        self.assertEqual(self.run_action("ensure").returncode, 0)
        self.assertEqual(list(self.applications.iterdir()), [])
        self.assertEqual(json.loads((self.config / "aero7/system-settings.json").read_text()),
                         {"enabled": True})
        self.assertEqual(self.run_action("disable").returncode, 0)
        self.assertTrue((self.applications / "systemsettings.desktop").exists())

    def test_existing_user_launcher_is_not_overwritten(self):
        self.applications.mkdir(parents=True)
        custom = self.applications / "systemsettings.desktop"
        custom.write_text("[Desktop Entry]\nName=Custom Settings\n")
        result = self.run_action("ensure")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("custom launcher", result.stderr)
        self.assertEqual(custom.read_text(), "[Desktop Entry]\nName=Custom Settings\n")


if __name__ == "__main__":
    unittest.main()
