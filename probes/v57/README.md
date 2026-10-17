# v57: trace the exact CoreApplicationView method used by XAML Initialize

**Diagnostic instrumentation only; no COM behavior is modified.**

The real Shapr3D v56 replay of 2026-10-17 successfully created the hidden v55 CoreWindow, then `Windows.UI.Xaml.FrameworkView::Initialize` returned `E_NOTIMPL` before `SetWindow`, `Load`, and `Run`.

The existing v56 `ICoreApplicationView` implementation contains several uninstrumented methods that return `E_NOTIMPL`, notably `add_Activated`, `GetIids`, `GetRuntimeClassName`, `GetTrustLevel`, `get_IsMain`, and `get_IsHosted`. Microsoft examples register `CoreApplicationView.Activated` inside `IFrameworkView.Initialize`; this is a **hypothesis**, not yet the proven source of the application's HRESULT.

This branch adds `COREAPP57_CALL_*` TRACE messages at entry to each `ICoreApplicationView` vtable method and retains the existing `COREAPP56_*` lifecycle markers. **No return value, COM AddRef/Release, output parameter or vtable layout has been changed.**

To record the next bounded replay using a separately built and SHA-checked candidate `twinapi.appcore.dll`:

```text
WINE_COREAPP_INIT_TRACE_V57=1
WINE_COREAPP_EXPERIMENT=1
WINE_COREAPP_VIEW_INIT_V54=1
WINE_COREAPP_LIFECYCLE_V56=1
WINE_COREWINDOW_V55=1
WINEDEBUG=-all,+twinapi,+ui,+combase,+seh,+loaddll
```

Look for the first `COREAPP57_CALL_*` event between `COREAPP56_WINDOW_CREATE hr=0` and `COREAPP56_INITIALIZE hr=0x80004001`. If `COREAPP57_CALL_ADD_ACTIVATED` appears, proceed to a **separate Windows-native event ABI, token, handler-lifetime and callback parity test** before implementing activation. If no vtable method is called, inspect XAML initialization with a narrow debugger trace instead. Never return invented `S_OK`, intercept private licensing, change the Shapr3D binary or suppress .NET Native exceptions.

This source branch is not compiled/run inside the real Shapr3D installation until a user-authorized, rollback-guarded replay is performed. The GitHub Actions job only verifies that the trace-only code compiles; it does not assert real native Windows compatibility.
