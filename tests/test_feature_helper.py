#!/usr/bin/python3

import json
import os
import stat
import subprocess
import tempfile
import unittest
from pathlib import Path


HELPER = Path(__file__).resolve().parents[1] / "helpers" / "aero7-feature-helper"


class FeatureHelperTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.state = self.root / "installed"
        self.catalog = self.root / "features.json"
        self.log = self.root / "feature.log"
        self.lock = self.root / "db.lck"
        self.reboot_dir = self.root / "reboot-required"
        self.pacman = self.root / "pacman"
        self.systemctl = self.root / "systemctl"
        self._write_executable(self.pacman, """#!/usr/bin/python3
import os, pathlib, sys
state = pathlib.Path(os.environ['FAKE_STATE'])
args = sys.argv[1:]
pkg = args[-1] if args else ''
if os.environ.get('FAKE_PACMAN_FAIL') == '1' and ('--sync' in args or '--remove' in args):
    print('simulated transaction failure')
    sys.exit(1)
if '-Q' in args:
    sys.exit(0 if (state / pkg).exists() else 1)
if '-Si' in args:
    if os.environ.get('FAKE_NETWORK_FAIL') == '1':
        print('error: failed retrieving file: Could not resolve host')
        sys.exit(1)
    if os.environ.get('FAKE_MISSING') == pkg:
        print('error: package not found')
        sys.exit(1)
    sys.exit(0)
if '--print-format' in args:
    requested = args[args.index('%n') + 1:]
    blocked = os.environ.get('FAKE_REQUIRED', '').split(',')
    sys.exit(1 if any(name in blocked for name in requested) else 0)
state.mkdir(exist_ok=True)
if '--sync' in args:
    for name in args[args.index('--noconfirm') + 1:]: (state / name).touch()
elif '--remove' in args:
    for name in args[args.index('--noconfirm') + 1:]: (state / name).unlink(missing_ok=True)
sys.exit(0)
""")
        self._write_executable(self.systemctl, """#!/usr/bin/python3
import os, sys
args = sys.argv[1:]
if os.environ.get('FAKE_SERVICE_FAIL') == '1': sys.exit(1)
if os.environ.get('FAKE_VERIFY_FAIL') == '1' and 'is-active' in args: sys.exit(1)
sys.exit(0)
""")
        self.write_catalog([
            self.feature("demo", ["demo-pkg"], ["demo.service"]),
        ])

    def tearDown(self):
        self.temp.cleanup()

    @staticmethod
    def _write_executable(path, text):
        path.write_text(text, encoding="utf-8")
        path.chmod(path.stat().st_mode | stat.S_IXUSR)

    @staticmethod
    def feature(feature_id, packages, services=None):
        return {
            "id": feature_id,
            "name": feature_id.title(),
            "packages": packages,
            "services": services or [],
            "availability": {"mode": "repository"},
        }

    def write_catalog(self, features):
        self.catalog.write_text(json.dumps({"features": features}), encoding="utf-8")

    def run_helper(self, *changes, extra_env=None):
        env = os.environ.copy()
        env.update({
            "AERO7_FEATURE_HELPER_TESTING": "1",
            "AERO7_TEST_CATALOG": str(self.catalog),
            "AERO7_TEST_LOG": str(self.log),
            "AERO7_TEST_LOCK": str(self.lock),
            "AERO7_TEST_PACMAN": str(self.pacman),
            "AERO7_TEST_SYSTEMCTL": str(self.systemctl),
            "AERO7_TEST_REBOOT_DIR": str(self.reboot_dir),
            "FAKE_STATE": str(self.state),
        })
        if extra_env:
            env.update(extra_env)
        return subprocess.run([str(HELPER), *changes], env=env, text=True,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              check=False)

    def test_install_and_remove_keep_user_data(self):
        user_data = self.root / "user-config"
        user_data.write_text("keep", encoding="utf-8")
        install = self.run_helper("demo=install")
        self.assertEqual(install.returncode, 0, install.stdout)
        self.assertTrue((self.state / "demo-pkg").exists())
        stages = [json.loads(line)["stage"] for line in install.stdout.splitlines()]
        self.assertIn("checking", stages)
        self.assertIn("installing", stages)
        self.assertIn("configuring", stages)
        self.assertIn("verifying", stages)
        remove = self.run_helper("demo=remove")
        self.assertEqual(remove.returncode, 0, remove.stdout)
        self.assertFalse((self.state / "demo-pkg").exists())
        self.assertEqual(user_data.read_text(encoding="utf-8"), "keep")

    def test_unknown_feature_is_rejected(self):
        result = self.run_helper("not-allowlisted=install")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("not allowlisted", result.stdout)

    def test_missing_repository_package_is_clear(self):
        result = self.run_helper("demo=install", extra_env={"FAKE_MISSING": "demo-pkg"})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unavailable", result.stdout)

    def test_package_manager_lock_is_clear(self):
        self.lock.touch()
        result = self.run_helper("demo=install")
        self.assertEqual(result.returncode, 75)
        self.assertIn("package manager is busy", result.stdout)

    def test_transaction_and_postinstall_failures_are_reported(self):
        transaction = self.run_helper("demo=install", extra_env={"FAKE_PACMAN_FAIL": "1"})
        self.assertNotEqual(transaction.returncode, 0)
        self.assertIn("could not install the required packages", transaction.stdout)
        self.assertIn("simulated transaction failure", self.log.read_text(encoding="utf-8"))
        service = self.run_helper("demo=install", extra_env={"FAKE_SERVICE_FAIL": "1"})
        self.assertNotEqual(service.returncode, 0)
        self.assertIn("configuration did not finish", service.stdout)

    def test_service_verification_failure_is_reported(self):
        result = self.run_helper("demo=install", extra_env={"FAKE_VERIFY_FAIL": "1"})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("required service did not start", result.stdout)

    def test_network_failure_has_friendly_message(self):
        result = self.run_helper("demo=install", extra_env={"FAKE_NETWORK_FAIL": "1"})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("can't reach the package repositories", result.stdout)

    def test_dependencies_and_conflicts_are_enforced(self):
        dependency = self.feature("dependency", ["dependency-pkg"])
        parent = self.feature("parent", ["parent-pkg"])
        parent["dependencies"] = ["dependency"]
        conflict = self.feature("conflict", ["conflict-pkg"])
        parent["conflicts"] = ["conflict"]
        self.write_catalog([dependency, parent, conflict])
        missing_dependency = self.run_helper("parent=install")
        self.assertNotEqual(missing_dependency.returncode, 0)
        self.assertIn("requires", missing_dependency.stdout)
        together = self.run_helper("dependency=install", "parent=install")
        self.assertEqual(together.returncode, 0, together.stdout)
        install_conflict = self.run_helper("conflict=install")
        self.assertEqual(install_conflict.returncode, 0, install_conflict.stdout)
        conflict_result = self.run_helper("parent=install")
        self.assertNotEqual(conflict_result.returncode, 0)
        self.assertIn("conflicts", conflict_result.stdout)

    def test_reboot_marker_and_audit_fields_follow_real_state(self):
        feature = self.feature("restart-demo", ["restart-pkg"])
        feature["rebootRequired"] = True
        feature["logoutRequired"] = True
        self.write_catalog([feature])
        installed = self.run_helper("restart-demo=install")
        self.assertEqual(installed.returncode, 0, installed.stdout)
        self.assertTrue((self.reboot_dir / "restart-demo").exists())
        record = json.loads(self.log.read_text(encoding="utf-8").splitlines()[-1])
        self.assertEqual(record["packages"], ["restart-pkg"])
        self.assertTrue(record["rebootRequired"])
        self.assertTrue(record["logoutRequired"])
        removed = self.run_helper("restart-demo=remove")
        self.assertEqual(removed.returncode, 0, removed.stdout)
        self.assertFalse((self.reboot_dir / "restart-demo").exists())

    def test_shared_package_is_not_removed(self):
        self.write_catalog([
            self.feature("first", ["shared"]),
            self.feature("second", ["shared"]),
        ])
        installed = self.run_helper("first=install")
        self.assertEqual(installed.returncode, 0, installed.stdout)
        removed = self.run_helper("first=remove")
        self.assertEqual(removed.returncode, 0, removed.stdout)
        self.assertTrue((self.state / "shared").exists())

    def test_shared_system_dependency_is_retained_while_feature_is_removed(self):
        feature = self.feature("reader", ["reader-app"])
        feature["supportPackages"] = ["speech-service", "shared-voice"]
        self.write_catalog([feature])
        installed = self.run_helper("reader=install")
        self.assertEqual(installed.returncode, 0, installed.stdout)
        removed = self.run_helper(
            "reader=remove", extra_env={"FAKE_REQUIRED": "shared-voice"})
        self.assertEqual(removed.returncode, 0, removed.stdout)
        self.assertFalse((self.state / "reader-app").exists())
        self.assertFalse((self.state / "speech-service").exists())
        self.assertTrue((self.state / "shared-voice").exists())


if __name__ == "__main__":
    unittest.main()
