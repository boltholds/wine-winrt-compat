# v54 — bounded ICoreApplicationView / IFrameworkView.Initialize experiment

This is an **incomplete, feature-flagged prototype**, not a functional UWP/CoreWindow host.

## Motivation

The verified Shapr3D v52 breakpoints identified `E_NOTIMPL` returned by experimental `ICoreApplication::Run`. Full fidelity requires native CoreWindow, dispatcher, app-view identity, activation, threading, and orderly `IFrameworkView.Initialize → SetWindow → Load → Run → Uninitialize`.

The first v54 attempt called a historical v47 probe whose fake `IFrameworkView` lacked method slots for `Initialize`. This caused a null-execute fault, automatically attached WineDbg and timed out. That unsafe source was reverted and the original lab PE restored exactly. **Do not run lifecycle experiments against that legacy mock.**

## Implementation in this branch

Under **both** `WINE_COREAPP_RUN_PROBE=1` and `WINE_COREAPP_VIEW_INIT_V54=1`, Wine creates a heap-allocated `ICoreApplicationView` with explicit COM reference counting and real QueryInterface for IUnknown / IInspectable / ICoreApplicationView. It passes that object to `IFrameworkView.Initialize` after validating `IFrameworkViewSource.CreateView` and the returned view.

- If Initialize fails, propagate the **exact** failing HRESULT and release all acquired references.
- If Initialize succeeds, call `IFrameworkView.Uninitialize`, release references and still return **`E_NOTIMPL`**. This is deliberate; a real CoreWindow and dispatcher have **not** been created.
- `ICoreApplicationView.get_CoreWindow` returns `E_NOTIMPL` and null instead of pretending a real CoreWindow exists.
- Disabled feature flag retains v47 Run probe behavior, including `E_NOTIMPL`.

### Bounded tests (real Wine 11.19 PE execution in isolated WSL lab)

`full_frameworkview_mock.c` implements all IFrameworkView vtable methods, rather than the incomplete v47 mock. The `test_results.json` documents three executed cases:

| Case | Expected sequence | Observed |
|---|---|---|
| v47 flag off | CreateView → QI; no Initialize | PASS, exit 0 |
| v54 init succeeds | CreateView → QI → Initialize → Uninitialize; no SetWindow/Load/Run | PASS, exit 0, Run reports E_NOTIMPL |
| v54 init fails | CreateView → QI → Initialize(E_FAIL); no Uninitialize | PASS, exit 0, propagated E_FAIL |

All three cases returned source/view COM counts to 1. The Wine source baseline `twinapi.appcore.dll` was restored with SHA256 `63747669d8286097ddfc3effe3b36914c99da491124413fcc791276c1baa8a10` after temporary testing.

**Not implemented or claimed:** CoreWindow construction, dispatch queue, activation events, CurrentView context, SetWindow, Load, Run loop, true Uninitialize after a real app, concurrency, native-Windows parity, interactive Shapr3D UI. DO NOT merge into `master` or advertise usable application support.

## Follow-up

Build `ICoreWindow` and `CoreDispatcher` on a real owned HWND with thread affinity, then add native Windows and Wine parity tests (including error paths and reference counts) before enabling further lifecycle stages.
