# Changelog

All notable changes to BNM (ByNameModding) will be documented in this file.

Format based on [Keep a Changelog](https://keepachangelog.com/).

---

## [Unreleased]

### Fixed
- List growth uses bounded unsigned arithmetic and signed-safe capacity checks for legacy Unity array layouts.
- Fixed-width managed integer mappings on 32-bit ABIs, proxy-to-proxy array/list assignments, and stale field bindings after rejected instances.
- Single-delegate addition/removal, dirty UnityEvent runtime listeners, and null collider resolver/handle failures.
- Matrix self-multiplication, TRS rotation extraction, and nonterminating angle normalization for large/non-finite inputs.
- Unterminated string_view diagnostic inputs and standalone public-header includes.
- Audit matrix settings are applied strictly and checked against compiler-selected macros and header hashes.
- Managed reference stores in fields, arrays, lists, delegates and coroutine proxies use GC write barriers; value-type copies mark embedded reference fields when runtime metadata is available.
- Per-descriptor metadata transactions roll back native allocation failures, owner changes, method/vtable edits and newly published assemblies; old buffers are freed only after commit.
- Virtual override lookup and vtable hooks resolve metadata slots and exact interface offsets, avoiding collisions with hidden methods and explicit interface implementations.
- Invalid virtual hook targets and null replacements leave the original-output pointer unchanged; checked method rebinding clears stale instances and accepts resolved virtual interfaces.
- Independent ownership for copied compile-time class descriptors and generic arguments.
- Runtime class initialization publication, internal cache synchronization, null-symbol scanning, signed x86 decoding, and hook failure propagation.
- JNI error checks, local-reference cleanup, and installed split-APK discovery.
- Array size overflow, collection allocation failures, removed dictionary entries, and invalid vector type punning.
- Coroutine frame ownership, null-yield scheduling, completion, and native callback construction.
- Custom metadata counter validation and checked allocation; runtime-owned state and backend unhook failures remain outside native rollback.

### Changed
- `NewListBNM<T>()` now uses the runtime-backed `NewList<T>()` path; unsafe synthetic open-generic vtables and metadata caches were removed. Native-only types without a runtime instantiation are unsupported.
- x86/x64 loader scanning follows instruction boundaries and fails closed on unsupported instructions/control flow.
- Android.mk exports public include directories to consumers.
- Typed `cast()` returns a value; coroutine return wrappers are move-only. Rebuild all consumers and see `docs/REFACTOR.md`.
- Existing class instance-layout growth/reparenting and method calls with mismatched argument counts are rejected.
- Debug pointer checks inspect readable mappings; this does not establish object lifetime.

### Added
- `ScopedIl2CppThread`, a non-copyable, thread-affine attachment guard with nested/borrowed attachment support.
- Managed store/copy helpers, required GC barrier symbol resolution, native OOM sweeps and thread/barrier regression fixtures.
- Native core regression fixtures and before/after audit tooling, with separate Unity runtime verification requirements.

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
