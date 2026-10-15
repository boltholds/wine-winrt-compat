# v52 root cause: XAML activation succeeds; Wine CoreApplication.Run is incomplete

**Verified 2026-10-15.** This is an engineering diagnosis of the pinned Wine 11.19 experimental source branch, not an interactive Shapr3D success claim.

## Exact call path

The original app uses `Windows.UI.Xaml.FrameworkViewSource`. Static PE import inspection of the native XAML component identifies `RoActivateInstance`; debugger breakpoints placed *after* the corresponding calls in the **same ASLR-randomized process** demonstrate:

| Return site | API or effect | Observed HRESULT |
| --- | --- | --- |
| `Windows.UI.Xaml.dll + 0x1AA373` | `RoActivateInstance(Windows.UI.Xaml.FrameworkViewSource)` | `S_OK` |
| `Windows.UI.Xaml.dll + 0x1AA39B` | `QueryInterface(ICoreApplication)` | `S_OK` |
| `Windows.UI.Xaml.dll + 0x1AA3C6` | `ICoreApplication::Run`, indirect call at vtable offset `0x68` (index 13) | **`E_NOTIMPL (0x80004001)`** |

The `IFrameworkViewSource::CreateView` and `IFrameworkView` QI have already returned `S_OK`. Next, `Shapr3D.dll + 0x1A3ED89` projects that **same** `E_NOTIMPL` into a .NET Native exception through `SharedLibrary.dll` `RhpThrowEx`. The observed app process then fails fast with `0x302BFF64`.

The specific `CoreApplication::Run` function in the source-verified v47 Wine experiment deliberately reports `E_NOTIMPL` after the bounded ViewSource QI/CreateView/View QI probe, because no genuine `CoreWindow` or dispatcher lifecycle is implemented. **This intentional implementation gap is the confirmed root cause of this v52 first fault.**

## What was *not* the root cause

When duplicate old test-app directories had been cleaned up, the WinRT class registry still referred to an old `Windows.UI.Xaml.dll` path. An earlier run returned `0x8007007E` (`ERROR_MOD_NOT_FOUND`) due to that **test-environment artifact**. Restoring an exact-SHA-pinned temporary link to the surviving disposable clone made `CreateView` succeed. The link has since been removed on rollback.

`AccessibilitySettings.add_HighContrastChanged` also returns `E_NOTIMPL` twice in Wine traces. While this is an independent incomplete contract, WineDbg located the first thrown exception in the *different* indirect call to `ICoreApplication::Run`; the event stub is **not** established as the immediate fatal path.

## Next milestone: implement the real view and window lifecycle (v53)

Do not merely change `CoreApplication.Run` to return `S_OK`. The operation needs a consistent, tested object and ownership graph, including:

1. `ICoreApplicationView` COM identity, lifecycle ownership and guaranteed usable `get_CoreWindow` only when one really exists.
2. A `CoreWindow` bound to a real, same-thread HWND, with `CoreWindow.GetForCurrentThread` resolving the current thread's window rather than a global/fake singleton.
3. A functional `CoreDispatcher` with proper thread and message queue semantics, event registration, callback dispatch and shutdown.
4. Verified invocation of `IFrameworkView.Initialize(view)` → `SetWindow(window)` → `Load(entryPoint)` → `Run()` → `Uninitialize()` as appropriate to the native contract.
5. Failure handling/COM reference balancing/rollback, including STA/MTA tests and event subscriptions (activation, closed, scale/high-contrast).

Stage these as separate opt-in, ABI-conformant patches and native Windows comparison probes. Before claiming application compatibility, verify end-to-end event processing and an actual interactive window. A passing compile or `CreateView` alone is insufficient.

## Evidence and safety

The machine-readable summary is [`coreapp_run_root_cause.json`](coreapp_run_root_cause.json) and is verified by repository unit tests. Raw debugger logs, exact binary hashes and a signed-off restoration report are retained in the user's local diagnostic workspace, not in this public repository. No proprietary Shapr3D/Windows executables, copyrighted code or private data are imported.

The experimental Wine lab was restored with **exact SHA-256 copies of five PE DLLs**; temporary app-local BCP47 and temporary XAML symlink were removed; the restored v47 regression suite again passed **17/17**.
