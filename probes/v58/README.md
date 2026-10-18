# v58 — implement IsMain for the owned primary CoreApplicationView

This is a **single-view experimental Wine 11.19** compatibility change. It is
independent of normal Wine behavior and gated behind
`WINE_COREAPP_MAINVIEW_V58=1`. The existing v57 diagnostic markers remain.

## Evidence

The uploaded bounded Shapr3D v57 diagnostic replay from October 18, 2026
(`replay-v57-20261018T092737Z-23738.zip`) observed:

```text
COREWINDOW55_CREATED hwnd=0x10056
COREAPP56_WINDOW_CREATE hr=0
COREAPP57_CALL_GET_ISMAIN iface=0x194BB320
COREAPP56_INITIALIZE hr=0x80004001
```

Only one ICoreApplicationView vtable method was observed between window
creation and Initialize returning E_NOTIMPL. The v57 implementation of
`get_IsMain` itself returns E_NOTIMPL, making it the concrete first compatibility
gap to address. No `add_Activated` call was observed.

Microsoft documents that the first application view is the **main view** and
its IsMain property is true:
https://learn.microsoft.com/en-us/windows/uwp/ui-input/application-view

## Change and scope

When all of the following hold, `get_IsMain` returns `S_OK` and sets
`*value=TRUE`:

- `WINE_COREAPP_MAINVIEW_V58=1` is explicitly enabled
- the CoreApplicationView owns an actual non-null v55 CoreWindow
- its HWND is valid and exists
- the caller is the owner thread

With the flag off or without an owned CoreWindow, behavior remains
`E_NOTIMPL`; null pointers remain `E_POINTER`, and foreign-thread access
returns `RPC_E_WRONG_THREAD`. An invalid HWND returns an explicit error.
The `COREAPP58_ISMAIN_SUCCESS` trace marker appears only on actual success.

**Not claimed:** multi-view/secondary-view support, app activation event
semantics, native Windows packaged-app parity, working Shapr3D UI, or
completion of `IFrameworkView.Initialize`. Only the next guarded real-app
replay can establish how far initialization proceeds. No proprietary app
binaries, input protection, package identity, or license handling are modified.

This is intentionally **Draft / experimental**, not ready for merge to
the Wine master branch.
