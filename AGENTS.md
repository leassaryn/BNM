# AGENTS.md

BNM (ByNameModding) is a C++20 static library for modding il2cpp Unity games on Android. This repo is a fork of ByNameModding/BNM-Android. It is a library, not an app: there are no tests, CI, linter, or entrypoint — consumers build it into a game's shared library.

## Build & verification

- Android-only (NDK) build, C++20 required. `Android.mk` (ndk-build) and `CMakeLists.txt` define the same STATIC library from the 17 files in `src/` — keep both file lists in sync when adding a `.cpp`.
- The checked-in tree does **not** compile by design: the dummy hooking backend in `include/BNM/UserSettings/GlobalSettings.hpp` contains `static_assert(false, "No hooking software!")` (added in v2.5.0). Integrators replace it with the Dobby or ShadowHook block (both pre-written, commented out in that same file).
- Verified syntax check: temporarily comment out that static_assert, then run per file (e.g. `src/Utils.cpp`):
  `aarch64-linux-android-clang++ -std=c++20 -fsyntax-only -Iinclude -Iexternal/include -Iexternal -Iexternal/utf8 -Isrc/private src/<File>.cpp`
  Revert the temporary edit before committing.

## Unity version selection (core mechanism)

- `include/BNM/UserSettings/GlobalSettings.hpp`: uncomment exactly one `UNITY_VER`; for Unity 2022.x+ / Unity 6 (6000.x) also set `UNITY_PATCH_VER` to the game's patch (e.g. `2022.3.20f1` -> 20).
- `include/BNM/Il2CppHeaders.hpp` maps (UNITY_VER, UNITY_PATCH_VER) to a header in `include/BNM/Il2CppHeaders/`. Unity < 2022: one header per minor, no patch needed (exception: 2021.1 — set patch 24 for 2021.1.24f1+). 2022.x+ / 6000.x: per-patch headers with threshold rules; follow the table in GlobalSettings.hpp, don't guess.
- f2/f3 hotfix builds use the f1 layout of the same patch number. Unmapped minor versions fail with `static_assert(... "Setup UNITY_VER in GlobalSettings.hpp")`.
- `include/BNM/Il2CppApi/*.h` are per-version il2cpp function lists, reference only — never included automatically.

## Conventions

- Library version string `BNM_VER` lives in `GlobalSettings.hpp`; commit/tag style is `vX.Y.Z: description`.
- `GlobalSettings.hpp` (and a few `UnityStructures` headers) carry bilingual EN+RU comments — keep both languages when editing.
- Debug features (`BNM_DEBUG`, `BNM_ALLOW_SELF_CHECKS`, etc.) are gated by `#ifndef NDEBUG` in GlobalSettings.hpp, so they are automatically off in release builds.
- Feature flags `BNM_CLASSES_MANAGEMENT`, `BNM_COROUTINE`, `BNM_USE_IL2CPP_ALLOCATOR` are enabled by default in the checked-in GlobalSettings.hpp; `BNM_COROUTINE` requires `BNM_CLASSES_MANAGEMENT`.
- User-facing docs live at bynamemodding.github.io (separate project); there is no `docs/` in this repo.
