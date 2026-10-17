# Native regression checks

Run on Android ARM64 (for example Termux) with Python 3, Android NDK, CMake and
Ninja. `ANDROID_NDK_HOME` must identify the NDK. No Unity player or JVM is
provided by these fixtures. The hook stub deliberately fails every installation.

## Prepare and test

From the repository root, with `cmake` and `ninja` on PATH:

```sh
python3 tools/audit/baseline.py --mode prepare --out build/regression
python3 tools/audit/baseline.py --mode regression --out build/regression
python3 tools/audit/baseline.py --mode scanner --out build/regression
```

Pass `--cmake /absolute/path/to/cmake --ninja /absolute/path/to/ninja` if needed.
Preparation copies production inputs into an isolated snapshot and activates the
existing Dobby block there only. It deliberately records the unconfigured
production syntax failure first. Use a fresh `--out` after source changes;
preparation refuses to overwrite a snapshot.

`regression` builds the actual 17-source library with ASan/UBSan, compiles
`core_regressions.cpp`, and returns nonzero on any compile or runtime failure.
Each case runs in a separate process. Array allocation-failure injection uses
the fixture API table. A linker `--wrap=malloc` probe also fails the first metadata allocation; it is not a full failure sweep. Class synchronization tests model a blocking runtime
initializer; they are not evidence of real IL2CPP locking or a TSan run.

The scanner test extracts the actual implementation and selects x64 decoding
on the ARM64 host. It checks signed bytes, backward relative calls, null entry
and a mapping boundary. It is not execution of a native x86 binary.

## Compatibility and integration checks

```sh
python3 tools/audit/baseline.py --mode build --out build/regression
python3 tools/audit/baseline.py --mode matrix --out build/regression
python3 tools/audit/baseline.py --mode variants --out build/regression
python3 tools/audit/baseline.py --mode integration --out build/regression
python3 tools/audit/baseline.py --mode analyze --out build/regression
python3 tests/check_results.py build/regression
```

Build covers four ABIs in Debug/Release at API 21. Matrix checks all 38 selector
branches, with all source files at six representative boundaries. Variants
include synchronization, DOTNET35, disabled classes/coroutines and an intentionally
unsupported Unity selector. Integration links a consumer shared library and runs
ndk-build Debug/Release. Analyzer requires the CMake Debug compile database.

Historical `probes` mode is deliberately crash-oriented and is retained for the
before-refactor record; do not interpret it as a current regression suite.
Audit modes other than `regression` collect failures without stopping. The final
`check_results.py` command enforces expected outcomes and input freshness.
Raw commands, exit codes and output are retained under `<out>/logs/`.

## Runtime work still required

Use a consumer Unity project to test Dobby/ShadowHook installation and rollback,
normal/late initialization, overloads/generics/value types on each shipped ABI,
CheckJNI and split APK loading, coroutine collection/finalization, and incremental
GC write barriers. Keep metadata writers at an application-controlled safe point.
Leak detection is disabled for native fixtures because published IL2CPP metadata
has process lifetime. Full allocation-failure rollback is not covered.
