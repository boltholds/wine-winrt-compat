#!/usr/bin/env python3
"""Verify exact reviewed Wine v47–v51 source blobs in this experimental branch.

This is a *source identity* check, not native ABI or runtime verification.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
EXPECTED_BLOBS = {
  "dlls/twinapi.appcore/core_application.c": "4c0d8967b29988bf1622a81b05947ce9ee8f79f4",
  "dlls/shcore/main.c": "258d6664e2530fe613fea3ed67d19ed981e3cbd6",
  "dlls/shcore/shcore.spec": "1cc4623bf301b213cdbfa99057625cd84b028372",
  "dlls/kernelbase/kernelbase.spec": "d24ee7f81d3908582cd8da814eebf3d30b8bf4d7",
  "dlls/kernelbase/security.c": "45bdb256e06a221ebd98f6e4d3d2aba0733191ce",
  "dlls/kernelbase/version.c": "a1617af8d35ccb1c7f5444a39d0683c560e4e681",
  "include/winbase.h": "9fe25ba7bd90bc0598b5979079f96c8e0b32f957",
  "dlls/dxgi/device.c": "60be18383818a51c5ea0e3a0c9e7da301a88178d",
  "dlls/dxgi/dxgi_private.h": "b65266bfcfc8c60976020ef42b0717f890e50d62",
  "dlls/wintypes/propertyset.c": "26edc0dc81650028636bf2a552a91972033444a7",
  "dlls/kernelbase/debug.c": "694960dc4ceb9d30ae7a70a39b9d0237930bd726"
}

def git_blob(path):
    cp = subprocess.run(["git", "-C", str(ROOT), "hash-object", str(ROOT / path)],
        capture_output=True, text=True, timeout=30)
    if cp.returncode:
        raise ValueError(f"Cannot hash {path}: {cp.stderr.strip()}")
    return cp.stdout.strip()

def validate_expected(expected):
    if len(expected) != 11:
        raise ValueError("Expected exactly eleven reviewed Wine source files")
    for path, sha in expected.items():
        if not (ROOT / path).is_file():
            raise ValueError(f"Missing source file: {path}")
        if len(sha) != 40 or git_blob(path) != sha:
            raise ValueError(f"Source differs from reviewed v47–v51 patch series: {path}")
    version = (ROOT / "VERSION").read_text(encoding="utf-8").strip()
    if version != "Wine version 11.19":
        raise ValueError("Wine upstream version drifted: " + version)
    return len(expected)

if __name__ == "__main__":
    print("wine-winrt-source-pins:", validate_expected(EXPECTED_BLOBS), "files match reviewed Git blobs")
