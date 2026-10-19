# ByNameModding (BNM) for Android

BNM is a C++20 static library for working with Unity IL2CPP classes, methods, and
fields by name. Link it into your Android native shared library; the repository
does not provide an application or a `JNI_OnLoad` entry point.

This is a personal learning and experimental fork. The upstream project is
[ByNameModding/BNM-Android](https://github.com/ByNameModding/BNM-Android).

## Core features

- Class/image lookup, overload and generic resolution, method calls, fields,
  properties, events, and delegates.
- Managed strings, arrays, lists, dictionaries, and Unity math structures.
- Method hooking through a consumer-supplied backend.
- Custom class creation and method modification through ClassesManagement.
- C++20 coroutines exposed as managed enumerators.

## Integration

1. Use an Android NDK toolchain with C++20, exceptions, and RTTI enabled.
   The recorded verification used NDK 29 (Clang 21); no minimum NDK release has
   been established by the audit.
2. Configure [GlobalSettings.hpp](include/BNM/UserSettings/GlobalSettings.hpp)
   for the target player's Unity version and patch. The committed `222` / `32`
   values are placeholders. The [selector](include/BNM/Il2CppHeaders.hpp) maps
   headers from Unity 5.6 through 6000.5; this is layout selection coverage,
   not proof of runtime compatibility across that range.
3. Enable exactly one Dobby or ShadowHook block and disable the Dummy block,
   including its intentional `#error`. Supply the backend header and library;
   neither backend is bundled. Test stubs are unsuitable for game integration.
4. Add BNM to CMake or ndk-build and link it into your shared library. Follow the
   [build setup](docs/DOCS.md#2-setup-proyek) for include paths and link settings.
5. Register callbacks and configure loading before installing hooks. A successful
   `TryLoadBy*` means the initial hook was installed. Use `BNM::IsLoaded()` or
   `AddOnLoadedEvent` before accessing runtime APIs. See the
   [loading guide](docs/DOCS.md#5-cara-memuat-loading-bnm).

## Refactor migration

Rebuild BNM and all consumers together. Typed `cast<T>()` on method, field,
property, and event wrappers now returns a value; coroutine handles are
move-only. Existing-class field growth and reparenting are rejected; new-class
creation and method modification remain available. Read the
[migration guide](docs/REFACTOR.md) for ownership, threading, and failure contracts.

## Verification

Recorded post-refactor checks passed: eight CMake builds (four ABIs ×
Debug/Release at API 21), 20 native ASan/UBSan regression cases, three decoder
cases, and 38 Unity selector checks. A consumer shared-library link and ARM64
ndk-build Debug/Release also passed. Full source checks cover six representative
Unity selections, not every ABI/version/feature combination.

These checks use synthetic runtime fixtures and a hook stub that always fails.
Real Unity loading, backend behavior, GC/finalization, and runtime-owned allocation
failures remain unverified. Native metadata now has per-descriptor rollback; managed
stores use write barriers and `ScopedIl2CppThread` owns native thread attachments.
See the [updated contracts](docs/REFACTOR.md) for scope and migration requirements. API 21 is the test target,
not a newly imposed minimum. Reproduce the checks using the
[test guide](tests/README.md); inspect the [audit report](REVIEW.md) and
[recorded results](tools/audit/refactor-results.json) for evidence and limitations.

## Documentation and layout

| Resource | Purpose |
|---|---|
| [API guide (Indonesian)](docs/DOCS.md) | Configuration, API examples, troubleshooting, verification scope |
| [Migration guide](docs/REFACTOR.md) | Core refactor behavior and consumer changes |
| [Native tests](tests/README.md) | Reproducible regression and integration commands |
| [Audit tooling](tools/audit/README.md) | Baseline snapshots and diagnostic records |
| `include/BNM/` | Public APIs, settings, version-specific IL2CPP headers |
| `src/`, `src/private/` | Implementation and internal runtime integration |
| `external/` | Bundled UTF8-CPP and custom RTTI dependencies |

Bundled dependencies include [UTF8-CPP](https://github.com/nemtrif/utfcpp) and
[Open-hierarchy custom RTTI](https://github.com/royvandam/rtti/tree/cf0dee6fb3999573f45b0726a8d5739022e3dacf).

[Upstream online documentation](https://bynamemodding.github.io/) describes the
upstream project; use this fork's migration guide for changed contracts.
