#include <cstring>
#include <string>
#include <cstdio>
#include <algorithm>

#include <BNM/UserSettings/GlobalSettings.hpp>
#include <BNM/Utils.hpp>
#include <BNM/Method.hpp>
#include <Internals.hpp>

using namespace BNM;

size_t BNM::ReadableBytes(const void *address, size_t maximum) {
    if (!address || !maximum) return 0;
    auto *maps = std::fopen("/proc/self/maps", "r");
    if (!maps) return 0;
    const auto target = reinterpret_cast<uintptr_t>(address);
    uintptr_t cursor = target;
    size_t remaining = maximum;
    char line[1024];
    while (std::fgets(line, sizeof(line), maps)) {
        unsigned long begin{}, end{};
        char permissions[5]{};
        if (std::sscanf(line, "%lx-%lx %4s", &begin, &end, permissions) != 3) continue;
        if (end <= cursor) continue;
        if (begin > cursor || permissions[0] != 'r') break;
        const auto available = std::min(remaining, static_cast<size_t>(end - cursor));
        remaining -= available;
        if (!remaining) break;
        cursor = end;
    }
    std::fclose(maps);
    return maximum - remaining;
}

Structures::Mono::String *BNM::CreateMonoString(const std::string_view &str) {
    // il2cpp_string_new expects a null-terminated C string; string_view may not be one.
    std::string cStr(str);
    return Internal::il2cppMethods.il2cpp_string_new ? Internal::il2cppMethods.il2cpp_string_new(cStr.c_str()) : nullptr;
}

void *BNM::GetExternMethod(const std::string_view &str) {
    // il2cpp_resolve_icall expects a null-terminated C string; string_view may not be one.
    std::string cStr(str);
    auto ret = Internal::il2cppMethods.il2cpp_resolve_icall ? Internal::il2cppMethods.il2cpp_resolve_icall(cStr.c_str()) : nullptr;
    BNM_LOG_WARN_IF(!ret, DBG_BNM_MSG_GetExternMethod_Warn, cStr.c_str());
    return ret;
}

bool BNM::IsLoaded() {
    return Internal::states.state;
}

void *BNM::GetIl2CppLibraryHandle() {
    std::lock_guard lock(Internal::configurationMutex);
    return Internal::il2cppLibraryHandle;
}

bool BNM::InvokeHookImpl(IL2CPP::MethodInfo *info, void *newMet, void **oldMet) {
    if (!info) return false;
    if (oldMet) *oldMet = reinterpret_cast<void *>(info->methodPointer);
    info->methodPointer = reinterpret_cast<IL2CPP::Il2CppMethodPointer>(newMet);
    return true;
}

IL2CPP::VirtualInvokeData *BNM::Internal::ResolveVirtualSlot(IL2CPP::Il2CppClass *target,
                                                         IL2CPP::MethodInfo *method)
{
    constexpr uint16_t invalidSlot = 65535;
    constexpr uint16_t staticMethod = 0x0010;
    constexpr uint16_t virtualMethod = 0x0040;
    constexpr uint32_t interfaceType = 0x0020;
    if (!target || !method || method->slot == invalidSlot ||
        (method->flags & staticMethod) || !(method->flags & virtualMethod)) return nullptr;

    auto *declaring = BNM::PRIVATE_INTERNAL::GetMethodClass(method);
    if (!declaring) return nullptr;
    target = Class(target).GetClass();

    uint32_t slot = method->slot;
    if (declaring->flags & interfaceType)
    {
        if (!target->interfaceOffsets) return nullptr;
        bool found = false;
        for (uint16_t i = 0; i < target->interface_offsets_count; ++i)
        {
            const auto &entry = target->interfaceOffsets[i];
            if (entry.interfaceType != declaring) continue;
            if (entry.offset < 0) return nullptr;
            slot += static_cast<uint32_t>(entry.offset);
            found = true;
            break;
        }
        if (!found) return nullptr;
    }
    else
    {
        auto *parent = target;
        while (parent && parent != declaring) parent = parent->parent;
        if (!parent) return nullptr;
    }

    if (slot >= target->vtable_count) return nullptr;
    auto *entry = &target->vtable[slot];
    return entry->method ? entry : nullptr;
}

bool BNM::VirtualHookImpl(Class targetClass, IL2CPP::MethodInfo *info, void *newMet, void **oldMet)
{
    if (!newMet) return false;
    auto *slot = Internal::ResolveVirtualSlot(targetClass._data, info);
    if (!slot || !slot->methodPtr) return false;
    if (oldMet) *oldMet = reinterpret_cast<void *>(slot->methodPtr);
    slot->methodPtr = reinterpret_cast<IL2CPP::Il2CppMethodPointer>(newMet);
    return true;
}

template<> bool BNM::IsA<IL2CPP::Il2CppObject *>(IL2CPP::Il2CppObject *object, IL2CPP::Il2CppClass *_class) {
    if (!object || !_class) return false;
    for (auto cls = object->klass; cls; cls = cls->parent) if (cls == _class) return true;
    return false;
}

#ifdef BNM_DEBUG

static const char *CompileTimeClassModifiers[] = {
        "None",
        "Array",
        "Pointer",
        "Reference"
};

static void LogCompileTimeClassInfo(CompileTimeClass::_BaseInfo *info, const CompileTimeClass &tmp) {
    switch (info->_baseType) {
        case CompileTimeClass::_BaseType::None:
            BNM_LOG_ERR("\tNone");
            break;
        case CompileTimeClass::_BaseType::Class: {
            auto classInfo = static_cast<CompileTimeClass::_ClassInfo *>(info);
            BNM_LOG_ERR("\tClass( imageName: \"%s\", namespace: \"%s\", name: \"%s\") - %s", classInfo->_imageName, classInfo->_namespace, classInfo->_name, tmp._loadedClass.str().data());
        } break;
        case CompileTimeClass::_BaseType::Inner: {
            auto innerInfo = static_cast<CompileTimeClass::_InnerInfo *>(info);
            BNM_LOG_ERR("\tClass( name: \"%s\") - %s", innerInfo->_name, tmp._loadedClass.str().data());
        } break;
        case CompileTimeClass::_BaseType::Modifier: {
            BNM_LOG_ERR("\tModifier(\"%s\") - %s", CompileTimeClassModifiers[BNM::to_underlying(static_cast<CompileTimeClass::_ModifierInfo *>(info)->_modifierType)], tmp._loadedClass.str().data());
        } break;
        case CompileTimeClass::_BaseType::Generic: {
            auto genericInfo = static_cast<CompileTimeClass::_GenericInfo *>(info);
            BNM_LOG_ERR("\tGeneric: ");
            for (auto type : genericInfo->_types) BNM_LOG_ERR("\t\t%s", type.ToClass().str().data());
            BNM_LOG_ERR("\t%s", tmp._loadedClass.str().data());
        } break;
        case CompileTimeClass::_BaseType::MaxCount: break;
    }
}

namespace CompileTimeClassProcessors {
    typedef void (*ProcessorType)(CompileTimeClass &target, CompileTimeClass::_BaseInfo *info);
    extern ProcessorType processors[BNM::to_underlying(CompileTimeClass::_BaseType::MaxCount)];
}

void Utils::LogCompileTimeClass(const CompileTimeClass &compileTimeClass) {
    if (compileTimeClass._stack.IsEmpty()) return BNM_LOG_ERR("\t" DBG_BNM_MSG_ClassesManagement_LogCompileTimeClass_None);

    CompileTimeClass tmp{};

    auto &stack = compileTimeClass._stack;
    auto lastElement = stack.lastElement;
    auto current = lastElement->next;
    do {
        auto info = current->value;
        auto index = BNM::to_underlying(info->_baseType);
        if (index >= BNM::to_underlying(CompileTimeClass::_BaseType::MaxCount)) {
            BNM_LOG_ERR("\t" DBG_BNM_MSG_CompileTimeClass_ToClass_OoB_Warn, (unsigned long) index);
            current = current->next;
            continue;
        }
        CompileTimeClassProcessors::processors[index](tmp, info);

        LogCompileTimeClassInfo(info, tmp);

        current = current->next;
    } while (current != lastElement->next);
}
#endif

bool BNM::AttachIl2Cpp() {
    if (CurrentIl2CppThread()) return false;
    if (!Internal::il2cppMethods.il2cpp_thread_attach || !Internal::il2cppMethods.il2cpp_domain_get) return false;
    auto *domain = Internal::il2cppMethods.il2cpp_domain_get();
    return domain && Internal::il2cppMethods.il2cpp_thread_attach(domain);
}

IL2CPP::Il2CppThread *BNM::CurrentIl2CppThread() {
    if (!Internal::il2cppMethods.il2cpp_thread_current || !Internal::il2cppMethods.il2cpp_domain_get) return nullptr;
    auto *domain = Internal::il2cppMethods.il2cpp_domain_get();
    return domain ? Internal::il2cppMethods.il2cpp_thread_current(domain) : nullptr;
}

void BNM::DetachIl2Cpp() {
    auto thread = BNM::CurrentIl2CppThread();
    if (!thread || !Internal::il2cppMethods.il2cpp_thread_detach) return;
    Internal::il2cppMethods.il2cpp_thread_detach(thread);
}

BNM::ScopedIl2CppThread::ScopedIl2CppThread() {
    _thread = CurrentIl2CppThread();
    if (_thread) return;
    const auto &api = Internal::il2cppMethods;
    if (!api.il2cpp_thread_current || !api.il2cpp_domain_get ||
        !api.il2cpp_thread_attach || !api.il2cpp_thread_detach) return;
    auto *domain = api.il2cpp_domain_get();
    if (!domain) return;
    _thread = api.il2cpp_thread_attach(domain);
    _owns = _thread != nullptr;
}

BNM::ScopedIl2CppThread::~ScopedIl2CppThread() noexcept {
    if (!_owns) return;
    if (_owner != std::this_thread::get_id() || CurrentIl2CppThread() != _thread) {
        BNM_LOG_ERR("ScopedIl2CppThread must outlive its attachment on the creating thread");
        return;
    }
    if (Internal::il2cppMethods.il2cpp_thread_detach)
        Internal::il2cppMethods.il2cpp_thread_detach(_thread);
}

bool BNM::StoreManagedReference(IL2CPP::Il2CppObject *owner, void *slot, void *value) {
    if (!slot || !Internal::il2cppMethods.il2cpp_gc_wbarrier_set_field) return false;
    Internal::il2cppMethods.il2cpp_gc_wbarrier_set_field(owner, static_cast<void **>(slot), value);
    return true;
}

namespace {
    // Fields in value types use boxed offsets; destination points to unboxed bytes.
    void BarrierValueFields(IL2CPP::Il2CppObject *owner, std::byte *destination,
                            size_t size, IL2CPP::Il2CppClass *klass) {
        for (uint16_t i = 0; i < klass->field_count; ++i) {
            const auto &field = klass->fields[i];
            if (!field.type || (field.type->attrs & 0x0010) ||
                field.offset < static_cast<int32_t>(sizeof(IL2CPP::Il2CppObject))) continue;
            const auto offset = static_cast<size_t>(field.offset) - sizeof(IL2CPP::Il2CppObject);
            if (offset >= size) continue;
            auto *type = field.type;
            const auto kind = type->type;
            if (kind == IL2CPP::IL2CPP_TYPE_STRING || kind == IL2CPP::IL2CPP_TYPE_OBJECT ||
                kind == IL2CPP::IL2CPP_TYPE_CLASS || kind == IL2CPP::IL2CPP_TYPE_ARRAY ||
                kind == IL2CPP::IL2CPP_TYPE_SZARRAY) {
                if (sizeof(void *) > size - offset) continue;
                void *value{};
                std::memcpy(&value, destination + offset, sizeof(value));
                (void)BNM::StoreManagedReference(owner, destination + offset, value);
            } else if (kind == IL2CPP::IL2CPP_TYPE_VALUETYPE || kind == IL2CPP::IL2CPP_TYPE_GENERICINST) {
                auto *nested = Class(type).GetClass();
                if (!nested) continue;
#if UNITY_VER < 211
                const bool isValue = nested->valuetype;
#else
                const bool isValue = nested->byval_arg.valuetype;
#endif
                if (isValue) {
                    if (nested->has_references) BarrierValueFields(owner, destination + offset, size - offset, nested);
                } else {
                    if (sizeof(void *) > size - offset) continue;
                    void *value{};
                    std::memcpy(&value, destination + offset, sizeof(value));
                    (void)BNM::StoreManagedReference(owner, destination + offset, value);
                }
            }
        }
    }
}

bool BNM::CopyManagedValue(IL2CPP::Il2CppObject *owner, void *destination, const void *source,
                           size_t size, IL2CPP::Il2CppClass *valueClass) {
    if (!destination || !source) return false;
    const bool references = valueClass && valueClass->has_references;
    if (references && !Internal::il2cppMethods.il2cpp_gc_wbarrier_set_field) return false;
    std::memmove(destination, source, size);
    if (references) BarrierValueFields(owner, static_cast<std::byte *>(destination), size, valueClass);
    return true;
}

void *BNM::Allocate(size_t size) {
    return Internal::il2cppMethods.il2cpp_gc_alloc_fixed ? Internal::il2cppMethods.il2cpp_gc_alloc_fixed(size) : nullptr;
}

void BNM::Free(void *ptr) {
    if (ptr && Internal::il2cppMethods.il2cpp_gc_free_fixed) Internal::il2cppMethods.il2cpp_gc_free_fixed(ptr);
}
