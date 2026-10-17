# AGENTS.md

BNM (ByNameModding) is a C++20 static library for modding il2cpp Unity games on Android. It is a library, not an app: there is no entrypoint, no tests, and no CI (no `.github/`, no active git hooks). Consumers compile it into the game's shared library.

This clone is a personal fork used for C++ learning and experiments. The stable upstream is `ByNameModding/BNM-Android`. Work on `feat/refactor` is experimental.

## Build

- Android only (NDK), C++20 required. `$ANDROID_NDK_HOME` is set in this environment; the NDK clang lives under `toolchains/llvm/prebuilt/*/bin/`.
- The tree does **not** compile as checked in. `GlobalSettings.hpp` ends its dummy backend section with `#error "No hooking backend selected!..."`. Integrators uncomment the Dobby or ShadowHook block above it, and must supply that library's header (`dobby.h` / `shadowhook.h`). Neither is in the repo. The `#error` text mentions `XOFHook`, but no such block exists.
- Sources are listed twice: `CMakeLists.txt` and `Android.mk` both define the same 17 files in `src/`. Keep them in sync when adding or removing `.cpp`.
- `Android.mk` does **not** export include paths. Consumers must add `include/`, `external/include/`, `external/`, and (for BNM's own build) `external/utf8/` and `src/private/` themselves. CMake exposes the first three as PUBLIC.
- CMake options: `BNM_LINK_LOG` (link `liblog`), `BNM_SANITIZERS` (ASan + UBSan, applied PUBLIC).
- Single-file syntax check: `$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-arm64/bin/aarch64-linux-android21-clang++ -std=c++20 -fsyntax-only -Iinclude -Iexternal/include -Iexternal -Iexternal/utf8 -Isrc/private src/Class.cpp`. It fails with the `#error` above until a backend is selected.
- Style: `.clang-format` (LLVM base, 4-space indent, 120 cols, Allman braces) and `.clang-tidy`. Run them manually; nothing enforces them.

## Unity version selection (`include/BNM/UserSettings/GlobalSettings.hpp`)

- Uncomment exactly one `#define UNITY_VER` (the file's default is `222`, i.e. 2022.2.x).
- For 2022.x and newer (including 6000.x), also set `UNITY_PATCH_VER` to the game's patch number. The committed value (`32`) is a placeholder, not the game's patch. Example: `2022.2.12f1` with `UNITY_VER 222` needs `UNITY_PATCH_VER 12`.
- `Il2CppHeaders.hpp` maps (`UNITY_VER`, `UNITY_PATCH_VER`) to a file in `include/BNM/Il2CppHeaders/`. The full table is in the doc comment of `GlobalSettings.hpp` and in `Il2CppHeaders.hpp`. Update both when changing mappings.
- Unmapped versions (6000.6+) fail on purpose with `static_assert`.

## Feature flags and other settings

- Enabled by default in `GlobalSettings.hpp`: `BNM_CLASSES_MANAGEMENT`, `BNM_COROUTINE` (requires ClassesManagement), `BNM_USE_IL2CPP_ALLOCATOR`.
- Debug-only checks (`BNM_ALLOW_SELF_CHECKS`, `BNM_CHECK_INSTANCE_TYPE`, logging macros) are enabled only when `NDEBUG` is not defined.
- `BNM_VER` in the same file is the version string.
- Many comments in `GlobalSettings.hpp` and some UnityStructures headers are bilingual (EN + RU). Keep both languages when editing them.

## Conventions

- Commits: `fix:`, `refactor:`, `docs:`, `feat:`, `chore:`. Releases use `vX.Y.Z: description`.
- `CHANGELOG.md` uses Keep a Changelog format.
- `docs/DOCS.md` is the full API reference, written in Indonesian. `README.md` is in English.
- `.gitignore` excludes `.agents/`, `.opencode`, `build/`, and `.env`.
