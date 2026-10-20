#pragma once

#include <BNM/GCHandle.hpp>
#include <type_traits>

static_assert(!std::is_copy_constructible_v<GCHandle>);
static_assert(!std::is_copy_assignable_v<GCHandle>);
static_assert(std::is_nothrow_move_constructible_v<GCHandle>);
static_assert(std::is_nothrow_move_assignable_v<GCHandle>);
#if UNITY_VER >= 231
static_assert(std::is_pointer_v<GCHandle::NativeHandle>);
#else
static_assert(sizeof(GCHandle::NativeHandle) == 4);
#endif

template<typename T>
static T FixtureHandle(uintptr_t value)
{
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}

static int GCHandleRegression()
{
    using Handle = GCHandle::NativeHandle;
    static std::map<Handle, IL2CPP::Il2CppObject *> roots;
    static uintptr_t next = sizeof(Handle) > 4 ? static_cast<uintptr_t>(0x100000000ULL) : 1;
    static int created{}, freed{};
    static bool fail{}, lastPinned{};
    auto &api = Internal::il2cppMethods;
    api.il2cpp_gchandle_new = +[](IL2CPP::Il2CppObject *object, bool pinned) -> Handle
    {
        lastPinned = pinned;
        if (fail) return {};
        const auto handle = FixtureHandle<Handle>(next++);
        roots.emplace(handle, object);
        ++created;
        return handle;
    };
    api.il2cpp_gchandle_get_target = +[](Handle handle) { return roots.at(handle); };
    api.il2cpp_gchandle_free = +[](Handle handle)
    {
        if (roots.erase(handle) != 1) std::abort();
        ++freed;
    };
    IL2CPP::Il2CppObject first{}, second{};
    if (GCHandle::New(&first)) return 1; // Runtime not published.
    Internal::states.state = true;
    if (GCHandle::New(nullptr) || created) return 2;
    const auto freeHandle = api.il2cpp_gchandle_free;
    api.il2cpp_gchandle_free = nullptr;
    if (GCHandle::New(&first) || created) return 3;
    api.il2cpp_gchandle_free = freeHandle;
    const auto getTarget = api.il2cpp_gchandle_get_target;
    api.il2cpp_gchandle_get_target = nullptr;
    if (GCHandle::New(&first) || created) return 4;
    api.il2cpp_gchandle_get_target = getTarget;
    const auto newHandle = api.il2cpp_gchandle_new;
    api.il2cpp_gchandle_new = nullptr;
    if (GCHandle::New(&first)) return 5;
    api.il2cpp_gchandle_new = newHandle;
    fail = true;
    if (GCHandle::New(&first) || created || freed) return 6;
    fail = false;
    {
        auto a = GCHandle::New(&first);
        if (!a || a.Get() != &first || lastPinned) return 7;
        auto b = GCHandle::New(&second, true);
        if (!b || !lastPinned) return 8;
        auto c = std::move(a);
        if (a || a.Get() || c.Get() != &first) return 9;
        b = std::move(c);
        if (c || b.Get() != &first || freed != 1) return 10;
        auto *alias = &b;
        b = std::move(*alias);
        if (!b || freed != 1) return 11;
        // Existing owners retain their cleanup/lookup callbacks.
        api.il2cpp_gchandle_get_target = nullptr;
        api.il2cpp_gchandle_free = nullptr;
        if (b.Get() != &first) return 12;
        b.Reset();
        b.Reset();
        if (b || b.Get() || freed != 2) return 13;
        api.il2cpp_gchandle_get_target = getTarget;
        api.il2cpp_gchandle_free = freeHandle;
    }
    try
    {
        auto handle = GCHandle::New(&first);
        throw std::runtime_error("unwind");
    }
    catch (const std::runtime_error &) {}
    return created == 3 && freed == 3 && roots.empty() ? 0 : 14;
}
