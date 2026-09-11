# AGENTS.md

BNM (ByNameModding) is a C++20 static library for modding il2cpp Unity games on Android. This repo is a fork of ByNameModding/BNM-Android. It is a library, not an app: there are no tests, no CI, and no entrypoint — consumers build it into a game's shared library. `.clang-format` and `.clang-tidy` exist but nothing runs them automatically; the only build inputs are `Android.mk` and `CMakeLists.txt`.

## Build & verification

- Android-only (NDK) build, C++20 required. `Android.mk` (ndk-build) and `CMakeLists.txt` define the same STATIC library from the same 17 files in `src/` — keep both file lists in sync when adding/removing a `.cpp`.
- The checked-in tree does **not** compile by design: the dummy hooking backend in `include/BNM/UserSettings/GlobalSettings.hpp` ends with `static_assert(false, "No hooking software!")`. Integrators replace it with the Dobby or ShadowHook block (both pre-written, commented out just above it in that same file).
- Verified per-file syntax check: temporarily comment out that static_assert, then run per file (e.g. `src/Utils.cpp`):
  `aarch64-linux-android-clang++ -std=c++20 -fsyntax-only -Iinclude -Iexternal/include -Iexternal -Iexternal/utf8 -Isrc/private src/<File>.cpp`
  Revert the temporary edit before committing. The committed file already selects a `UNITY_VER`, so no version setup is needed just to run the check.
- CMake adds extras beyond `Android.mk`: options `BNM_LINK_LOG` (link `liblog.so`) and `BNM_SANITIZERS` (ASan+UBSan), plus stricter warnings (`-Wconversion -Wsign-conversion -Wnon-virtual-dtor -Woverloaded-virtual`). Warnings are kept at zero — don't introduce new ones.

## Unity version selection (core mechanism)

- `GlobalSettings.hpp`: uncomment exactly one `UNITY_VER`; for Unity 2022.x+ / Unity 6 (6000.x) also set `UNITY_PATCH_VER` to the game's patch (e.g. `2022.3.20f1` -> 20). The committed tree ships with `222` active as a default.
- `include/BNM/Il2CppHeaders.hpp` maps (UNITY_VER, UNITY_PATCH_VER) to a header in `include/BNM/Il2CppHeaders/`. Unity < 2022: one header per minor, no patch needed (exception: 2021.1 — set patch 24 for 2021.1.24f1+). 2022.x+ / 6000.x: per-patch headers with threshold rules; follow the tables in `GlobalSettings.hpp` and `Il2CppHeaders.hpp`, don't guess.
- f2/f3 hotfix builds use the f1 layout of the same patch number. Unmapped minor versions fail with `static_assert(... "Setup UNITY_VER in GlobalSettings.hpp")`.
- `include/BNM/Il2CppApi/*.h` are per-version il2cpp function lists, reference only — never included automatically (don't confuse them with `Il2CppHeaders/`).

## Conventions

- Library version string `BNM_VER` (currently `2.5.3`) lives in `GlobalSettings.hpp`.
- Commits: conventional-commit prefixes for normal work (`fix:`, `refactor:`, `docs:`, `feat:`); version bumps/releases use `vX.Y.Z: description`.
- `GlobalSettings.hpp` (and a few `UnityStructures` headers, e.g. `Quaternion.hpp`, `Matrix4x4.hpp`) carry bilingual EN+RU comments — keep both languages when editing.
- Style source of truth: `.clang-format` (LLVM base, 4-space indent, 120 col, Allman braces, regrouped includes) and `.clang-tidy` (bugprone / clang-analyzer / modernize / performance / identifier-naming). Run them manually — no hook enforces them.
- Debug features (`BNM_DEBUG`, `BNM_ALLOW_SELF_CHECKS`, etc.) are gated by `#ifndef NDEBUG` in `GlobalSettings.hpp`, so they are automatically off in release builds.
- Feature flags `BNM_CLASSES_MANAGEMENT`, `BNM_COROUTINE`, `BNM_USE_IL2CPP_ALLOCATOR` are enabled by default in the checked-in `GlobalSettings.hpp`; `BNM_COROUTINE` requires `BNM_CLASSES_MANAGEMENT`.
- Full API reference is `docs/DOCS.md` (written in Indonesian); `README.md` links to it. The user-facing site bynamemodding.github.io is a separate project.
