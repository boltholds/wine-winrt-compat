# v52 native Windows / Wine contracts — corrected experiment

This is **research**, not a merged Wine compatibility implementation.

The compiled test in `v52_contract_probe.c` runs without loading Shapr3D or changing system accessibility settings. It was rebuilt after fixing a single C dispatcher mistake: the two test modes had originally been reversed. The original logs were preserved; only the corrected runs are used below.

**Recorded on October 15, 2026:** four native Windows 11 and four isolated Wine 11.19 tests, covering STA and MTA.

| API behavior (no CoreWindow attached to caller) | Native Windows 11 | Wine 11.19 |
| --- | --- | --- |
| `CoreWindowStatics.GetForCurrentThread` | `S_OK`, NULL | `S_OK`, NULL |
| `AccessibilitySettings.add_HighContrastChanged(handler)` | `0x80070490` (`ERROR_NOT_FOUND`) | `0x80004001` (`E_NOTIMPL`) |
| `AccessibilitySettings.add_HighContrastChanged(NULL)` | `0x80070490` | `0x80004001` |

On this Windows test environment both event registrations failed and left the output token sentinel unchanged. The test handler received no AddRef and returned to reference count 1. Since no native subscription succeeded, **event callback delivery and unsubscribe remain untested**.

The Windows result may depend on the application/package context. Do not implement unconditional `S_OK`, invent a token, or assume that an unpackaged test reproduces Shapr3D's application context. The native result above also does not reveal the HRESULT projected into the Shapr3D .NET Native exception.

**Provenance:** C source SHA-256 `d77949eb568ddfdd6501cbce15c8214e01a98912beba4f4bb2c8dff0c1490459`. The source and normalized result JSON are included. No proprietary application or Windows binaries are committed.

## Repeatable probes

Cross-compile:

```bash
x86_64-w64-mingw32-gcc -std=c11 -O2 -Wall -Wextra -Werror \
  -Wno-cast-function-type -Wl,--no-insert-timestamp \
  -o v52_contract_probe.exe probes/v52/v52_contract_probe.c
```

Execute each executable on an authorized Windows host **and** in a disposable Wine prefix:

```text
v52_contract_probe.exe corewindow sta
v52_contract_probe.exe corewindow mta
v52_contract_probe.exe accessibility sta
v52_contract_probe.exe accessibility mta
```

The source records HRESULT, tokens and delegate reference counts. Do not run native/Windows benchmarks in GitHub Linux CI: the CI job only cross-compiles and validates the captured evidence.
