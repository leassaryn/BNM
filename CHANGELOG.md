# Changelog

All notable changes to BNM (ByNameModding) will be documented in this file.

Format based on [Keep a Changelog](https://keepachangelog.com/).

---

## [2.5.4-dirty] - 2026-09-20

### Fixed
- **`IsAllocated()` UB**: replaced `SIGSEGV` + `longjmp` (undefined behavior) with `mincore()` (async-signal-safe)
- **JNI pointer leak**: `ReleaseStringUTFChars` now passes correct pointer (`dirChars` instead of `cDir.data()`)
- **JNI local ref leak**: `DeleteLocalRef` moved before null check in `TryLoadByJNI`
- **ModifyClass/CreateClass use-after-free**: removed dangling `field->myInfo` writes
- **Split APK path**: `rfind("/lib/")` instead of `substr(len-8)` for robustness
- **Delegate::Remove**: fixed `memmove` ordering, removed dead `max_length` check
- **Loading mutex**: `shared_lock` → `unique_lock` for write operations
- **Null dereferences**: added guards for assembly, FieldInfo, vtable methods
- **CompileTimeClass const**: `const_cast` → temporary copy (correct const-correctness)
- **Parameter count guard**: uint8_t overflow check in `ProcessCustomMethod`/`CreateMethod`
- **Orphaned `#undef kls`**: removed
- **Duplicate ShadowHook block**: removed from `GlobalSettings.hpp`
- **Missing `<cmath>`**: added in Vector2 and Color headers
- **Compiler warnings**: eliminated all warnings with `-Wall -Wextra -Wpedantic`

### Changed
- **`Class::TryInit`**: added `_initialized` flag to skip redundant `Class$$Init` calls
- **`List::GrowIfNeeded`**: geometric growth (2x) instead of linear
- **`List::CopyFrom`**: skip old data copy when resizing
- **`TryInvoke`**: static `voidType`/`voidClass`, `memset` → value-init
- **Named constants**: `BNM_METHOD_PUBLIC`, `BNM_METHOD_STATIC`, etc. for il2cpp flags
- **`static_assert(false)` → `#error`**: clearer error message, no C++20 issues
- **C++20 concepts**: `enable_if_t` → `requires` across codebase
- **Modern casts**: C-style casts → `reinterpret_cast`/`static_cast`/`const_cast`
- **`constexpr` methods**: `IsValid()`, `Alive()`, `operator bool()` now compile-time evaluable
- **Move semantics**: added to `ForwardList`, `MethodBase`, `FieldBase`, `EventBase`, `PropertyBase`
- **`[[nodiscard]]`**: added to public API methods with return values
- **Type aliases**: `typedef` → `using` throughout codebase

### Added
- `docs/DOCS.md`: comprehensive API reference (Indonesian)
- `AGENTS.md`: developer guide for AI agents
- `.clang-format`: LLVM base, 4 indent, 120 col, Allman braces
- `.clang-tidy`: bugprone, clang-analyzer, modernize, performance checks
- `CMakeLists.txt`: sanitizer options (`BNM_SANITIZERS`), stricter warnings
- `BNM::to_underlying`: C++23 polyfill for enum → integer conversion
- `PRIVATE_INTERNAL::GetMethodParam`/`GetPropertySetParam`: unified parameter access helpers
- Unity math structs: modernized operators, constexpr, W2S correctness verified

### Removed
- Orphaned `#undef kls` in `ClassesManagement.cpp`
- Duplicate ShadowHook block in `GlobalSettings.hpp`
- Redundant casts across multiple files

---

## [2.5.3] - 2026-09-XX

### Changed
- Full C++20 modernization: concepts, constexpr, move semantics, RAII
- Build system: CMake improvements, Android.mk cleanup
- Code style: `.clang-format` and `.clang-tidy` added

---

## [2.5.2] - Unknown

### Fixed
- Critical bugs, UB, memory leaks, and thread safety
- Unity struct math operations
- W2S (WorldToScreen) correctness

---

*This changelog tracks changes from v2.5.3 onwards. Earlier versions are not documented.*
