#include <cstring>

#include <BNM/Coroutine.hpp>

#if defined(BNM_CLASSES_MANAGEMENT) && defined(BNM_COROUTINE)

#include <Internals.hpp>

namespace Classes {
    static BNM::Class AsyncOperation{}, WaitForEndOfFrame{}, WaitForFixedUpdate{}, WaitForSeconds{}, WaitForSecondsRealtime{};
}

namespace IEnumeratorData {
    static BNM::MANAGEMENT_STRUCTURES::CustomClass customClass{};
    static BNM::MANAGEMENT_STRUCTURES::CustomField currentField{};
    static BNM::MANAGEMENT_STRUCTURES::CustomMethod customMethod_Finalize{}, customMethod_MoveNext{}, customMethod_Reset{}, customMethod_Current{};
}

namespace CustomWaitData {
    static BNM::MANAGEMENT_STRUCTURES::CustomClass customClass{};
    static BNM::MANAGEMENT_STRUCTURES::CustomMethod customMethod_Finalize{}, customMethod_MoveNext{}, customMethod_Reset{}, customMethod_Current{};
}

struct CustomWait : BNM::IL2CPP::Il2CppObject {
    std::function<bool()> _func{};
    bool _isUntil = false;
    bool _constructed = false;
    void Finalize() {
        if (_constructed) { _func.~function(); _constructed = false; }
    }
    [[nodiscard]] bool MoveNext() const {
        if (!_constructed || !_func) return false;
        try { return _isUntil == !_func(); }
        catch (...) { BNM_LOG_ERR("Coroutine predicate threw an exception"); return false; }
    }
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
        // Expose the current yielded object to the managed GC descriptor.
        currentField._name = "_current";
        currentField._size = sizeof(BNM::IL2CPP::Il2CppObject *);
        currentField._type = BNM::Defaults::Get<BNM::IL2CPP::Il2CppObject *>();
        IEnumerator layout;
        currentField.offset = reinterpret_cast<BNM_PTR>(&layout._current) - reinterpret_cast<BNM_PTR>(&layout);
        customClass._fields = {&currentField};
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
    if (!obj) return;
    new (&obj->_func) std::function<bool()>(function);
    obj->_constructed = true; obj->_isUntil = true; _object = obj;
}

BNM::Coroutine::WaitWhile::WaitWhile(const std::function<bool()> &function) {
    auto obj = reinterpret_cast<CustomWait *>(BNM::Class(CustomWaitData::customClass.myClass).CreateNewInstance());
    if (!obj) return;
    new (&obj->_func) std::function<bool()>(function);
    obj->_constructed = true; obj->_isUntil = false; _object = obj;
}

BNM::Coroutine::IEnumerator *BNM::Coroutine::IEnumerator::Get() {
    if (!_coroutine) return reinterpret_cast<IEnumerator *>(_current);
    auto *object = reinterpret_cast<IEnumerator *>(Class(IEnumeratorData::customClass.myClass).CreateNewInstance());
    if (!object) return nullptr;
    // IL2CPP owns the proxy; its finalizer owns the transferred native frame.
    // Do not advance here: Unity's first MoveNext must observe the first yield,
    // including a null yield (wait one frame).
    new (&object->_coroutine) std::coroutine_handle<promise_type>(std::exchange(_coroutine, {}));
    (void)StoreManagedValue(object, &object->_current, static_cast<IL2CPP::Il2CppObject *>(nullptr));
    _current = object;
    return object;
}

void BNM::Coroutine::IEnumerator::Finalize() {
    if (_coroutine) { _coroutine.destroy(); _coroutine = {}; }
    (void)StoreManagedValue(this, &_current, static_cast<IL2CPP::Il2CppObject *>(nullptr));
}

bool BNM::Coroutine::IEnumerator::MoveNext() {
    if (!_coroutine || _coroutine.done()) return false;
    _coroutine.resume();
    (void)StoreManagedValue(this, &_current, _coroutine.done() ? nullptr : _coroutine.promise().value()._object);
    if (_coroutine.promise()._exception) {
        BNM_LOG_ERR("Coroutine body threw an exception; enumeration stopped");
        return false;
    }
    return !_coroutine.done();
}

void BNM::Coroutine::IEnumerator::Reset() {
    if (_coroutine) { _coroutine.destroy(); _coroutine = {}; }
    (void)StoreManagedValue(this, &_current, static_cast<IL2CPP::Il2CppObject *>(nullptr));
}

BNM::Coroutine::IEnumerator::Il2CppObject *BNM::Coroutine::IEnumerator::Current() {
    if (!_coroutine || _coroutine.done()) return nullptr;
    return _coroutine.promise().value()._object;
}

#endif
