# v53 research: originating HRESULT and CoreApplication.Run boundary

**Research status:** confirmed causal link for the captured v52 Shapr3D run; no new Wine API implementation.

## Dynamic breakpoint findings

An isolated WineDbg run of an unchanged proprietary application captured the following flow, preserving registers and exception processing:

1. `Windows.UI.Xaml.dll` successfully returns from `RoActivateInstance` with `S_OK`.
2. The following `QueryInterface` returns `S_OK`.
3. The virtual dispatch to `ICoreApplication::Run` returns `E_NOTIMPL (0x80004001)`.
4. In `Shapr3D.dll + 0x1A3ED89`, WineDbg observed `EAX=ECX=0x80004001` on the exception-projection branch.
5. `SharedLibrary.dll + 0x8FEF6C` invokes the imported .NET Native `RhpThrowEx`; it ultimately reaches a fail-fast path.

These observations connect the v52 application failure to the currently intentionally unimplemented `ICoreApplication::Run` method. This does **not** establish that fixing Run alone will make Shapr3D operational.

A separate earlier debugger run observed `0x8007007E` (`ERROR_MOD_NOT_FOUND`), so other paths/configurations can fail for unrelated reasons. Preserve this distinction in future diagnostics.

## Source behavior

In pinned Wine 11.19 experiment v47, `CoreApplication.Run` has a bounded `CreateView` probe and deliberately returns `E_NOTIMPL` without constructing the actual CoreWindow, dispatcher, activation or view lifecycle. It must not claim `S_OK` until a real implementation exists.

The next implementation contract is:

- Obtain `IFrameworkView` from `IFrameworkViewSource.CreateView`.
- Construct and associate a real `CoreApplicationView` and `CoreWindow` on the correct thread.
- Implement `Initialize → SetWindow → Load → Run → Uninitialize` with real failure propagation, cleanup and thread affinity.
- Validate `GetCurrentView` and `CoreWindow.GetForCurrentThread` both without and with the current thread's owned CoreWindow.
- Test callbacks, early failure, COM reference-count balance, dispatcher shutdown and exception propagation.

**Do not** fabricate success HRESULTs, change the proprietary binary, suppress application exceptions, or bypass package/license/input/security checks.

## Additional tested results

The existing `v53-unpackaged-GetCurrentView` synthetic Wine probe exercised STA and MTA. The Wine experimental interface returned `E_NOTIMPL` with null view in both cases; this is **not yet native-Windows parity evidence**. Its Win64 test executable was SHA-pinned at `b4da1d4c288eef6498886af1a4eeaa9a4e8e7b360b6327709a3602185a94a668`.

A further native Windows execution was attempted but blocked by the remote execution authorization boundary. No native v53 HRESULT may be claimed from that attempt.

## Evidence provenance

These observations originate in the isolated local `runtime-v52/hresult-debug-v52` research directory. The experiment left original Wine source and application bytes unchanged; source and known DLL hashes were restored after the run. Detailed logs remain in that private laboratory and are not redistributed in this repository.

Last verified on 2026-10-16.
