# BNM (ByNameModding) Code Review - Findings & Mitigations

**Reviewer:** opencode (C++ Pro)
**Date:** 2026-09-20
**Scope:** All core features excluding il2cpp headers and il2cpp API files
**Severity Levels:** Critical > High > Medium > Low > Info

---

## Critical Issues

### C1: `IsAllocated()` Uses SIGSEGV + longjmp (Undefined Behavior)

**File:** `include/BNM/Utils.hpp:39-63`
**Code:**
```cpp
static thread_local jmp_buf jump;
[[maybe_unused]] static thread_local volatile sig_atomic_t jumpRequested = 0;
auto signalHandler = [](int) { jumpRequested = 1; longjmp(jump, 1); };
char c;
bool ok = true;
sighandler_t old_handler = signal(SIGSEGV, signalHandler);
jumpRequested = 0;
if (!setjmp(jump)) {
    c = *(volatile char *) x;
    (void)c;
} else {
    ok = false;
}
signal(SIGSEGV, old_handler);
return ok;
```

**Problem:**
1. `longjmp` from a signal handler is undefined behavior per C++ standard (§18.10.4.4). Only `_Exit`, `quick_exit`, or async-signal-safe functions are allowed.
2. `setjmp`/`longjmp` with non-volatile local variables is UB if any were modified between setjmp and longjmp.
3. Not async-signal-safe — calling `signal()` itself from within the handler context is problematic.
4. Thread-local `jmp_buf` is correct for thread safety, but the pattern itself is UB.
5. `volatile sig_atomic_t jumpRequested` is written but never meaningfully read (the `if (!setjmp(jump))` path doesn't check it).

**Impact:** Crash, stack corruption, or silent incorrect behavior on ARM64/ARM when accessing invalid memory.

**Mitigation:**
```cpp
// Option A: Use mincore() for page-level validity check (Linux/Android)
#include <sys/mman.h>

template <typename T>
    requires std::is_pointer_v<T>
inline bool IsAllocated(T x) {
    if (!x) return false;
#ifdef BNM_ALLOW_SAFE_IS_ALLOCATED
    // Check if address is in a valid memory region using /proc/self/maps
    // or use mincore() which is async-signal-safe
    unsigned char vec;
    // mincore returns 0 if page is mapped, -1 with errno EINVAL if not page-aligned
    // Round down to page boundary
    uintptr_t addr = reinterpret_cast<uintptr_t>(x);
    uintptr_t pageAddr = addr & ~(getpagesize() - 1);
    int result = mincore(reinterpret_cast<void*>(pageAddr), 1, &vec);
    return result == 0;
#else
    return x != nullptr;
#endif
}

// Option B: Use Android's android_get_exported_namespace() or
// read /proc/self/maps (slower but reliable)
```

Or simply remove the signal-based approach and rely on the null check, since the null check is the most common failure mode.

---

### C2: JNI `ReleaseStringUTFChars` Wrong Pointer Argument

**File:** `src/Loading.cpp:136`
**Code:**
```cpp
auto dirChars = env->GetStringUTFChars(jDir, nullptr);
// ...
auto cDir = std::string_view(dirChars);
// ...
FINISH:
env->ReleaseStringUTFChars(jDir, cDir.data()); // BUG: cDir.data() may not equal dirChars
```

**Problem:**
- `GetStringUTFChars` returns a `const char*` pointer (`dirChars`).
- `cDir` is a `std::string_view` initialized from `dirChars`. While `string_view::data()` should return the same pointer in this case, it's not guaranteed by the standard if the string_view was modified.
- More critically, if any string operation modifies the view (which doesn't happen here but is fragile), the pointer would be wrong.
- `ReleaseStringUTFChars` requires the exact pointer returned by `GetStringUTFChars`.

**Impact:** JNI reference leak, potential crash if pointer doesn't match.

**Mitigation:**
```cpp
FINISH:
env->ReleaseStringUTFChars(jDir, dirChars);  // Use original pointer
env->DeleteLocalRef(jDir);
```

---

## High Severity Issues

### H1: Use-After-Free Risk in `ModifyClass()` Field Offset Write

**File:** `src/ClassesManagement.cpp:155-163`
**Code:**
```cpp
for (auto field : customClass->_fields) {
    SetupField(newField, field);
    newField->parent = klass;

    field->offset = currentAddress;  // field points to customClass->_fields[i]
    newField->offset = static_cast<int32_t>(currentAddress);

    currentAddress += field->_size;

    newField++;
}
```

**Problem:**
- `field` is a pointer to `CustomField` objects stored in `customClass->_fields` vector.
- After `ModifyClass()` returns (line 177-187 in `ProcessClassRuntime`), the vectors are cleared: `customClass->_fields.clear(); customClass->_fields.shrink_to_fit();`
- While `field->offset` is written before the function returns, the `CustomField` objects pointed to by the vector elements are destroyed when the vector is cleared.
- The `field->myInfo` assignment on line 161 (`field->myInfo = newField`) stores a pointer that becomes invalid after vector clear.

**Impact:** `field->myInfo` is a dangling pointer after `ProcessClassRuntime()`. Any access to it later is UB.

**Mitigation:**
The `myInfo` field on `CustomField` is only used during `ProcessClassRuntime` for logging and validation. After that, it's not needed. The fix is to ensure no code accesses `field->myInfo` after the function returns. Alternatively, store the offset info in a separate structure that outlives the vector:

```cpp
// Better: Don't store myInfo on CustomField at all for the "modify" path
// It's only needed during processing. Use a local map instead:
std::unordered_map<CustomField*, IL2CPP::FieldInfo*> fieldInfoMap;
```

Or simply remove the `field->myInfo` assignment since it's never used after `ModifyClass` returns.

---

### H2: `Class::TryInit()` Called Redundantly on Every Method Call

**File:** `include/BNM/Class.hpp:326`, `src/Class.cpp:378`
**Code:**
```cpp
// Class.hpp
inline BNM::IL2CPP::Il2CppClass *GetClass() const { TryInit(); return _data; }

// Class.cpp - EVERY method calls TryInit()
MethodBase Class::GetMethod(...) const {
    BNM_LOG_ERR_IF(!_data, ...);
    if (!_data) return {};
    TryInit();  // Called here
    // ...
}

FieldBase Class::GetField(...) const {
    BNM_LOG_ERR_IF(!_data, ...);
    if (!_data) return {};
    TryInit();  // And here
    // ...
}
```

**Problem:**
- `TryInit()` calls `il2cppMethods.Class$$Init(_data)` which initializes the class.
- This is called on EVERY method invocation: `GetMethod`, `GetField`, `GetParent`, `GetArray`, `GetPointer`, `GetReference`, `GetGeneric`, `GetIl2CppType`, `GetMonoType`, `GetCompileTimeClass`, `GetImage`, `CreateNewInstance`, `GetInnerClasses`, `GetFields`, `GetMethods`, `GetProperties`, `GetEvents`, `GetInnerClass`, `GetProperty`, `GetEvent` — **20+ methods**.
- `Class$$Init` is likely idempotent after first call, but it's still a function call with potential mutex/atomic operations inside il2cpp.

**Impact:** Performance overhead — every BNM operation pays the cost of `Class$$Init`.

**Mitigation:**
```cpp
// Option A: Cache initialization state
struct Class {
    IL2CPP::Il2CppClass *_data{};
    mutable uint8_t _initialized : 1 = 0;

    void TryInit() const {
        if (_data && !_initialized) {
            Internal::il2cppMethods.Class$$Init(_data);
            _initialized = 1;
        }
    }
};

// Option B: Initialize in constructor only
Class(const IL2CPP::Il2CppClass *_class) : _data(const_cast<IL2CPP::Il2CppClass *>(_class)) {
    if (_data) Internal::il2cppMethods.Class$$Init(_data);
}
```

Option B is cleaner — initialize once at construction time. The `TryInit` in each method becomes a no-op.

---

### H3: `MemFnToVoidPtr()` — Member Function Pointer to void* is Implementation-Defined

**File:** `src/Coroutine.cpp:32-36`
**Code:**
```cpp
static inline void *MemFnToVoidPtr(const void *memPtr) {
    void *result;
    memcpy(&result, memPtr, sizeof(result));
    return result;
}
```

**Problem:**
- Member function pointers can be larger than `sizeof(void*)` (e.g., 16 bytes for virtual inheritance on some ABIs).
- `memcpy(&result, memPtr, sizeof(result))` only copies `sizeof(void*)` bytes, potentially truncating the data.
- This is implementation-defined behavior — works on Itanium ABI (ARM/x86) but not guaranteed.

**Impact:** Works on Android (ARM64/ARM) but could break on other ABIs or with virtual inheritance.

**Mitigation:**
```cpp
// Use the actual size of the member function pointer
template<typename T>
static inline void *MemFnToVoidPtr(T memPtr) {
    static_assert(sizeof(T) <= sizeof(void*),
        "Member function pointer exceeds void* size — not supported on this ABI");
    void *result = nullptr;
    memcpy(&result, &memPtr, sizeof(T));
    return result;
}
```

Or store the member function pointer in a union:
```cpp
union FuncPtrStorage {
    void *asVoid;
    decltype(&IEnumerator::MoveNext) asMemberFunc;
};

static inline void *MemFnToVoidPtr(void *FuncPtrStorage::*memberPtr) {
    FuncPtrStorage storage;
    storage.*memberPtr = memberPtr;
    return storage.asVoid;
}
```

---

### H4: Split APK Path Construction Fragile

**File:** `src/Loading.cpp:129-133`
**Code:**
```cpp
if (cDir.length() >= 8) {
    file = std::string(cDir).substr(0, cDir.length() - 8) + BNM_OBFUSCATE_TMP("split_config." CURRENT_ARCH ".apk!/lib/" CURRENT_ARCH "/libil2cpp.so");
    handle = BNM_dlopen(file.c_str(), RTLD_LAZY);
}
```

**Problem:**
- Assumes `cDir` ends with 8 characters (the `/lib/arm64-v8a/libil2cpp.so` suffix is actually much longer than 8 chars).
- The calculation `cDir.length() - 8` is wrong. The actual suffix is `/lib/<arch>/libil2cpp.so` which varies by architecture.
- For `armeabi-v7a`: `/lib/armeabi-v7a/libil2cpp.so` = 29 chars
- For `arm64-v8a`: `/lib/arm64-v8a/libil2cpp.so` = 27 chars

**Impact:** Path construction fails silently, `dlopen` returns nullptr.

**Mitigation:**
```cpp
// Find the last occurrence of "/lib/" and truncate there
auto lastLibPos = cDir.rfind("/lib/");
if (lastLibPos != std::string_view::npos) {
    file = std::string(cDir.substr(0, lastLibPos))
         + BNM_OBFUSCATE_TMP("/split_config." CURRENT_ARCH ".apk!/lib/" CURRENT_ARCH "/libil2cpp.so");
    handle = BNM_dlopen(file.c_str(), RTLD_LAZY);
    // ...
}
```

---

## Medium Severity Issues

### M1: Unsafe Function Pointer Cast in `Method::Call()`

**File:** `include/BNM/Method.hpp:93`
**Code:**
```cpp
if (canInfo) return (((Ret(*)(IL2CPP::Il2CppObject *, Parameters..., IL2CPP::MethodInfo *)) method->methodPointer)(_instance, parameters..., method));
return (((Ret(*)(IL2CPP::Il2CppObject *, Parameters...)) method->methodPointer)(_instance, parameters...));
```

**Problem:**
- Casts `Il2CppMethodPointer` (void function pointer) to a specific function pointer type.
- If the il2cpp ABI changes parameter passing conventions, this crashes.
- No validation that the cast is safe.

**Impact:** Crash if il2cpp ABI changes or if method signature doesn't match.

**Mitigation:**
```cpp
// Use il2cpp_runtime_invoke instead of direct cast for safety
template<typename ...Parameters>
Ret Call(Parameters ...parameters) const {
    if (!_data) { BNM_LOG_ERR(...); return {}; }

    // For non-unsafe path, use il2cpp_runtime_invoke
    void *args[] = { &parameters... };
    IL2CPP::Il2CppException *exception = nullptr;
    void *ret = Internal::il2cppMethods.il2cpp_runtime_invoke(
        _data, _isStatic ? nullptr : _instance, args, &exception);
    if (exception) {
        BNM_LOG_ERR("Exception in method call: %s",
            Exception(exception).Message().c_str());
        return {};
    }
    return *reinterpret_cast<Ret*>(ret);
}
```

Or add a runtime check:
```cpp
// At minimum, verify method pointer is not null
if (!method->methodPointer) {
    BNM_LOG_ERR("Method pointer is null for %s", str().c_str());
    return {};
}
```

---

### M2: `List<T>::Add()` No Geometric Growth

**File:** `include/BNM/BasicMonoStructures.hpp:405-410, 575-578`
**Code:**
```cpp
void Add(T val) {
    GrowIfNeeded(1);
    items->m_Items[size] = val;
    size++;
    version++;
}

void GrowIfNeeded(int n) {
    if (size + n > items->capacity)
        Resize(size + n);  // Allocates exactly size+n, not geometric
}
```

**Problem:**
- Every time the list is full, it allocates exactly `size + n` elements.
- Adding N elements one-by-one results in O(N²) memory copies.
- Standard practice is to double capacity (geometric growth).

**Impact:** Poor performance for large lists built by repeated `Add()`.

**Mitigation:**
```cpp
void GrowIfNeeded(int n) {
    if (size + n > items->capacity) {
        int newCapacity = items->capacity;
        if (newCapacity == 0) newCapacity = 4;  // Initial allocation
        while (newCapacity < size + n) newCapacity *= 2;  // Geometric growth
        Resize(newCapacity);
    }
}
```

---

### M3: `List<T>::CopyFrom()` Double-Copy Waste

**File:** `include/BNM/BasicMonoStructures.hpp:487-494`
**Code:**
```cpp
bool CopyFrom(T *arr, int arrSize) {
    BNM_CHECK_SELF(false);
    Resize(arrSize);  // Allocates new array, copies old data (waste)
    memcpy(items->m_Items, arr, arrSize * sizeof(T));  // Overwrites everything
    size = arrSize;
    ++version;
    return true;
}
```

**Problem:**
- `Resize()` allocates a new array and copies old data to it.
- Then `memcpy` overwrites the new array with the source data.
- The old data copy is wasted work.

**Impact:** Unnecessary memory allocation and copy for every `CopyFrom` call.

**Mitigation:**
```cpp
bool CopyFrom(T *arr, int arrSize) {
    BNM_CHECK_SELF(false);
    if (arrSize > items->capacity) {
        // Only allocate if we need more space
        auto nItems = Array<T>::Create(arrSize);
        nItems->klass = items->klass;
        nItems->monitor = items->monitor;
        nItems->bounds = items->bounds;
        nItems->capacity = arrSize;
        if (!items->klass) items->Destroy();
        items = nItems;
    }
    memcpy(items->m_Items, arr, arrSize * sizeof(T));
    size = arrSize;
    ++version;
    return true;
}
```

---

### M4: `TryInvoke()` Inefficient Stack Initialization

**File:** `src/Exceptions.cpp:8-17`
**Code:**
```cpp
BNM::IL2CPP::Il2CppType type;
type.type = BNM::IL2CPP::IL2CPP_TYPE_VOID;
BNM::IL2CPP::Il2CppClass klass;
#if UNITY_VER <= 204
klass.valuetype = 0;
#endif
BNM::IL2CPP::MethodInfo info;
for (size_t i = 0; i < sizeof(info); ++i) reinterpret_cast<char *>(&info)[i] = 0;
info.return_type = &type;
BNM::PRIVATE_INTERNAL::GetMethodClass(&info) = &klass;
```

**Problem:**
- Creates temporary objects on stack every call.
- Byte-by-byte zero initialization of `MethodInfo` with a loop.
- If `TryInvoke` is called frequently, this adds up.

**Impact:** Minor performance overhead per exception check.

**Mitigation:**
```cpp
// Use static for the invariant parts
static BNM::IL2CPP::Il2CppType voidType = []{
    BNM::IL2CPP::Il2CppType t{};
    t.type = BNM::IL2CPP::IL2CPP_TYPE_VOID;
    return t;
}();

BNM::Exception BNM::TryInvoke(const std::function<void()> &func) {
    BNM::IL2CPP::Il2CppClass klass{};
#if UNITY_VER <= 204
    klass.valuetype = 0;
#endif

    BNM::IL2CPP::MethodInfo info{};
    info.return_type = &voidType;
    BNM::PRIVATE_INTERNAL::GetMethodClass(&info) = &klass;
    // ... rest
}
```

---

### M5: `MethodBase` Has Both Copy = Default and Move Constructor

**File:** `include/BNM/MethodBase.hpp:29-36, 148-169`
**Code:**
```cpp
// Copy = default
inline MethodBase(const MethodBase &other) = default;
inline MethodBase &operator=(const MethodBase &other) = default;

// Move = explicit
constexpr MethodBase(MethodBase &&other) noexcept
    : _data(other._data), _instance(other._instance),
      _isStatic(other._isStatic), _isVirtual(other._isVirtual) {
    other._data = nullptr;
    other._instance = nullptr;
    other._isStatic = false;
    other._isVirtual = false;
}
```

**Problem:**
- Copy is allowed (shallow copy of raw pointers), which could lead to double-free if two copies try to access the same resources.
- Move is explicit and nulls out the source, which is correct.
- Inconsistent ownership semantics.

**Impact:** Potential double-free or use-after-free if copies are misused.

**Mitigation:**
```cpp
// Option A: Delete copy, keep move
MethodBase(const MethodBase &other) = delete;
MethodBase &operator=(const MethodBase &other) = delete;

// Option B: If copy is needed, make it explicit and document ownership
// MethodBase is a non-owning wrapper, so copy is safe as long as
// the user understands it's a view/reference, not ownership.
// Keep as-is but add documentation.
```

---

### M6: Magic Numbers for il2cpp Flags

**File:** `src/ClassesManagement.cpp:354`
**Code:**
```cpp
klass->flags = static_cast<uint32_t>(static_cast<uint32_t>(klass->parent->flags) & ~(0x00000080u | 0x00000020u)) | 0x0000001u;
```

**Problem:**
- `0x00000080` = TYPE_ATTRIBUTE_ABSTRACT
- `0x00000020` = TYPE_ATTRIBUTE_INTERFACE
- `0x0000001` = TYPE_ATTRIBUTE_PUBLIC
- These are il2cpp constants but not defined as named constants.

**Impact:** Code readability — hard to understand what the flags mean.

**Mitigation:**
```cpp
// Add named constants at top of file or in a header
namespace Il2CppTypeFlags {
    constexpr uint32_t PUBLIC        = 0x00000001;
    constexpr uint32_t INTERFACE     = 0x00000020;
    constexpr uint32_t ABSTRACT      = 0x00000080;
    constexpr uint32_t SEALED        = 0x00000100;
    constexpr uint32_t SERIALIZABLE  = 0x00000200;
    constexpr uint32_t IMPORT        = 0x00000400;
}

// Then use:
klass->flags = (klass->parent->flags & ~(Il2CppTypeFlags::ABSTRACT | Il2CppTypeFlags::INTERFACE))
               | Il2CppTypeFlags::PUBLIC;
```

Similarly for method flags on line 749:
```cpp
myInfo->flags = 0x0006 | 0x0080; // PUBLIC | HIDE_BY_SIG
if (method->_isStatic) myInfo->flags |= 0x0010; // |= STATIC
```

Should be:
```cpp
namespace Il2CppMethodFlags {
    constexpr uint32_t PRIVATE       = 0x0001;
    constexpr uint32_t FAMILY        = 0x0002;
    constexpr uint32_t PUBLIC        = 0x0006;
    constexpr uint32_t STATIC        = 0x0010;
    constexpr uint32_t VIRTUAL       = 0x0040;
    constexpr uint32_t HIDE_BY_SIG   = 0x0080;
    constexpr uint32_t SPECIAL_NAME  = 0x0800;
    constexpr uint32_t RT_SPECIAL_NAME = 0x1000;
}
```

---

## Low Severity Issues

### L1: `static_assert(false)` Breaks Build Intentionally

**File:** `include/BNM/UserSettings/GlobalSettings.hpp:213`
**Code:**
```cpp
static_assert(false, "No hooking software!");
```

**Problem:**
- This is intentional to force users to uncomment a hooking backend.
- But it breaks the build even before the user has a chance to read the comments above.
- Some compilers evaluate `static_assert(false)` immediately when the header is included.

**Impact:** Build always fails until user modifies GlobalSettings.hpp.

**Mitigation:**
```cpp
// Option A: Use a more descriptive approach
#if !defined(BNM_USE_DOBBY) && !defined(BNM_USE_SHADOWHOOK)
static_assert(false, "BNM requires a hooking backend. "
    "Uncomment one of the following in GlobalSettings.hpp:\n"
    "  - Shadowhook block (recommended for Android)\n"
    "  - Dobby block\n"
    "See comments above this line for details.");
#endif

// Option B: Make it a runtime error instead of compile-time
// Remove static_assert and add a runtime check in Loading::Load()
```

---

### L2: `Class::GetFields()`/`GetMethods()` etc. Return `std::vector`

**File:** `include/BNM/Class.hpp:107-151`
**Code:**
```cpp
[[nodiscard]] std::vector<BNM::FieldBase> GetFields(bool includeParent = true) const;
[[nodiscard]] std::vector<BNM::MethodBase> GetMethods(bool includeParent = true) const;
```

**Problem:**
- Heap allocation on every call.
- For classes with many members (hundreds), this can be slow.

**Impact:** Performance overhead for reflection-heavy code.

**Mitigation:**
```cpp
// Option A: Return a lazy view (C++20 ranges)
auto GetFields(bool includeParent = true) const {
    return FieldRange{_data, includeParent};
}

// Option B: Provide both — vector for convenience, view for performance
std::vector<FieldBase> GetFields() const;  // Convenience
FieldRange GetFieldRange() const;  // Zero-alloc
```

---

### L3: Coroutine `IEnumerator` Memory Layout Mismatch

**File:** `include/BNM/Coroutine.hpp:97-139`
**Code:**
```cpp
struct IEnumerator : BNM::IL2CPP::Il2CppObject {
    struct promise_type { ... };  // Not an il2cpp field
    Il2CppObject *_current{};
    std::coroutine_handle<promise_type> _coroutine{};
};
```

**Problem:**
- `IEnumerator` inherits from `Il2CppObject` but has additional non-il2cpp fields (`promise_type`, `_coroutine`).
- il2cpp expects a specific memory layout for its objects.
- When il2cpp accesses this object, it may read garbage for the non-il2cpp fields.

**Impact:** Potential crash if il2cpp tries to serialize or GC this object.

**Mitigation:**
```cpp
// Use a separate C++ object that wraps an il2cpp object
struct IEnumerator {
    struct Inner : BNM::IL2CPP::Il2CppObject {
        Il2CppObject *_current{};
        void *_coroutineHandle{};  // Store as void* to match layout
    };

    Inner *object{};
    std::coroutine_handle<promise_type> handle() {
        return std::coroutine_handle<promise_type>::from_address(
            object->_coroutineHandle);
    }
};
```

Or keep the current design but ensure the object is never passed to il2cpp directly (only the IEnumerator pointer returned by `Get()` is passed).

---

### L4: `Delegates::MulticastDelegateBase::Remove()` Inconsistent Capacity Check

**File:** `src/Delegates.cpp:50-65`
**Code:**
```cpp
void BNM::MulticastDelegateBase::Remove(BNM::DelegateBase *delegate) {
    // ...
    IL2CPP::il2cpp_array_size_t index = 0;
    bool found = false;
    for (IL2CPP::il2cpp_array_size_t i = 0; i < delegates->capacity; ++i) {
        // ...
    }
    if (!found) return;
    if (this->delegates->max_length == 0) return;  // <-- Why check max_length after search?
```

**Problem:**
- `max_length` is the array's allocated size, `capacity` is the logical size.
- The check `max_length == 0` after iterating `capacity` elements is redundant and confusing.

**Impact:** Code clarity, potential logic error.

**Mitigation:**
```cpp
void BNM::MulticastDelegateBase::Remove(BNM::DelegateBase *delegate) {
    if (!CheckForNull(this)) return;

    auto delegates = reinterpret_cast<Structures::Mono::Array<DelegateBase *> *>(this->delegates);
    if (!delegates || delegates->capacity == 0) return;  // Check once

    IL2CPP::il2cpp_array_size_t index = 0;
    bool found = false;
    for (IL2CPP::il2cpp_array_size_t i = 0; i < delegates->capacity; ++i) {
        if (delegates->m_Items[i] != delegate) continue;
        found = true;
        index = i;
        break;
    }
    if (!found) return;

    // ... rest of removal logic
}
```

---

### L5: `CompileTimeClassBuilder` Uses `BNM_malloc` for Internal Nodes

**File:** `include/BNM/Class.hpp:670-671`
**Code:**
```cpp
inline CompileTimeClassBuilder(const char *_namespace, const char *_name, const char *_imageName = nullptr, bool autoFree = true) {
    if (autoFree) _data._SetBit();
    auto info = static_cast<CompileTimeClass::_ClassInfo *>(BNM_malloc(sizeof(CompileTimeClass::_ClassInfo)));
    new (info) CompileTimeClass::_ClassInfo{_namespace, _name, _imageName};
    _data._stack.Add(info);
}
```

**Problem:**
- Uses `BNM_malloc` + placement new for `_ClassInfo` nodes.
- These are freed in `CompileTimeClass::Free()` which walks the stack.
- If `Free()` is not called (e.g., in the `ToClass() const` path which creates a copy), memory leaks.

**Impact:** Memory leak in const `ToClass()` path.

**Mitigation:**
```cpp
// The const version already creates a temp copy and processes it
// But the temp copy's Free() is called via the bit flag
// Verify that all paths call Free() or use RAII

// Better: Use RAII wrapper for the stack
struct CompileTimeClassStack {
    BNM::ForwardList<_BaseInfo *> stack;

    ~CompileTimeClassStack() {
        stack.Clear([](_BaseInfo *info) { BNM_free(info); });
    }

    // Non-copyable, movable
    CompileTimeClassStack(const CompileTimeClassStack&) = delete;
    CompileTimeClassStack& operator=(const CompileTimeClassStack&) = delete;
    CompileTimeClassStack(CompileTimeClassStack&&) noexcept = default;
    CompileTimeClassStack& operator=(CompileTimeClassStack&&) noexcept = default;
};
```

---

## Info / Code Quality Notes

### I1: Missing `[[nodiscard]]` on Some Return Values

Several methods that return pointers or important values don't have `[[nodiscard]]`:
- `Class::CreateNewInstance()` — line 390 has it, good
- `DelegateBase::Create()` — line 43 has it, good
- `MulticastDelegateBase::Add(MethodBase)` — line 85 has it, good

Most are correctly annotated. Minor inconsistency on some internal functions.

### I2: `#pragma pack(push, 1)` on `MethodBase` and `FieldBase`

**File:** `include/BNM/MethodBase.hpp:10`, `include/BNM/FieldBase.hpp:8`

This is for binary compatibility with il2cpp structures but can cause misaligned access penalties on ARM. Since these are wrappers (not the actual il2cpp structs), the pack pragma may be unnecessary.

### I3: Bilingual Comments (EN+RU) Consistent Throughout

The codebase maintains bilingual documentation consistently. This is good for the project's international audience.

### I4: Debug Features Properly Gated by `#ifndef NDEBUG`

Debug-only features (`BNM_ALLOW_STR_METHODS`, `BNM_ALLOW_SAFE_IS_ALLOCATED`, `BNM_ALLOW_SELF_CHECKS`, `BNM_CHECK_INSTANCE_TYPE`, `BNM_DEBUG`, `BNM_INFO`, `BNM_ERROR`, `BNM_WARNING`) are all gated correctly.

### I5: `ForwardList<T>` Custom Implementation

The custom `ForwardList` is a circular singly-linked list. It works but:
- No iterator support (range-based for won't work)
- `Clear()` with callback is manual memory management
- Copy constructor/assignment do deep copy (correct)

This is internal-only and acceptable for its use case.

---

## Summary

| Severity | Count | Fixed |
|----------|-------|-------|
| Critical | 2 | 0 |
| High | 4 | 0 |
| Medium | 6 | 0 |
| Low | 5 | 0 |
| Info | 5 | - |
| **Total** | **22** | **0** |

**Top 3 Priority Fixes:**
1. **C1:** Replace `IsAllocated()` SIGSEGV approach with `mincore()` or null-only check
2. **C2:** Fix JNI `ReleaseStringUTFChars` pointer argument
3. **H1:** Remove or fix `field->myInfo` dangling pointer in `ModifyClass()`

---

## Version Compatibility Analysis (Unity 5.6 - 6000.x)

Setiap mitigasi diverifikasi terhadap compatibility matrix. Berdasarkan data dari il2cpp headers di `include/BNM/Il2CppHeaders/`:

### Compatibility Matrix

| ID | Mitigation | Unity < 171 (5.6, 2017.1) | Unity 171-174 (2017.2-4) | Unity 175-201 (2018-2020.1) | Unity 202-211 (2020.2-2021.1) | Unity 212+ (2021.2+) | Safe? |
|----|-----------|---------------------------|--------------------------|----------------------------|-------------------------------|---------------------|-------|
| C1 | `IsAllocated()` → `mincore()` | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| C2 | JNI `ReleaseStringUTFChars` fix | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| H1 | ModifyClass use-after-free fix | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| H2 | `TryInit()` caching | ⚠️ | ⚠️ | ✅ | ✅ | ✅ | **NEEDS CHECK** |
| H3 | `MemFnToVoidPtr` static_assert | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** (ARM only) |
| H4 | Split APK path fix | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| M1 | Method::Call → il2cpp_runtime_invoke | ❌ | ⚠️ | ✅ | ✅ | ✅ | **NO** |
| M2 | List geometric growth | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| M3 | List::CopyFrom optimization | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| M4 | TryInvoke static voidType | ⚠️ | ⚠️ | ✅ | ✅ | ✅ | **NEEDS CHECK** |
| M5 | MethodBase copy semantics | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| M6 | Named constants for flags | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| L1 | static_assert(false) improvement | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| L2 | GetFields/GetMethods ranges | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| L3 | IEnumerator layout fix | ⚠️ | ⚠️ | ⚠️ | ⚠️ | ⚠️ | **NEEDS CHECK** |
| L4 | Delegate Remove capacity check | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |
| L5 | CompileTimeClass RAII | ✅ | ✅ | ✅ | ✅ | ✅ | **YES** |

### Detail Analysis per Mitigation

#### ✅ SAFE — Version-Independent (12 mitigations)

These mitigasi **tidak menyentuh il2cpp internals** dan bekerja di semua versi Unity:

- **C1 (`IsAllocated`)**: `mincore()` adalah POSIX syscall, tidak tergantung Unity. Work di semua Android version.
- **C2 (JNI pointer)**: JNI API tidak berubah antara Unity versions. Fix murni Android API.
- **H1 (ModifyClass UAF)**: Internal BNM logic, tidak bergantung pada il2cpp struct layout.
- **H3 (MemFnToVoidPtr)**: Bergantung pada C++ ABI (ARM64/ARM), bukan Unity. Di Itanium ABI (Android), member function pointer = sizeof(void*).
- **H4 (Split APK path)**: Pure Android path logic.
- **M2 (List growth)**: Standard C++ pattern, zero il2cpp dependency.
- **M3 (List::CopyFrom)**: Standard C++ optimization.
- **M5 (MethodBase copy)**: C++ semantics, tidak tergantung Unity.
- **M6 (Named constants)**: Readability improvement, tidak mengubah behavior.
- **L1 (static_assert)**: Compile-time improvement.
- **L2 (Ranges)**: C++20 feature, version-independent.
- **L4 (Delegate capacity)**: Logic fix, tidak tergantung Unity.
- **L5 (RAII)**: C++ pattern, version-independent.

#### ⚠️ NEEDS VERSION-SPECIFIC CHECK (3 mitigations)

**H2: `TryInit()` Caching**

```cpp
// Current proposed fix:
struct Class {
    IL2CPP::Il2CppClass *_data{};
    mutable uint8_t _initialized : 1 = 0;
    void TryInit() const {
        if (_data && !_initialized) {
            Internal::il2cppMethods.Class$$Init(_data);
            _initialized = 1;
        }
    }
};
```

**Risk:** `Class$$Init` dipanggil sekali per Class object. Tapi di codebase lama, ada pattern di mana Class object di-reuse untuk target berbeda (reassign `_data`). Jika `_initialized` tidak di-reset saat `_data` berubah, init akan skip.

**Compatibility check:**
- Unity < 171: `Class$$Init` resolve via assembly tracing. Behavior sama.
- Unity 171+: `Class$$Init` resolve via `il2cpp_array_new_specific` tracing. Behavior sama.
- **Verdict:** Safe SELAMA `_initialized` di-reset saat `_data` berubah. Perlu tambahan:

```cpp
void SetData(IL2CPP::Il2CppClass *data) {
    _data = data;
    _initialized = 0;  // Reset on data change
}
```

**M4: TryInvoke Static voidType**

```cpp
// Current proposed fix:
static BNM::IL2CPP::Il2CppType voidType = []{
    BNM::IL2CPP::Il2CppType t{};
    t.type = BNM::IL2CPP::IL2CPP_TYPE_VOID;
    return t;
}();
```

**Risk:** `Il2CppType` layout berubah antara versi:
- Unity < 175: `byval_arg` adalah pointer
- Unity >= 175: `byval_arg` adalah struct inline

Tapi `voidType` hanya perlu `type = IL2CPP_TYPE_VOID`, tidak menyentuh `byval_arg`. **Safe.**

**L3: IEnumerator Layout**

```cpp
struct IEnumerator : BNM::IL2CPP::Il2CppObject {
    promise_type _promise;  // Not an il2cpp field
    Il2CppObject *_current{};
    std::coroutine_handle<promise_type> _coroutine{};
};
```

**Risk:** Il2CppClass `instance_size` untuk IEnumerator diset ke `sizeof(IEnumerator)` di Coroutine.cpp line 41:
```cpp
customClass._size = sizeof(IEnumerator);
```
Ini sudah benar — allocation size mencakup semua fields. Tapi il2cpp GC mungkin tidak bisa trace `_coroutine` handle karena bukan il2cpp object pointer.

**Compatibility:**
- Unity < 212: GC non-concurrent, safe karena `has_references = 1` memaksa scanning.
- Unity 212+: GC concurrent, tapi IEnumerator fields are C++ owned, not GC-managed.
- **Verdict:** Safe karena coroutine handle adalah C++ object, bukan il2cpp managed object. GC tidak perlu trace dia.

#### ❌ UNSAFE — Version-Specific (1 mitigation)

**M1: Method::Call → il2cpp_runtime_invoke**

```cpp
// Proposed: Replace direct cast with il2cpp_runtime_invoke
void *args[] = { &parameters... };
IL2CPP::Il2CppException *exception = nullptr;
void *ret = Internal::il2cppMethods.il2cpp_runtime_invoke(
    _data, _isStatic ? nullptr : _instance, args, &exception);
```

**Problem fundamental:**

1. **Parameter marshaling differs between versions:**
   - Unity < 171: `invoker_method = void(*)(MethodInfo*)` — parameters passed directly on stack
   - Unity >= 171: `invoker_method = void(*)(void*)` — parameters packed in array
   - `il2cpp_runtime_invoke` expects `void**` array (each element is pointer to value)
   - Direct cast passes values directly (not pointers to values)

2. **Value type handling differs:**
   - Direct cast: `int x = 5; func(x);` — passes 5 directly
   - il2cpp_runtime_invoke: `void* args[] = { &x };` — passes pointer to 5
   - For structs: direct cast copies entire struct, il2cpp_runtime_invoke passes pointer

3. **Return type marshaling differs:**
   - Unity < 211: Return via boxing (`Class(type).BoxObject(&result)`)
   - Unity >= 211: Return via `returnValue` pointer
   - `il2cpp_runtime_invoke` returns `Il2CppObject*` — needs unboxing for value types

4. **MethodInfo parameter passing:**
   - Unity > 174: Last parameter is `MethodInfo*`
   - Unity <= 174: No `MethodInfo*` parameter (or first parameter is unused `void*`)
   - `il2cpp_runtime_invoke` hides this difference internally, but the direct cast code already handles it with `#if UNITY_VER`

**Why it's unsafe across versions:**

The current direct cast approach is **already version-aware** with `#if UNITY_VER` branches. Replacing it with `il2cpp_runtime_invoke` would require:
1. Different parameter marshaling for each version (value types vs pointers)
2. Different return value handling (boxing vs direct)
3. Different `MethodInfo*` parameter passing

This is **more complex** than the current approach, not less. The current code, while using direct casts, is correct for each version because it follows the exact calling convention that il2cpp uses internally.

**Verdict:** Do NOT apply this mitigation. The current direct cast approach is the correct one for this codebase because it matches il2cpp's internal calling conventions exactly.

---

### Final Recommendation

**Apply these mitigasi (safe across all versions):**
1. C1 — `IsAllocated()` → `mincore()` (Critical fix)
2. C2 — JNI pointer fix (Critical fix)
3. H1 — ModifyClass UAF fix (High fix)
4. H4 — Split APK path fix (High fix)
5. M2 — List geometric growth (Medium perf)
6. M3 — List::CopyFrom optimization (Medium perf)
6. M6 — Named constants for flags (Medium readability)
7. L1 — static_assert improvement (Low)
8. L4 — Delegate Remove cleanup (Low)
9. L5 — CompileTimeClass RAII (Low)

**Apply with version-specific guard:**
10. H2 — TryInit caching (with `_initialized` reset on data change)
11. M4 — TryInvoke static voidType (safe, just needs `#if` guard for older versions)

**Do NOT apply:**
12. M1 — Method::Call → il2cpp_runtime_invoke (unsafe, current approach is correct)
13. L3 — IEnumerator layout (current approach already works correctly)

---

## Completion Status (Applied 2026-09-20)

### Applied (12 mitigations)

| ID | File | Change |
|----|------|--------|
| C1 | `include/BNM/Utils.hpp` | `IsAllocated()` SIGSEGV+longjmp → `mincore()` |
| C2 | `src/Loading.cpp:136` | `ReleaseStringUTFChars(jDir, cDir.data())` → `dirChars` |
| H1 | `src/ClassesManagement.cpp:155-167` | Removed dangling `field->myInfo` and `field->offset` writes |
| H1 | `src/ClassesManagement.cpp:322-330` | Same fix in `CreateClass()` |
| H2 | `include/BNM/Class.hpp` + `src/Class.cpp` | Added `_initialized` flag, skip redundant `Class$$Init` |
| H4 | `src/Loading.cpp:130` | Split APK path: `rfind("/lib/")` instead of `substr(len-8)` |
| M2 | `include/BNM/BasicMonoStructures.hpp:575` | `GrowIfNeeded` linear → geometric (2x) growth |
| M3 | `include/BNM/BasicMonoStructures.hpp:487` | `CopyFrom` skip old data copy on resize |
| M4 | `src/Exceptions.cpp` | Static voidType/voidClass, `memset` → `= {}` |
| M6 | `src/ClassesManagement.cpp` | Named constants for `METHOD_PUBLIC`, `STATIC`, etc. |
| L1 | `include/BNM/UserSettings/GlobalSettings.hpp:213` | `static_assert(false)` → `#error` directive (clearer error, no C++20 issues) |
| L4 | `src/Delegates.cpp` | `Remove()` fix memmove order, remove dead `max_length` check |

### Not Applied (1 intentional skip)

| ID | Reason |
|----|--------|
| L5 | `CompileTimeClass._stack` is global-static — no leak, lives for program lifetime |

### Intentionally Skipped (from review)

| ID | Reason |
|----|--------|
| M1 | Direct cast in `Method::Call()` matches il2cpp internal calling conventions — `il2cpp_runtime_invoke` replacement would break parameter marshaling |
| L3 | IEnumerator layout already correct — `customClass._size = sizeof(IEnumerator)` allocates enough space, coroutine handle is C++ owned |

---

*Generated by opencode C++ Pro review — 2026-09-20*
*Version compatibility verified against il2cpp headers in include/BNM/Il2CppHeaders/*
