#pragma once

#include <sys/mman.h>
#include <unistd.h>

#include <string_view>
#include <type_traits>
#include <new>
#include <utility>

// std::to_underlying is C++23; provide a portable fallback that works everywhere.
namespace BNM {
    // Mapping permission check only; callers must still own the object's lifetime.
    size_t ReadableBytes(const void *address, size_t maximum);
    template<typename E>
    constexpr auto to_underlying(E e) noexcept -> std::underlying_type_t<E> {
        return static_cast<std::underlying_type_t<E>>(e);
    }
} // namespace BNM

#include "UserSettings/GlobalSettings.hpp"
#include "Il2CppHeaders.hpp"

namespace BNM {
#if __DOXYGEN__
#define NO_INLINE
#else
#define NO_INLINE __attribute__((noinline))
#endif

    /**
        @brief Macro function for checking pointer for null. Due to noinline attribute allows to check even `this` of objects.
    */
    template <typename T>
    NO_INLINE bool CheckForNull(T obj) { return (void *) obj; }

    /**
        @brief Macro function for checking if pointer points to valid address.
        @return True if address is valid.
    */
    template <typename T>
        requires std::is_pointer_v<T>
    inline bool IsAllocated(T x) {
#ifdef BNM_ALLOW_SAFE_IS_ALLOCATED
        if (!x) return false;
        return ReadableBytes(reinterpret_cast<const void *>(x), 1) == 1;
#else
        return x != nullptr;
#endif
    }

    namespace Structures::Mono { struct String; }

    /**
        @brief Macro function for creating C# strings (BNM::Structures::Mono::String).
        @return New mono string object
    */
    Structures::Mono::String *CreateMonoString(const std::string_view &str);

    /**
        @brief Macro function for getting external methods (icall).
        @return Pointer to extern method if it's found, otherwise null.
    */
    void *GetExternMethod(const std::string_view &str);

    /**
        @brief Check if BNM and il2cpp are loaded.
        @return True if BNM and il2cpp are loaded.
    */
    bool IsLoaded();

    /**
        @brief Get handle of libil2cpp.so if it's used by BNM.
        @return Dlfcn handle of libil2cpp.so if it's used by BNM and BNM is loaded, otherwise null.
    */
    void *GetIl2CppLibraryHandle();

    /**
        @brief Unbox any object.
        @return Unboxed object of passed type
    */
    template<typename T>
    inline T UnboxObject(T obj) { return reinterpret_cast<T>(reinterpret_cast<void *>(reinterpret_cast<char *>(obj) + sizeof(BNM::IL2CPP::Il2CppObject))); }

#ifdef BNM_DEPRECATED
    template <typename T>
        requires std::is_pointer_v<T>
    inline T CheckObj(T obj) {
        if (obj && IsAllocated(obj)) return obj;
        return nullptr;
    }

    template<typename MET_T, typename PTR_T>
    inline void InitFunc(MET_T& method, PTR_T ptr) {
        *reinterpret_cast<void **>(&method) = reinterpret_cast<void *>(ptr);
    }
#endif

    /// @cond
    // For thread static adn const fields
    namespace PRIVATE_FieldUtils {
        void GetStaticValue(IL2CPP::FieldInfo *info, void *value);
        void SetStaticValue(IL2CPP::FieldInfo *info, void *value);
    }
    /// @endcond

    struct CompileTimeClass;
#ifdef BNM_DEBUG
    /**
        @brief BNM namespace with utils
    */
    namespace Utils {
        void *OffsetInLib(void *);
        void LogCompileTimeClass(const BNM::CompileTimeClass &compileTimeClass);
    }
#endif

    /**
        @brief Attach current thread to il2cpp VM.
        @return True if thread was attached.
    */
    bool AttachIl2Cpp();

    /**
        @brief Get current thread attached to il2cpp VM.
        @return IL2CPP::Il2CppThread if current tread is attached to VM, otherwise null.
    */
    IL2CPP::Il2CppThread *CurrentIl2CppThread();

    /**
        @brief Detach current thread from il2cpp VM.
    */
    void DetachIl2Cpp();

    /**
        @brief Allocates memory that is registered in il2cpp's GC.
        @return Allocated memory
    */
    void *Allocate(size_t size);

    /**
        @brief Frees memory that was allocated by il2cpp.
    */
    void Free(void *);

#if UNITY_VER >= 232
    /**
        @brief Unmarshals unity object.
        @return Unmarshaled unity object
    */
    inline BNM_INT_PTR UnmarshalUnityObject(BNM_INT_PTR gcHandlePtr) {
        return *reinterpret_cast<BNM_INT_PTR *>(gcHandlePtr & ~static_cast<BNM_INT_PTR>(1));
    }
#endif

    /// @cond
    namespace PRIVATE_INTERNAL {
        template<typename T>
        inline T ReturnEmpty() { if constexpr (std::is_same_v<T, void>) return; else return {}; }

        inline IL2CPP::Il2CppClass *&GetMethodClass(IL2CPP::MethodInfo *methodInfo) {
#if UNITY_VER > 174
            return methodInfo->klass;
#else
            return methodInfo->declaring_type;
#endif
        }

        // Parameter type access: Unity < 212 uses ParameterInfo struct, >= 212 uses Il2CppType* array
        inline const IL2CPP::Il2CppType *GetMethodParam(const IL2CPP::MethodInfo *method, uint8_t index) {
#if UNITY_VER < 212
            return (method->parameters + index)->parameter_type;
#else
            return method->parameters[index];
#endif
        }

        inline const IL2CPP::Il2CppType *GetPropertySetParam(const IL2CPP::PropertyInfo *prop) {
#if UNITY_VER < 212
            return prop->set->parameters->parameter_type;
#else
            return prop->set->parameters[0];
#endif
        }
    }
    /// @endcond

    /**
        @brief BNM's custom forward list implementation

        @warning Don't use it anywhere! Internal Only

        Almost same as std::forward_list, but works how BNM needs to.
        Has only pointer to last element, but elements are looped, so last element points to first one.
        It used internally, so it lacks a lot of C++ stuff like foreach.
    */
    template<typename T>
    struct ForwardList {
        struct Element {
            Element *next{};
            T value{};
        };

        Element *lastElement{};

        inline ForwardList() = default;

        inline ~ForwardList() { Clear(); }

        inline ForwardList(const ForwardList& other) {
            ForwardList copy;
            if (!other.IsEmpty()) {
                auto *current = other.lastElement->next;
                do {
                    copy.Add(current->value);
                    current = current->next;
                } while (current != other.lastElement->next);
            }
            lastElement = std::exchange(copy.lastElement, nullptr);
        }

        inline ForwardList& operator=(const ForwardList& other) {
            if (this != &other) { ForwardList copy(other); *this = std::move(copy); }
            return *this;
        }

        inline ForwardList(ForwardList&& other) noexcept : lastElement(other.lastElement) {
            other.lastElement = nullptr;
        }

        inline ForwardList& operator=(ForwardList&& other) noexcept {
            if (this != &other) {
                Clear();
                lastElement = other.lastElement;
                other.lastElement = nullptr;
            }
            return *this;
        }

        [[nodiscard]] inline bool IsEmpty() const { return !lastElement; }

        inline void Add(T value) {
            auto newElement = reinterpret_cast<Element *>(BNM_malloc(sizeof(Element)));
            if (!newElement) throw std::bad_alloc();
            new (newElement) Element{};
            newElement->next = nullptr;
            newElement->value = value;

            if (IsEmpty()) {
                newElement->next = newElement;
                lastElement = newElement;
            } else {
                newElement->next = lastElement->next;
                lastElement->next = newElement;
                lastElement = newElement;
            }
        }

        inline void Clear(void (*onElementFreed)(T value) = nullptr) {
            if (IsEmpty()) return;

            auto current = lastElement->next;
            lastElement->next = nullptr;

            while (current) {
                auto next = current->next;
                if (onElementFreed) onElementFreed(current->value);
                current->~Element();
                BNM_free(current);
                current = next;
            }

            lastElement = nullptr;
        }
    };
}
