# Research status (2026-10-15)

| Milestone | Observed result | Missing evidence |
| --- | --- | --- |
| v47 | CoreApplication WinRT/COM regression: 17/17; restored Wine baseline verified after v51 | Full `IFrameworkView` lifecycle |
| v50 | Private `ICoreApplicationPrivate` PropertySet ABI and object identity tested in STA/MTA; feature-flagged Wine experiment | Broader app compatibility |
| v51 | Native SHCore ordinals 240/246 identified; nine Wine smoke cases; Shapr3D reached successful registration with cookie 1 | Real DPI callbacks and COM marshaling parity |
| v52 | Static investigation identified `RhpThrowEx` in `mrt100_app.dll` (.NET Native); probe design for WinRT APIs | Original failed HRESULT and managed exception; native v52 execution |

**Application result:** Shapr3D has **not** reached a working interactive window. After the successful v51 SHCore call the observed run terminated with exit code 100 and .NET Native fail-fast exception code `0x302BFF64`. These are observations, not proof of the originating error.

**Important API distinctions:** `CoreWindow.GetForCurrentThread` may legitimately return `S_OK` with a null window when the calling thread has no CoreWindow. In the validated Wine source it already does so. `AccessibilitySettings.add_HighContrastChanged` and `remove_HighContrastChanged` still return `E_NOTIMPL` in that source; native token, callback and lifetime behavior must be measured before implementation.

**Evidence location:** local experimental logs/patch archives were produced outside this Git repository. They are **not yet imported** as verified source commits. Do not infer that the `master` branch contains v47–v52 functionality.