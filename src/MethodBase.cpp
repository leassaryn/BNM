#include <BNM/MethodBase.hpp>
#include <BNM/DebugMessages.hpp>
#include <Internals.hpp>

using namespace BNM;

MethodBase::MethodBase(const IL2CPP::MethodInfo *info)  {
    if (!BNM::IsAllocated(info)) return;

    _isStatic = (info->flags & 0x0010) == 0x0010;
    _isVirtual = info->slot != 65535;
    _data = (decltype(_data)) info;
}

MethodBase::MethodBase(const IL2CPP::Il2CppReflectionMethod *reflectionMethod) {
    if (!BNM::IsAllocated(reflectionMethod) || !BNM::IsAllocated(reflectionMethod->method)) return;

    auto info = reflectionMethod->method;

    _isStatic = (info->flags & 0x0010) == 0x0010;
    _isVirtual = info->slot != 65535;
    _data = (decltype(_data)) info;
}

MethodBase &MethodBase::SetInstance(IL2CPP::Il2CppObject *val)  {
    if (!_data) return *this;
    if (_isStatic) {
        BNM_LOG_WARN(DBG_BNM_MSG_MethodBase_SetInstance_Warn, str().c_str());
        return *this;
    }
#ifdef BNM_CHECK_INSTANCE_TYPE
    _instance = nullptr;
    if (!val) return *this;
    if (BNM::IsA(val, BNM::PRIVATE_INTERNAL::GetMethodClass(_data)) ||
        Internal::ResolveVirtualSlot(val->klass, _data)) _instance = val;
    else BNM_LOG_ERR(DBG_BNM_MSG_MethodBase_SetInstance_Wrong_Instance_Error, BNM::Class(val).str().c_str(), str().c_str());
#else
    _instance = val;
#endif
    return *this;
}

MethodBase MethodBase::GetGeneric(const std::initializer_list<CompileTimeClass> &templateTypes) const {
    if (!_data) return {};
    BNM_LOG_WARN_IF(!_data->is_generic, DBG_BNM_MSG_MethodBase_GetGeneric_Warn, str().c_str());
    if (!_data->is_generic) return {};
    return Internal::TryMakeGenericMethod(*this, templateTypes);
}

MethodBase MethodBase::GetOverride() const {
    if (!_data || _isStatic || (_data->flags & 0x0040) == 0) return {};
    if (!BNM::IsAllocated(_instance)) {
        BNM_LOG_WARN(DBG_BNM_MSG_MethodBase_Virtualize_Warn, str().c_str());
        return {};
    }

    auto *slot = Internal::ResolveVirtualSlot(_instance->klass, _data);
    if (!slot) return {};
    MethodBase result(slot->method);
    result._instance = _instance;
    return result;
}

BNM::Class MethodBase::GetReturnType() const {
    if (!_data) return {};
    return _data->return_type;
}

BNM::Class MethodBase::GetParentClass() const {
    if (!_data) return {};
    return BNM::PRIVATE_INTERNAL::GetMethodClass(_data);
}
