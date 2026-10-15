# Development workflow

## Upstream and pinned baseline

This is a real fork of `wine-mirror/wine`. Its `master` branch is reserved for Wine upstream; do not force-push or mix in project experiments.

The isolated v47–v51 laboratory was developed against:

- Wine source commit: `a312fc87a501a447576effe8b02444558f9bbf30`
- Version: Wine 11.19
- A dedicated WSL2 source/build tree and disposable Wine prefix.

The fork's upstream `master` was **36 commits ahead** of this historical source snapshot at project bootstrap. Until revalidation, branch from `baseline/wine-11.19-a312fc87` rather than applying old changes blindly onto newer Wine.

Example for a new checkout:

```bash
git clone https://github.com/boltholds/wine-winrt-compat.git
cd wine-winrt-compat
git remote add upstream https://github.com/wine-mirror/wine.git
git fetch origin
git switch -c experiment/corewindow origin/baseline/wine-11.19-a312fc87
```

Synchronize upstream in a separate step: `git fetch upstream`, then fast-forward `master` only. Never merge partially tested experiments into upstream-tracking `master`.

## Patch discipline

1. One independent Wine API contract per patch, with clear failure behavior.
2. Preserve existing HRESULTs; do not return `S_OK` merely to bypass initialization failures.
3. Verify STA/MTA behavior, invalid pointers, null outputs, reference ownership and event unsubscribe.
4. Run native Windows comparison tests where legal and possible.
5. Keep sample programs, synthetic tests and test logs separate from any proprietary application bytes.
6. Use short-lived feature flags for incomplete implementations. Do not treat a flag as production readiness.
7. Submit a PR to an integration branch for review; upstream-focused fixes can later be proposed to WineHQ through its contribution process.

## CI

The `winrt-metadata` workflow validates project manifests and negative-control tests. It intentionally **does not build the entire Wine tree**. Building Wine, running Wine ABI tests and native Windows parity will be independent CI phases and must not be reported as green until configured and run.