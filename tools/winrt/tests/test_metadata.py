"""Contract and negative-control checks for project metadata."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / "tools" / "winrt" / "check_metadata.py"
SPEC = importlib.util.spec_from_file_location("winrt_check_metadata", SCRIPT)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class ManifestTests(unittest.TestCase):
    def setUp(self):
        self.original = json.loads((ROOT / "docs" / "winrt" / "milestones.json").read_text(encoding="utf-8"))

    def test_current_evidence_is_well_formed(self):
        self.assertEqual(MODULE.validate(self.original), 4)

    def test_missing_baseline_fails_closed(self):
        self.original["upstream"]["validated_lab_commit"] = "a312fc87"
        with self.assertRaises(ValueError):
            MODULE.validate(self.original)

    def test_unverified_v52_cannot_be_credited(self):
        self.original["milestones"][-1]["state"] = "verified_in_isolated_lab"
        with self.assertRaises(ValueError):
            MODULE.validate(self.original)

    def test_upstreamed_flag_must_match_state(self):
        self.original["milestones"][0]["upstreamed"] = True
        with self.assertRaises(ValueError):
            MODULE.validate(self.original)

    def test_no_binary_policy_cannot_be_dropped(self):
        self.original["repository_policy"]["no_proprietary_binaries"] = False
        with self.assertRaises(ValueError):
            MODULE.validate(self.original)


if __name__ == "__main__":
    unittest.main()
