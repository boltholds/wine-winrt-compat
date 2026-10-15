#!/usr/bin/env python3
"""Fail-closed structural validation for wine-winrt-compat project metadata.

This does not assert Wine API correctness. Run native/Wine ABI tests separately.
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FILE = ROOT / "docs" / "winrt" / "milestones.json"
STATES = {"verified_in_isolated_lab", "research_only", "rejected", "upstreamed"}


def validate(data):
    if not isinstance(data, dict) or data.get("schema_version") != 1:
        raise ValueError("Unsupported metadata schema")
    if data.get("project") != "wine-winrt-compat":
        raise ValueError("Unexpected project identity")
    upstream = data.get("upstream", {})
    if upstream.get("repository") != "wine-mirror/wine":
        raise ValueError("Upstream identity mismatch")
    sha = upstream.get("validated_lab_commit", "")
    if not re.fullmatch(r"[a-f0-9]{40}", sha):
        raise ValueError("A full validated upstream commit SHA is required")
    if upstream.get("baseline_branch") != "baseline/wine-11.19-a312fc87":
        raise ValueError("Unexpected pinned-baseline branch")
    policy = data.get("repository_policy", {})
    if policy.get("stable_branch") != "master" or policy.get("no_proprietary_binaries") is not True:
        raise ValueError("The upstream or binary policy was weakened")
    if policy.get("merge_requires_native_parity") is not True:
        raise ValueError("Missing native parity gate")
    milestones = data.get("milestones")
    if not isinstance(milestones, list) or not milestones:
        raise ValueError("Empty milestone ledger")
    ids = []
    for item in milestones:
        if not isinstance(item, dict):
            raise ValueError("Malformed milestone")
        if not re.fullmatch(r"v[0-9]+", item.get("id", "")):
            raise ValueError("Bad milestone identifier")
        if item.get("state") not in STATES:
            raise ValueError("Unsupported evidence state")
        if not item.get("area") or not item.get("evidence"):
            raise ValueError("Missing claim or evidence")
        if item.get("upstreamed") is not False and item["state"] != "upstreamed":
            raise ValueError("An experiment cannot claim to be upstreamed")
        ids.append(item["id"])
    if len(ids) != len(set(ids)):
        raise ValueError("Duplicate milestone")
    by_id = {i["id"]: i for i in milestones}
    if by_id.get("v52", {}).get("state") != "research_only":
        raise ValueError("v52 has not yet been validated")
    if not data.get("limitations"):
        raise ValueError("Known limitations must not be omitted")
    return len(milestones)


def main():
    count = validate(json.loads(FILE.read_text(encoding="utf-8")))
    for name in ("README.md", "STATUS.md", "DEVELOPMENT.md", "PATCH_ACCEPTANCE.md"):
        if not (FILE.parent / name).is_file():
            raise ValueError("Missing documentation: " + name)
    print(f"winrt-metadata: {count} milestones validated; no Wine runtime verification implied")


if __name__ == "__main__":
    main()
