# AGENTS.md

BNM (ByNameModding) is a C++20 static library for modding il2cpp Unity games on Android. It is a library, not an app: there are no tests, no CI, and no entrypoint — consumers build it into a game's shared library.

## Build

- Android-only (NDK), C++20 required.
- `Android.mk` (ndk-build) and `CMakeLists.txt` define the same STATIC library from 17 files in `src/` — keep both in sync when adding/removing `.cpp`.
- The checked-in tree does **not** compile by design: the dummy hooking backend in `GlobalSettings.hpp` ends with `#error "No hooking backend selected!"`. Integrators uncomment one of the Dobby or ShadowHook blocks above it.
- CMake options: `BNM_LINK_LOG` (link `liblog.so`), `BNM_SANITIZERS` (ASan+UBSan).
- Style: `.clang-format` (LLVM, 4 indent, 120 col, Allman braces), `.clang-tidy` (bugprone/modernize/performance). Run manually — no hooks.

## Unity Version

- `GlobalSettings.hpp`: uncomment one `UNITY_VER`; for 2022.x+ also set `UNITY_PATCH_VER`.
- `Il2CppHeaders.hpp` maps (UNITY_VER, UNITY_PATCH_VER) to the correct header in `Il2CppHeaders/`.
- Default: `222` (2022.2.x).

## Conventions

- Version string: `BNM_VER` in `GlobalSettings.hpp` (currently `2.5.4-dirty`).
- Commits: `fix:`, `refactor:`, `docs:`, `feat:`; releases use `vX.Y.Z: description`.
- `GlobalSettings.hpp` and some UnityStructures headers have bilingual EN+RU comments — keep both.
- Feature flags (`BNM_CLASSES_MANAGEMENT`, `BNM_COROUTINE`, `BNM_USE_IL2CPP_ALLOCATOR`) enabled by default.
- Full API reference: `docs/DOCS.md` (Indonesian).
