# v65 — bounded private CoreApplication UnhandledErrorDetected ABI experiment

## Evidence and scope

Native v62/v63 confirmed that IID `17b0e613-942a-422d-904c-f90dc71a7dae`
returns a CoreApplication private interface whose vtable includes a slot 11
at offset `0x58`, and that the slot forwards a handler and a token pointer
to another COM interface. The v64 XAML callback identified the GUID
`f68bc421-6b54-559b-9cdd-489aad0bd41d`, matching the WinRT
`EventHandler<UnhandledErrorDetectedEventArgs>` delegate. The Wine 11.19
`include/windows.applicationmodel.core.idl` independently declares the
public `ICoreApplicationUnhandledError::UnhandledErrorDetected` event,
which uses the same delegate and an `EventRegistrationToken`.

The separately uploaded Shapr3D decompilation archive was inventoried:
it contains application WinRT metadata projections and Siemens Parasolid
pseudocode, but not the system `twinapi.appcore.dll` or
`Windows.UI.Xaml.dll` implementations, nor literal references to
`CoreApplication`/the private IID. **No decompiled Shapr3D/Siemens code
has been included or copied to this repository.**

## Design

This branch begins **directly from the safe v58 main-view revision**, not
v59. The unsupported v59 `GetIids` change is deliberately absent:
its real Shapr3D replay led to a native memory access violation after
XAML called beyond the end of Wine's 8-entry private vtable.

The v65 private interface explicitly defines:
- slots 0–7: existing v50 behavior (including implemented slot 7 properties);
- slots 8–10: unknown and unchanged in semantics, return `E_NOTIMPL`;
- slot 11: bounded subscription for the observed
  `UnhandledErrorDetectedEventArgs` event handler;
- slot 12: bounded matching unsubscription using the token.

The new behavior is **disabled** unless
`WINE_COREAPP_UNHANDLED_V65=1` is set. It validates the handler's
delegate IID with `QueryInterface`, retains the returned owning COM
reference, issues monotonic event tokens under a lock, and on token
removal releases the handler **outside the lock**.

**Major limitation**: no WinRT unhandled-error detection/dispatch is
implemented. A successful registration does **not** imply callbacks can
be invoked on real error events. It must not be used as a production
fault-reporting system. Registration is provided only to test the
observed ABI and identify subsequent initialization dependencies in
a bounded, rollback-guarded application replay.

No arbitrary native slot methods are executed, and the application or
package installation is not modified. No activation/license/protection
changes are made.

## Verification

The GitHub Actions v65 workflow runs source-contract tests and recompiles
`twinapi.appcore.dll` as PE64 for Wine 11.19. This is not live WinRT ABI
conformance testing. Never claim Shapr3D GUI success without a guarded
real-app replay with exact DLL restoration.

Next test: independently SHA-pin v65 Wine source, create an isolated Wine
worktree, preflight, and run a brief opt-in replay only with automatic
rollback. Look for `COREAPP65_UNHANDLED_ADD` and the first subsequent
fatal HRESULT/exception, **never infer full functionality from one S_OK**.
