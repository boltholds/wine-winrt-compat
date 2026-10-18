# v66 — ICoreApplication2 discovery, no event spoofing

## Real evidence (2026-10-18)
The isolated `replay-v65-20261018T200540Z-75744.zip` returned:
- `COREAPP58_ISMAIN_SUCCESS value=1`;
- `COREAPP65_UNHANDLED_ADD ... token=0x1`;
- next `QueryInterface` for IID `998681fb-1ab6-4b7f-be4a-9a0645224c04` twice returned **E_NOINTERFACE**;
- `RoFailFastWithErrorContextInternal2 0x80004002`, `c0000602`;
- no `COREAPP56_INITIALIZE` / `SETWINDOW` completion;
- owned-prefix wineserver shut down and DLLs restored exactly (`rollback.json: exact_restoration=true`).

The missing IID is **public ICoreApplication2**, declared in Wine 11.19
`include/windows.applicationmodel.core.idl` under UniversalApiContract 3.0.
Its IInspectable vtable defines seven domain slots, in order:
`add/remove_BackgroundActivated`, `add/remove_LeavingBackground`,
`add/remove_EnteredBackground`, `EnablePrelaunch(boolean)`.

## Diagnostic scope
**Do not claim a working application lifecycle or background events.**

This branch is layered directly on v65 (`WINE_COREAPP_UNHANDLED_V65=1`).
The additional `WINE_COREAPP2_TRACE_V66=1` gates only QI support for this
public IID and returns a factory-owned pointer to a correctly ordered
13-slot Win64 COM vtable (6 IInspectable slots + 7 methods).

Each method logs a `COREAPP66_CALL_...` marker and returns `E_NOTIMPL`;
registration output tokens are zeroed, not made up. `GetIids`,
`GetRuntimeClassName` and `GetTrustLevel` also remain explicitly
unimplemented. `QueryInterface`/AddRef/Release share the static factory
identity/refcount.

This allows an **opt-in, bounded application replay** to reveal exactly which
ICoreApplication2 method XAML asks for, without fake event callbacks or success.
It makes no changes to app binaries, package identity, DRM/licensing,
activation callbacks, or Wine master. v59 `GetIids` remains absent.

## Verification
GitHub Actions checks v66 method order/source contracts, rechecks v65 and
v58, and builds PE64 `twinapi.appcore.dll` against Wine 11.19.
Live behavior requires a SHA-pinned isolated worktree and rollback-guarded
v66 replay; it is **not** established by successful compilation alone.
