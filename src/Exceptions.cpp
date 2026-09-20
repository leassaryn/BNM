#include <BNM/Exceptions.hpp>
#include <Internals.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-function-type-mismatch"

BNM::Exception BNM::TryInvoke(const std::function<void()> &func) {
    // Static void type and class to avoid re-initialization on every call.
    // Thread-safe initialization guaranteed by C++11.
    static auto &voidType = []() -> BNM::IL2CPP::Il2CppType & {
        static BNM::IL2CPP::Il2CppType t{};
        t.type = BNM::IL2CPP::IL2CPP_TYPE_VOID;
        return t;
    }();
    static auto &voidClass = []() -> BNM::IL2CPP::Il2CppClass & {
        static BNM::IL2CPP::Il2CppClass k{};
#if UNITY_VER <= 204
        k.valuetype = 0;
#endif
        k.instance_size = 0;
        return k;
    }();

    BNM::IL2CPP::MethodInfo info{};
    info.return_type = &voidType;
    BNM::PRIVATE_INTERNAL::GetMethodClass(&info) = &voidClass;

    info.methodPointer = reinterpret_cast<decltype(info.methodPointer)>(const_cast<std::function<void()> *>(&func));
#if UNITY_VER < 171
    info.invoker_method = reinterpret_cast<BNM::IL2CPP::InvokerMethod>(+[](BNM::IL2CPP::MethodInfo *info) -> void { (*reinterpret_cast<std::function<void()> *>(info->methodPointer))(); });
#else
    info.invoker_method = reinterpret_cast<BNM::IL2CPP::InvokerMethod>(+[](std::function<void()> *func) -> void { (*func)(); });
#endif
    BNM::IL2CPP::Il2CppException *exception = nullptr;
    Internal::il2cppMethods.il2cpp_runtime_invoke(&info, nullptr, nullptr, &exception);
    return exception;
}

#pragma clang diagnostic pop