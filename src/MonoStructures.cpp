#include <new>
#include <memory>

#include <utf8.h>

#include <BNM/BasicMonoStructures.hpp>
#include <BNM/Class.hpp>
#include <Internals.hpp>

static std::string Utf16ToUtf8(BNM::IL2CPP::Il2CppChar *utf16String, size_t length) {
    std::string utf8String{};
    utf8String.reserve(length);
    utf8::unchecked::utf16to8(utf16String, utf16String + length, std::back_inserter(utf8String));
    return utf8String;
}

using namespace BNM::Structures::Mono;

void *__Internal_Array::ArrayFromClass(IL2CPP::Il2CppClass *cls, IL2CPP::il2cpp_array_size_t capacity) {
    return reinterpret_cast<void *>(BNM::Class(cls).NewArray<void *>(capacity));
}

std::string String::str() {
    BNM_CHECK_SELF(DBG_BNM_MSG_String_str_Error);
    if (!length) return {};
    return Utf16ToUtf8(chars, static_cast<size_t>(length));
}

unsigned int String::GetHash() const {
    BNM_CHECK_SELF(0);
    const IL2CPP::Il2CppChar *p = chars;
    unsigned int h = 0;
    for (int i = 0; i < length; ++i) { h = (h << 5) - h + *p; p++; }
    return h;
}

String *String::Empty() {
    return Internal::vmData.String$$Empty ? *Internal::vmData.String$$Empty : nullptr;
}

#ifdef BNM_ALLOW_SELF_CHECKS
bool String::SelfCheck() const {
    if (CheckForNull(this)) return true;
    BNM_LOG_ERR(DBG_BNM_MSG_String_SelfCheck_Error);
    return false;
}
#endif

// The only normal way to call CompareExchange for SyncRoot is for List
void *PRIVATE_MonoListData::CompareExchange4List(void *syncRoot) {
    if (Internal::vmData.Interlocked$$CompareExchange.IsValid()) Internal::vmData.Interlocked$$CompareExchange(reinterpret_cast<void **>(&syncRoot), reinterpret_cast<void *>(Internal::vmData.Object.CreateNewInstance()), nullptr);
    return syncRoot;
}

namespace {
    struct ListMetadataDeleter {
        template<typename T> void operator()(T *ptr) const noexcept { BNM_free(ptr); }
    };
}

// Build privately; cache publication is the only ownership-transfer point.
BNM::IL2CPP::Il2CppClass *BNM::Structures::Mono::PRIVATE_MonoListData::TryGetMonoListClass(uint32_t typeHash, std::array<PRIVATE_MonoListData::MethodData, 16> &data) try {
    static std::recursive_mutex cacheMutex;
    std::lock_guard lock(cacheMutex);
    if (auto found = Internal::customListsMap.find(typeHash); found != Internal::customListsMap.end() && found->second)
        return found->second;

    std::map<size_t, std::unique_ptr<IL2CPP::MethodInfo, ListMetadataDeleter>> createdMethods;
    auto *templateClass = Internal::customListTemplateClass.GetClass();
    if (!templateClass) return nullptr;
    const auto size = sizeof(IL2CPP::Il2CppClass) + templateClass->vtable_count * sizeof(IL2CPP::VirtualInvokeData);
    std::unique_ptr<IL2CPP::Il2CppClass, ListMetadataDeleter> typedClass(
        static_cast<IL2CPP::Il2CppClass *>(BNM_malloc(size)));
    if (!typedClass) return nullptr;
    std::memcpy(typedClass.get(), templateClass, size);
    for (uint16_t i = 4 /* Skip System.Object methods. */; i < typedClass->vtable_count; ++i) {
        auto &current = typedClass->vtable[i];
        if (!current.method || !current.method->name) continue;
        auto name = std::string_view(current.method->name);
        auto dot = name.rfind('.');
        if (dot != std::string_view::npos) name = name.substr(dot + 1);
        auto method = std::find_if(data.begin(), data.end(), [name](const auto &entry) { return entry.methodName == name; });
        if (method == data.end()) continue;
        auto &info = createdMethods[FNV1a(name)];
        if (!info) {
            info.reset(static_cast<IL2CPP::MethodInfo *>(BNM_malloc(sizeof(IL2CPP::MethodInfo))));
            if (!info) return nullptr;
            *info = *current.method;
            info->methodPointer = reinterpret_cast<IL2CPP::Il2CppMethodPointer>(method->ptr);
        }
        current.method = info.get();
        current.methodPtr = info->methodPointer;
    }
    auto [entry, inserted] = Internal::customListsMap.try_emplace(typeHash, typedClass.get());
    if (!inserted && entry->second) return entry->second;
    entry->second = typedClass.get();
    for (auto &[hash, info] : createdMethods) (void)info.release();
    return typedClass.release();
} catch (const std::bad_alloc &) {
    return nullptr;
}
