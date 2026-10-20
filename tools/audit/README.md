# Audit baseline and diagnostic probes

These tools record the Phase 2–3 baseline without changing production sources.
They are diagnostic fixtures, not a passing regression suite or a Unity runtime.
Run from the repository root with Python 3.9+ and `ANDROID_NDK_HOME` set.

## Reproduce

Select a new output directory under `build/` for a fresh snapshot. `prepare`
refuses to overwrite an existing snapshot. Supply CMake and Ninja paths when
they are not on `PATH` (the audit used the SDK's CMake 4.1.2).

```sh
python3 tools/audit/baseline.py --mode prepare
python3 tools/audit/baseline.py --mode build --cmake /path/to/cmake --ninja /path/to/ninja
python3 tools/audit/baseline.py --mode matrix
python3 tools/audit/baseline.py --mode variants
python3 tools/audit/baseline.py --mode headers
python3 tools/audit/baseline.py --mode integration --cmake /path/to/cmake --ninja /path/to/ninja
python3 tools/audit/baseline.py --mode analyze
python3 tools/audit/baseline.py --mode probes --cmake /path/to/cmake --ninja /path/to/ninja
python3 tools/audit/baseline.py --mode scanner
```

Pass the same `--out build/another-audit` to every command to use another snapshot.
Run `build` before `analyze`, which consumes its compilation database. Native
probe execution currently targets ARM64 Android; cross-compilation on another
host does not make that host capable of running the binaries.

## Scope and isolation

- `prepare`: syntax-check the original `src/Class.cpp`; copy production inputs;
  select the existing Dobby block **only in the copy**. Record the exact settings
  patch and SHA-256 manifest of original implementation inputs.
- The generated `dobby.h` stub always returns failure and never hooks anything.
  It exists only to compile/link and to expose failure handling. Never ship it.
- `build`: CMake static library, four ABIs × Debug/Release, Android API 21.
- `matrix`: compile `public_api.cpp` at 38 Unity/version-patch selections; also
  compile all 17 source files at six representative selections. This is not a
  complete ABI × Unity × feature × build-type cross-product.
- `variants`: compile synchronization enabled, `.NET 3.5` Dictionary, minimal
  features, and an intentionally unsupported Unity version; adds custom-class,
  UnityEvent, and coroutine template instantiations from `extended_api.cpp`.
- `integration`: consumer shared-library link through CMake; ndk-build static
  library on ARM64 Debug/Release with exceptions and RTTI enabled.
- `analyze`: clang-tidy's `clang-analyzer-*` checks for every `.cpp`, no fixes.
- `probes`: instrument actual BNM sources and fixtures with ASan/UBSan. A small
  synthetic API table replaces Unity services, including a malloc-backed fixed
  allocator. These probes do not validate Unity GC, metadata ABI, or real hooks.
- `scanner`: extract `AssemblerUtils` from the snapshot, rename architecture
  preprocessor symbols, and exercise the x86_64 algorithm on ARM64 with signed
  `char`. It does not execute x86 machine code or validate an x86 device.

## Interpret results

Each command's argv, working directory, exit code, duration, and diagnostic
counts are saved in `build/audit-phase23/logs/*.json`; full output is in `.log`.
The orchestration script continues after failures so the audit can collect all
results. **Its own exit code is not a test-suite verdict.** Inspect each record.
Compiler-warning counts include repeated diagnostics from shared headers.

The original syntax check is expected to fail while the backend is unselected.
Unsupported Unity 6006 should fail at the version selector. The positive probe
should return 0. Other probes assert desired behavior or expose a known bug:
exit 1 means a behavioral mismatch, and a sanitizer abort is evidence to inspect,
not a passing test. Native probes use 20-second timeouts. Leak detection is
disabled because these synthetic fixtures do not model full VM shutdown;
there is no claim of leak freedom or ThreadSanitizer coverage.

The findings and observed outcomes are recorded in [REVIEW.md](../../REVIEW.md).
Generated snapshots and raw logs stay under ignored `build/`; `results.json`
preserves a compact record of the audited session and its diagnostic excerpts.

## After-refactor verification

Use `--mode regression` and the final gate documented in [tests/README.md](../../tests/README.md).
The historical `probes` cases intentionally expose pre-refactor failures and are
not the current pass/fail suite. `refactor-results.json` records the subsequent
implementation checks separately from `results.json`. No hook stub is suitable
for a Unity consumer.

The current gate also requires standalone public-header checks, fixed-width integer
assertions on all four ABIs, compiler-observed Unity configuration/header records,
and a verification-input manifest. Use a fresh output directory with the current
runner; historical result directories do not satisfy the new gate. See [TODO.md](../../TODO.md).
