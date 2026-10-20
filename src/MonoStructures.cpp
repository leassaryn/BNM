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
