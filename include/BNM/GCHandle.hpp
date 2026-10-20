#pragma once

#include <cstdint>
#include <BNM/Il2CppHeaders.hpp>

namespace BNM
{
    /** Owns one strong IL2CPP GC root, optionally pinned.
        Use on an attached thread while the runtime is alive. The input must already
        be a live managed object. Destroy/reset before runtime shutdown or library unload.
        Moving transfers ownership; access to the same wrapper requires synchronization.
        This roots the managed object, not the native UnityEngine.Object it may represent. */
    class GCHandle final
    {
    public:
#if UNITY_VER >= 231
        using NativeHandle = IL2CPP::Il2CppGCHandle;
#else
        using NativeHandle = uint32_t;
#endif
        GCHandle() noexcept = default;
        ~GCHandle() noexcept;
        GCHandle(const GCHandle &) = delete;
        GCHandle &operator=(const GCHandle &) = delete;
        GCHandle(GCHandle &&other) noexcept;
        GCHandle &operator=(GCHandle &&other) noexcept;

        /// Returns empty for null input, unavailable runtime/API, or a zero handle.
        /// Runtime allocation exceptions retain IL2CPP's own behavior.
        [[nodiscard]] static GCHandle New(IL2CPP::Il2CppObject *object, bool pinned = false);
        [[nodiscard]] explicit operator bool() const noexcept { return _handle != NativeHandle{}; }
        /// Borrowed target, valid only while protected by a live root.
        [[nodiscard]] IL2CPP::Il2CppObject *Get() const noexcept;
        /// Releases the root once; repeated calls are harmless.
        void Reset() noexcept;

    private:
        NativeHandle _handle{};
        IL2CPP::Il2CppObject *(*_getTarget)(NativeHandle){};
        void (*_free)(NativeHandle){};
    };
}
