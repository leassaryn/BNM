#pragma once

#include <vector>

#include "UserSettings/GlobalSettings.hpp"
#include "Method.hpp"
#include "BasicMonoStructures.hpp"
#include "Utils.hpp"

// NOLINTBEGIN
namespace BNM {

    template<typename Ret>
    struct Delegate;
    template<typename Ret>
    struct MulticastDelegate;

    /**
        @brief Wrapper of Il2CppDelegate
    */
    struct DelegateBase : public IL2CPP::Il2CppDelegate {
        inline constexpr DelegateBase() = default;

        /**
            @brief Get instance of delegate.
            @return Instance if delegate isn't null, otherwise null.
        */
        inline IL2CPP::Il2CppObject* GetInstance() const {
            return CheckForNull(this) ? target : nullptr;
        }

        /**
            @brief Get method of delegate.
            @return MethodBase if delegate isn't null, otherwise empty MethodBase.
        */
        [[nodiscard]] BNM::MethodBase GetMethod() const;

        /**
            @brief Create delegate from method.
            @param method Method that will be used in new delegate
            @return New DelegateBase
        */
        [[nodiscard]] DelegateBase *Create(BNM::MethodBase method);

        /**
            @brief Check if delegate is valid.
        */
        [[nodiscard]] inline bool IsValid() const noexcept { return CheckForNull(this); }

        /**
            @brief Check if delegate is valid.
        */
        inline operator bool() const noexcept { return IsValid(); }

        /**
            @brief Cast delegate to be able to invoke it.
        */
        template<typename NewRet>
        inline Delegate<NewRet> &cast() const { return const_cast<Delegate<NewRet> &>(static_cast<const Delegate<NewRet> &>(*this)); }
    };

    /**
        @brief Wrapper of Il2CppMulticastDelegate
    */
    struct MulticastDelegateBase : public IL2CPP::Il2CppMulticastDelegate {
        inline constexpr MulticastDelegateBase() = default;

        /**
            @brief Get methods of delegate.
            @return Vector of MethodBase if delegate isn't null, otherwise empty vector.
        */
        std::vector<BNM::MethodBase> GetMethods() const;

        /**
            @brief Add delegate.
            @param delegate Delegate to add
        */
        void Add(DelegateBase *delegate);

        /**
            @brief Add method to delegate.
            @param method Method to add
            @return New added DelegateBase of method
        */
        DelegateBase *Add(BNM::MethodBase method);

        /**
            @brief Remove delegate.
            @param delegate Delegate to remove
        */
        void Remove(DelegateBase *delegate);

        inline void operator +=(DelegateBase *delegate) { return Add(delegate); }
        inline void operator +=(BNM::MethodBase method) { Add(method); }
        inline void operator -=(DelegateBase *delegate) { return Remove(delegate); }

        /**
            @brief Cast delegate to be able to invoke it.
        */
        template<typename NewRet>
        inline MulticastDelegate<NewRet> &cast() const { return const_cast<MulticastDelegate<NewRet> &>(static_cast<const MulticastDelegate<NewRet> &>(*this)); }
    };

    /**
        @brief Typed wrapper of Il2CppDelegate
        @tparam Ret Return type
    */
    template<typename Ret>
    struct Delegate : public DelegateBase {

        /**
            @brief Invoke delegate.
            @tparam Ret Return type
            @tparam Parameters Delegate parameter types
            @param parameters Delegate parameters
        */
        template<typename ...Parameters>
        inline Ret Invoke(Parameters ...parameters) {
            return GetMethod().template cast<Ret>().Call(parameters...);
        }

        template<typename ...Parameters>
        inline Ret operator()(Parameters ...parameters) { return Invoke(parameters...); }
    };

    /**
        @brief Typed wrapper of Il2CppMulticastDelegate
        @tparam Ret Return type
    */
    template<typename Ret>
    struct MulticastDelegate : public MulticastDelegateBase {

        /**
            @brief Invoke delegate.
            @tparam Ret Return type
            @tparam Parameters Delegate parameter types
            @param parameters Delegate parameters
        */
        template<typename ...Parameters>
        inline Ret Invoke(Parameters ...parameters) {
            if (!CheckForNull(this)) return BNM::PRIVATE_INTERNAL::ReturnEmpty<Ret>();

            auto delegates = reinterpret_cast<Structures::Mono::Array<DelegateBase *> *>(this->delegates);
            // Without a delegate array the object holds a single delegate in its first member.
            if (!delegates) return reinterpret_cast<Delegate<Ret> *>(&this->delegate)->Invoke(parameters...);

            if (delegates->capacity == 0) return BNM::PRIVATE_INTERNAL::ReturnEmpty<Ret>();
            for (IL2CPP::il2cpp_array_size_t i = 0; i < delegates->capacity - 1; ++i) {
                auto d = delegates->At(i);
                if (d) d->GetMethod().template cast<Ret>().Call(parameters...);
            }
            auto last = delegates->At(delegates->capacity - 1);
            if (last) return last->GetMethod().template cast<Ret>().Call(parameters...);
            return BNM::PRIVATE_INTERNAL::ReturnEmpty<Ret>();
        }

        template<typename ...Parameters>
        inline Ret operator()(Parameters ...parameters) { return Invoke(parameters...); }
    };

    namespace Structures::Mono {
        /**
           @brief System.Action type implementation
        */
        template <typename ...Parameters>
        struct Action : public MulticastDelegate<void> {};
    }
}
// NOLINTEND