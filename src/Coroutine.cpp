#include <cstring>

#include <BNM/Coroutine.hpp>

#if defined(BNM_CLASSES_MANAGEMENT) && defined(BNM_COROUTINE)

#include <Internals.hpp>

namespace Classes {
    static BNM::Class AsyncOperation{}, WaitForEndOfFrame{}, WaitForFixedUpdate{}, WaitForSeconds{}, WaitForSecondsRealtime{};
}

namespace IEnumeratorData {
    static BNM::MANAGEMENT_STRUCTURES::CustomClass customClass{};
    static BNM::MANAGEMENT_STRUCTURES::CustomMethod customMethod_Finalize{}, customMethod_MoveNext{}, customMethod_Reset{}, customMethod_Current{};
}

namespace CustomWaitData {
    static BNM::MANAGEMENT_STRUCTURES::CustomClass customClass{};
    static BNM::MANAGEMENT_STRUCTURES::CustomMethod customMethod_Finalize{}, customMethod_MoveNext{}, customMethod_Reset{}, customMethod_Current{};
}

struct CustomWait : BNM::IL2CPP::Il2CppObject {
    std::function<bool()> _func{};
    bool _isUntil = false;
    void Finalize() { this->~CustomWait(); }
    [[nodiscard]] bool MoveNext() const { return _isUntil == !_func(); }
    void Reset() {}
    [[nodiscard]] static Il2CppObject *Current() { return nullptr; }
};

static inline void *MemFnToVoidPtr(const void *memPtr) {
    void *result;
    memcpy(&result, memPtr, sizeof(result));
    return result;
}

struct BNM::Coroutine::_IEnumeratorInit {
    inline static void Init() {
        using namespace IEnumeratorData;
        customClass._size = sizeof(IEnumerator);
        customClass._targetType = BNM::CompileTimeClassBuilder(BNM_OBFUSCATE_TMP("BNM.Coroutine"), BNM_OBFUSCATE_TMP("IEnumerator")).Build();
        customClass._baseType = {};
        customClass._owner = {};
        customClass._interfaces = { BNM::CompileTimeClassBuilder(BNM_OBFUSCATE_TMP("System.Collections"), BNM_OBFUSCATE_TMP("IEnumerator"), BNM_OBFUSCATE_TMP("mscorlib.dll")).Build() };
        BNM::MANAGEMENT_STRUCTURES::AddClass(&customClass);
        {
            constexpr auto p = &IEnumerator::Finalize;
            customMethod_Finalize._address = MemFnToVoidPtr(&p);
            customMethod_Finalize._invoker = reinterpret_cast<void *>(&BNM::MANAGEMENT_STRUCTURES::GetMethodInvoker<false, decltype(&IEnumerator::Finalize)>::Invoke);
            customMethod_Finalize._name = BNM_OBFUSCATE_TMP("Finalize");
            customMethod_Finalize._returnType = BNM::Defaults::Get<void>();
            customMethod_Finalize._isStatic = false;
            customMethod_Finalize._parameterTypes = {};
            customClass._methods.push_back(&customMethod_Finalize);
        }
        {
            constexpr auto p = &IEnumerator::MoveNext;
            customMethod_MoveNext._address = MemFnToVoidPtr(&p);
            customMethod_MoveNext._invoker = reinterpret_cast<void *>(&BNM::MANAGEMENT_STRUCTURES::GetMethodInvoker<false, decltype(&IEnumerator::MoveNext)>::Invoke);
            customMethod_MoveNext._name = BNM_OBFUSCATE_TMP("MoveNext");
            customMethod_MoveNext._returnType = BNM::Defaults::Get<bool>();
            customMethod_MoveNext._isStatic = false;
            customMethod_MoveNext._parameterTypes = {};
            customClass._methods.push_back(&customMethod_MoveNext);
        }
        {
            constexpr auto p = &IEnumerator::Reset;
            customMethod_Reset._address = MemFnToVoidPtr(&p);
            customMethod_Reset._invoker = reinterpret_cast<void *>(&BNM::MANAGEMENT_STRUCTURES::GetMethodInvoker<false, decltype(&IEnumerator::Reset)>::Invoke);
            customMethod_Reset._name = BNM_OBFUSCATE_TMP("Reset");
            customMethod_Reset._returnType = BNM::Defaults::Get<void>();
            customMethod_Reset._isStatic = false;
            customMethod_Reset._parameterTypes = {};
            customClass._methods.push_back(&customMethod_Reset);
        }
        {
            constexpr auto p = &IEnumerator::Current;
            customMethod_Current._address = MemFnToVoidPtr(&p);
            customMethod_Current._invoker = reinterpret_cast<void *>(&BNM::MANAGEMENT_STRUCTURES::GetMethodInvoker<false, decltype(&IEnumerator::Current)>::Invoke);
            customMethod_Current._name = BNM_OBFUSCATE_TMP("get_Current");
            customMethod_Current._returnType = BNM::Defaults::Get<BNM::IL2CPP::Il2CppObject *>();
            customMethod_Current._isStatic = false;
            customMethod_Current._parameterTypes = {};
            customClass._methods.push_back(&customMethod_Current);
        }
    }
};

void BNM::Internal::SetupCoroutine() {
    Coroutine::_IEnumeratorInit::Init();

    using namespace CustomWaitData;
    customClass._size = sizeof(CustomWait);
    customClass._targetType = BNM::CompileTimeClassBuilder(BNM_OBFUSCATE_TMP("BNM.Coroutine"), BNM_OBFUSCATE_TMP("CustomWait")).Build();
    customClass._baseType = {};
    customClass._owner = {};
    customClass._interfaces = { BNM::CompileTimeClassBuilder(BNM_OBFUSCATE_TMP("System.Collections"), BNM_OBFUSCATE_TMP("IEnumerator"), BNM_OBFUSCATE_TMP("mscorlib.dll")).Build() };
    BNM::MANAGEMENT_STRUCTURES::AddClass(&customClass);
    {
        constexpr auto p = &CustomWait::Finalize;
        customMethod_Finalize._address = MemFnToVoidPtr(&p);
        customMethod_Finalize._invoker = reinterpret_cast<void *>(&BNM::MANAGEMENT_STRUCTURES::GetMethodInvoker<false, decltype(&CustomWait::Finalize)>::Invoke);
        customMethod_Finalize._name = BNM_OBFUSCATE_TMP("Finalize");
        customMethod_Finalize._returnType = BNM::Defaults::Get<void>();
        customMethod_Finalize._isStatic = false;
        customMethod_Finalize._parameterTypes = {};
        customClass._methods.push_back(&customMethod_Finalize);
    }
    {
        constexpr auto p = &CustomWait::MoveNext;
        customMethod_MoveNext._address = MemFnToVoidPtr(&p);
        customMethod_MoveNext._invoker = reinterpret_cast<void *>(&BNM::MANAGEMENT_STRUCTURES::GetMethodInvoker<false, decltype(&CustomWait::MoveNext)>::Invoke);
        customMethod_MoveNext._name = BNM_OBFUSCATE_TMP("MoveNext");
        customMethod_MoveNext._returnType = BNM::Defaults::Get<bool>();
        customMethod_MoveNext._isStatic = false;
        customMethod_MoveNext._parameterTypes = {};
        customClass._methods.push_back(&customMethod_MoveNext);
    }
    {
        constexpr auto p = &CustomWait::Reset;
        customMethod_Reset._address = MemFnToVoidPtr(&p);
        customMethod_Reset._invoker = reinterpret_cast<void *>(&BNM::MANAGEMENT_STRUCTURES::GetMethodInvoker<false, decltype(&CustomWait::Reset)>::Invoke);
        customMethod_Reset._name = BNM_OBFUSCATE_TMP("Reset");
        customMethod_Reset._returnType = BNM::Defaults::Get<void>();
        customMethod_Reset._isStatic = false;
        customMethod_Reset._parameterTypes = {};
        customClass._methods.push_back(&customMethod_Reset);
    }
    {
        constexpr auto p = &CustomWait::Current;
        customMethod_Current._address = MemFnToVoidPtr(&p);
        customMethod_Current._invoker = reinterpret_cast<void *>(&BNM::MANAGEMENT_STRUCTURES::GetMethodInvoker<true, decltype(&CustomWait::Current)>::Invoke);
        customMethod_Current._name = BNM_OBFUSCATE_TMP("get_Current");
        customMethod_Current._returnType = BNM::Defaults::Get<BNM::IL2CPP::Il2CppObject *>();
        customMethod_Current._isStatic = false;
        customMethod_Current._parameterTypes = {};
        customClass._methods.push_back(&customMethod_Current);
    }
}

void BNM::Internal::LoadCoroutine() {
    Classes::AsyncOperation = BNM::Class(BNM_OBFUSCATE_TMP("UnityEngine"), BNM_OBFUSCATE_TMP("AsyncOperation"));
    Classes::WaitForEndOfFrame = BNM::Class(BNM_OBFUSCATE_TMP("UnityEngine"), BNM_OBFUSCATE_TMP("WaitForEndOfFrame"));
    Classes::WaitForFixedUpdate = BNM::Class(BNM_OBFUSCATE_TMP("UnityEngine"), BNM_OBFUSCATE_TMP("WaitForFixedUpdate"));
    Classes::WaitForSeconds = BNM::Class(BNM_OBFUSCATE_TMP("UnityEngine"), BNM_OBFUSCATE_TMP("WaitForSeconds"));
    Classes::WaitForSecondsRealtime = BNM::Class(BNM_OBFUSCATE_TMP("UnityEngine"), BNM_OBFUSCATE_TMP("WaitForSecondsRealtime"));
}

BNM::Coroutine::AsyncOperation::AsyncOperation(intptr_t operation) { _object = Classes::AsyncOperation.CreateNewObjectParameters(operation); }

BNM::Coroutine::WaitForEndOfFrame::WaitForEndOfFrame() { _object = Classes::WaitForEndOfFrame.CreateNewInstance(); }

BNM::Coroutine::WaitForFixedUpdate::WaitForFixedUpdate() { _object = Classes::WaitForFixedUpdate.CreateNewInstance(); }

BNM::Coroutine::WaitForSeconds::WaitForSeconds(float seconds) { _object = Classes::WaitForSeconds.CreateNewObjectParameters(seconds); }

BNM::Coroutine::WaitForSecondsRealtime::WaitForSecondsRealtime(float seconds) { _object = Classes::WaitForSecondsRealtime.CreateNewObjectParameters(seconds); }

BNM::Coroutine::WaitUntil::WaitUntil(const std::function<bool()> &function) {
    auto obj = reinterpret_cast<CustomWait *>(BNM::Class(CustomWaitData::customClass.myClass).CreateNewInstance());
    obj->_func = function; obj->_isUntil = true; _object = obj;
}

BNM::Coroutine::WaitWhile::WaitWhile(const std::function<bool()> &function) {
    auto obj = reinterpret_cast<CustomWait *>(BNM::Class(CustomWaitData::customClass.myClass).CreateNewInstance());
    obj->_func = function; obj->_isUntil = false; _object = obj;
}

BNM::Coroutine::IEnumerator *BNM::Coroutine::IEnumerator::Get() {
    _current = nullptr;
    while (MoveNext()) {
        _current = Current();
        if (_current) break;
    }
    return this;
}

void BNM::Coroutine::IEnumerator::Finalize() {
    if (_coroutine) { _coroutine.destroy(); _coroutine = {}; }
}

bool BNM::Coroutine::IEnumerator::MoveNext() {
    if (!_coroutine) return false;
    if (_coroutine.done()) return false;
    _coroutine.resume();
    return true;
}

void BNM::Coroutine::IEnumerator::Reset() {
    if (_coroutine) { _coroutine.destroy(); _coroutine = {}; }
}

BNM::Coroutine::IEnumerator::Il2CppObject *BNM::Coroutine::IEnumerator::Current() {
    if (!_coroutine) return nullptr;
    return _coroutine.promise().value()._object;
}

#endif
