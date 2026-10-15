# Ported Wine v47–v51 experiments — verification record

This directory documents a **downstream research branch** and its bounded tests. It does not attest that Shapr3D or any other application is usable end-to-end.

**Source base:** Wine 11.19 commit `a312fc87a501a447576effe8b02444558f9bbf30`.

**Branch:** `experiment/winrt-v47-v51`, reviewed in [draft PR #2](https://github.com/boltholds/wine-winrt-compat/pull/2) against the historical baseline (not against upstream `master`).

Eight source commits ported eleven Wine files, grouped by feature: v47 CoreApplication, SHCore scale-percent ordinal 265, KernelBase unpackaged/SID error handling, XAML-DXGI priority; v50 private PropertySet and metadata; v51 SHCore sink ordinals 240/246 and trace-only fail-fast diagnostics.

## Independent PE64 build

An isolated WSL2 build directory, created **without modifying the original Wine laboratory**, configured with:

```bash
./configure --enable-win64 --with-mingw --disable-tests
make -j6 \
  dlls/twinapi.appcore/x86_64-windows/twinapi.appcore.dll \
  dlls/wintypes/x86_64-windows/wintypes.dll \
  dlls/shcore/x86_64-windows/shcore.dll \
  dlls/kernelbase/x86_64-windows/kernelbase.dll \
  dlls/dxgi/x86_64-windows/dxgi.dll
```

**Observed result:** configure PASS, five PE64 DLL targets PASS (source commit `d8fcabfdae695e221e02dfef53ff650516518303` in the isolated local worktree). The entire source Git tree was compared with the published GitHub source tree, SHA `d8df26e363d7898b0dc1e2d0507fd0a05e674768`.

The separate GitHub Actions workflow compiles these same five targets when triggered. Build success alone does not validate COM threading, reference counting, private ABI, DPI callbacks, UI activation or application startup.

## Historical lab test scope

- v47 CoreApplication: 17/17 regression checks in the original isolated laboratory.
- v50 PropertySet private/public identity: STA/MTA checks and Windows/Wine BCP47 comparison of 41 selected cases.
- v51 SHCore scale-sink: nine Wine synthetic cases and native Windows registration/revocation with reference ownership evidence.
- Genuine Shapr3D passed the formerly missing SHCore registration, then exited with a .NET Native fail-fast. No working interactive window has been demonstrated.
- v52 original HRESULT at Shapr3D.dll + 0x1A3ED89 remains unknown; v52 is **research only**.

No Microsoft or Shapr3D binaries or proprietary source files are included. Private API compatibility here is **experimental**, behind explicit feature flags. No functionality is claimed for DPI callback delivery, full `IFrameworkView` lifecycle, or GPU scheduling control.
