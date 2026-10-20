#include "X86Decoder.hpp"
#include <cstring>

#include <BNM/UserSettings/GlobalSettings.hpp>
#include <BNM/UserSettings/Il2CppMethodNames.hpp>
#include <BNM/Loading.hpp>
#include <BNM/Field.hpp>

#include <Internals.hpp>

using namespace BNM;

void Internal::Load() try {
    if (states.state) return;
    bool expected = false;
    if (!isLoading.compare_exchange_strong(expected, true)) return;
    struct ResetLoading { ~ResetLoading() { isLoading.store(false); } } resetLoading;
    if (states.state) return;
    static bool runtimeSetupStarted = false;
    if (runtimeSetupStarted) {
        BNM_LOG_ERR("BNM metadata setup previously failed; process restart required");
        return;
    }

    // Do not publish partially resolved runtime tables.
    if (!SetupBNM()) {
        BNM_LOG_ERR("BNM setup failed: runtime symbols, metadata or hooks unavailable");
        return;
    }

    runtimeSetupStarted = true;
    BNM::Internal::LoadDefaults();

#ifdef BNM_CLASSES_MANAGEMENT

#ifdef BNM_COROUTINE
    BNM::Internal::SetupCoroutine();
#endif

    if (!BNM::Internal::ClassesManagement::ProcessCustomClasses()) {
        BNM_LOG_ERR("BNM custom metadata setup failed; loaded state withheld");
        return;
    }

#ifdef BNM_COROUTINE
    BNM::Internal::LoadCoroutine();
#endif

#endif
    states.state = true;

    // Call all events after loading il2cpp
    BNM::ForwardList<void(*)()> events;
    {
        std::lock_guard lock(eventsMutex);
        events = onIl2CppLoaded;
    }
    if (events.IsEmpty()) return;
    auto current = events.lastElement->next;
    do {
        try { current->value(); }
        catch (...) { BNM_LOG_ERR("BNM on-loaded callback threw an exception"); }

        current = current->next;
    } while (current != events.lastElement->next);
} catch (const std::exception &error) {
    BNM_LOG_ERR("BNM loading/callback exception: %s", error.what());
} catch (...) {
    BNM_LOG_ERR("BNM loading/callback threw an unknown exception");
}

void *Internal::GetIl2CppMethod(const char *methodName) {
    std::lock_guard lock(configurationMutex);
    return currentFinderMethod ? currentFinderMethod(methodName, currentFinderData) : nullptr;
}

void Loading::AllowLateInitHook() {
    Internal::states.lateInitAllowed = true;
}

static bool CheckHandle(void *handle) {
    if (!handle) return false;
    std::lock_guard lock(Internal::configurationMutex);
    if (Internal::states.state || Internal::BNM_il2cpp_init_origin) return false;
    void *init = BNM_dlsym(handle, BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_init));
    if (!init) return false;

    Internal::BNM_il2cpp_init_origin = ::BasicHook(init, Internal::BNM_il2cpp_init, Internal::old_BNM_il2cpp_init);
    if (!Internal::BNM_il2cpp_init_origin) return false;

    if (Internal::states.lateInitAllowed) Internal::LateInit(BNM_dlsym(handle, BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_class_from_il2cpp_type)));

    Internal::il2cppLibraryHandle = handle;
    return true;
}

#if defined(__ARM_ARCH_7A__)
#    define CURRENT_ARCH "armeabi-v7a"
#elif defined(__aarch64__)
#    define CURRENT_ARCH "arm64-v8a"
#elif defined(__i386__)
#    define CURRENT_ARCH "x86"
#elif defined(__x86_64__)
#    define CURRENT_ARCH "x86_64"
#elif defined(__riscv)
#    define CURRENT_ARCH "riscv64"
#endif

bool Loading::TryLoadByJNI(JNIEnv *env, jobject context) {
    if (!env || env->ExceptionCheck()) return false;
    std::lock_guard lock(Internal::configurationMutex);
    if (Internal::il2cppLibraryHandle || Internal::states.state) return false;
    if (env->PushLocalFrame(24) < 0) return false;
    struct LocalFrame {
        JNIEnv *env;
        ~LocalFrame() { env->PopLocalFrame(nullptr); }
    } frame{env};

    // Preserve pending Java exceptions for the caller; never call through a failed ID lookup.
    if (!context) {
        auto activityThread = env->FindClass(BNM_OBFUSCATE_TMP("android/app/ActivityThread"));
        if (env->ExceptionCheck() || !activityThread) return false;
        auto getThread = env->GetStaticMethodID(activityThread, BNM_OBFUSCATE_TMP("currentActivityThread"), BNM_OBFUSCATE_TMP("()Landroid/app/ActivityThread;"));
        if (env->ExceptionCheck() || !getThread) return false;
        auto thread = env->CallStaticObjectMethod(activityThread, getThread);
        if (env->ExceptionCheck() || !thread) return false;
        auto getApplication = env->GetMethodID(activityThread, BNM_OBFUSCATE_TMP("getApplication"), BNM_OBFUSCATE_TMP("()Landroid/app/Application;"));
        if (env->ExceptionCheck() || !getApplication) return false;
        context = env->CallObjectMethod(thread, getApplication);
        if (env->ExceptionCheck() || !context) return false;
    }
    auto contextClass = env->GetObjectClass(context);
    if (env->ExceptionCheck() || !contextClass) return false;
    auto getInfo = env->GetMethodID(contextClass, BNM_OBFUSCATE_TMP("getApplicationInfo"), BNM_OBFUSCATE_TMP("()Landroid/content/pm/ApplicationInfo;"));
    if (env->ExceptionCheck() || !getInfo) return false;
    auto info = env->CallObjectMethod(context, getInfo);
    if (env->ExceptionCheck() || !info) return false;
    auto infoClass = env->GetObjectClass(info);
    if (env->ExceptionCheck() || !infoClass) return false;
    auto flagsId = env->GetFieldID(infoClass, BNM_OBFUSCATE_TMP("flags"), "I");
    if (env->ExceptionCheck() || !flagsId) return false;
    const auto flags = env->GetIntField(info, flagsId);
    if (env->ExceptionCheck()) return false;
    const bool extracted = (flags & 0x10000000) != 0;
    auto pathId = env->GetFieldID(infoClass, extracted ? BNM_OBFUSCATE_TMP("nativeLibraryDir") : BNM_OBFUSCATE_TMP("sourceDir"), "Ljava/lang/String;");
    if (env->ExceptionCheck() || !pathId) return false;
    auto path = static_cast<jstring>(env->GetObjectField(info, pathId));
    if (env->ExceptionCheck()) return false;
    auto tryPath = [&](jstring javaPath, bool isApk) {
        if (!javaPath) return false;
        const auto *chars = env->GetStringUTFChars(javaPath, nullptr);
        if (!chars) return false;
        struct UtfChars {
            JNIEnv *env; jstring string; const char *chars;
            ~UtfChars() { env->ReleaseStringUTFChars(string, chars); }
        } utf{env, javaPath, chars};
        std::string file(chars);
        file += isApk ? BNM_OBFUSCATE_TMP("!/lib/" CURRENT_ARCH "/libil2cpp.so") : BNM_OBFUSCATE_TMP("/libil2cpp.so");
        auto *handle = BNM_dlopen(file.c_str(), RTLD_LAZY);
        if (!handle) return false;
        if (CheckHandle(handle)) return true;
        BNM_dlclose(handle);
        return false;
    };
    if (tryPath(path, !extracted)) return true;
    if (env->ExceptionCheck() || extracted) return false;

    // Installed split filenames are package-defined, not derived from the ABI name.
    auto splitsId = env->GetFieldID(infoClass, BNM_OBFUSCATE_TMP("splitSourceDirs"), "[Ljava/lang/String;");
    if (env->ExceptionCheck() || !splitsId) return false;
    auto splits = static_cast<jobjectArray>(env->GetObjectField(info, splitsId));
    if (env->ExceptionCheck() || !splits) return false;
    const auto count = env->GetArrayLength(splits);
    if (env->ExceptionCheck()) return false;
    for (jsize i = 0; i < count; ++i) {
        auto split = static_cast<jstring>(env->GetObjectArrayElement(splits, i));
        if (env->ExceptionCheck()) return false;
        const bool loaded = tryPath(split, true);
        env->DeleteLocalRef(split);
        if (loaded) return true;
        if (env->ExceptionCheck()) return false;
    }
    return false;
}

bool Loading::TryLoadByDlfcnHandle(void *handle) {
    return CheckHandle(handle);
}

void Loading::SetMethodFinder(BNM::Loading::MethodFinder finderMethod, void *userData) {
    std::lock_guard lock(Internal::configurationMutex);
    if (Internal::states.state || Internal::isLoading || Internal::BNM_il2cpp_init_origin) return;
    Internal::currentFinderMethod = finderMethod;
    Internal::currentFinderData = userData;
}

bool Loading::TryLoadByUsersFinder() {
    std::lock_guard lock(Internal::configurationMutex);
    if (Internal::states.state || Internal::BNM_il2cpp_init_origin) return false;
    auto init = Internal::GetIl2CppMethod(BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_init));
    if (!init) return false;

    Internal::BNM_il2cpp_init_origin = ::BasicHook(init, Internal::BNM_il2cpp_init, Internal::old_BNM_il2cpp_init);
    if (!Internal::BNM_il2cpp_init_origin) return false;

    if (Internal::states.lateInitAllowed) Internal::LateInit(Internal::currentFinderMethod(BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_class_from_il2cpp_type), Internal::currentFinderData));

    return true;
}

void Loading::TrySetupByUsersFinder() {
    return Internal::Load();
}

namespace AssemblerUtils {
    // Reverse hexadecimal string (from 001122 to 221100)
    static std::string ReverseHexString(const std::string &hex) {
        std::string out{};
        for (size_t i = 0; i < hex.length(); i += 2) out.insert(0, hex.substr(i, 2));
        return out;
    }

    // Convert hexadecimal string to a value
    static BNM_PTR HexStr2Value(const std::string &hex) { return static_cast<BNM_PTR>(strtoull(hex.c_str(), nullptr, 16)); }

#if defined(__ARM_ARCH_7A__)

    // Check if the assembly is `bl ...`or `b ...`
    static bool IsBranchHex(const std::string &hex) {
        BNM_PTR hexW = HexStr2Value(ReverseHexString(hex));
        return (hexW & 0x0A000000) == 0x0A000000;
    }

#elif defined(__aarch64__)

    // Check if the assembly is `bl ...`or `b ...`
    static bool IsBranchHex(const std::string &hex) {
        BNM_PTR hexW = HexStr2Value(ReverseHexString(hex));
        return (hexW & 0xFC000000) == 0x14000000 || (hexW & 0xFC000000) == 0x94000000;
    }

#elif defined(__i386__) || defined(__x86_64__)

    // Check if the assembly is `call ...`
    static bool IsCallHex(const std::string &hex) { return hex[0] == 'E' && hex[1] == '8'; }
#elif defined(__riscv)
#error "Is it released for Android?"
#else
#error "BNM only supports arm64, arm, x86 and x86_64"
#endif
    static const char *hexChars = BNM_OBFUSCATE_TMP("0123456789ABCDEF");
    // Прочитать память, как шестнадцатеричную строку
    // Read the memory as a hexadecimal string
    template<size_t len>
    static std::string ReadMemory(BNM_PTR address) {
        unsigned char temp[len]{};
        std::string ret{};
        memcpy(temp, reinterpret_cast<void *>(address), len);
        ret.resize(len * 2, 0);
        auto buf = reinterpret_cast<char *>(ret.data());
        for (size_t i = 0; i < len; ++i) {
            *buf++ = hexChars[temp[i] >> 4];
            *buf++ = hexChars[temp[i] & 0x0F];
        }
        return ret;
    }

    // Decode b or bl and get the address it goes to
    static bool DecodeBranchOrCall(const std::string &hex, BNM_PTR offset, BNM_PTR &outOffset) {
#if defined(__ARM_ARCH_7A__) || defined(__aarch64__)
        if (!IsBranchHex(hex)) return false;
#if defined(__aarch64__)
        uint8_t add = 0;
#else
        uint8_t add = 8;
#endif
        // This line is based on the capstone code
#if defined(__aarch64__)
        // AArch64 B/BL: imm26 in bits 0..25, word-scaled (<< 2) and sign-extended
        auto insn = static_cast<uint32_t>(HexStr2Value(ReverseHexString(hex)));
        auto imm = static_cast<int32_t>(insn << 6) >> 4;
        outOffset = static_cast<BNM_PTR>(static_cast<int64_t>(imm)) + offset + add;
#else
        // ARMv7 B/BL: imm24 in bits 0..23, word-scaled (<< 2) and sign-extended
        outOffset = static_cast<BNM_PTR>(static_cast<int32_t>((((static_cast<uint32_t>(HexStr2Value(ReverseHexString(hex))) & (((static_cast<uint32_t>(1) << 24) - 1) << 0)) >> 0) << 2) << (32 - 26)) >> (32 - 26)) + offset + add;
#endif
#elif defined(__i386__) || defined(__x86_64__)
        if (!IsCallHex(hex)) return false;
        // Address + address from the `call` + size of the instruction
        const auto displacement = static_cast<int32_t>(static_cast<uint32_t>(HexStr2Value(ReverseHexString(hex).substr(0, 8))));
        outOffset = offset + static_cast<BNM_PTR>(static_cast<intptr_t>(displacement)) + 5;
#else
#error "BNM only supports arm64, arm, x86 and x86_64"
        return false;
#endif
        return true;
    }

    // Goes through memory and tries to find b-, bl- or call instructions
    // Then gets the address they go to
    // index: 1 is the first, 2 is the second, etc.
    static BNM_PTR FindNextJump(BNM_PTR start, uint8_t index) {
        const auto readable = BNM::ReadableBytes(reinterpret_cast<const void *>(start), 4096);
        if (!start || !index) return 0;
        BNM_PTR target{};
#if defined(__ARM_ARCH_7A__) || defined(__aarch64__)
        for (size_t offset = 0; offset + 4 <= readable; offset += 4) {
            if (DecodeBranchOrCall(ReadMemory<4>(start + offset), start + offset, target) && --index == 0)
                return target;
        }
#else
        for (size_t offset = 0; offset < readable;) {
            const auto *bytes = reinterpret_cast<const uint8_t *>(start + offset);
            const auto instruction = BNM::Internal::DecodeX86(bytes, readable - offset,
#if defined(__x86_64__)
                true
#else
                false
#endif
            );
            if (!instruction.length || instruction.stop) return 0;
            if (instruction.call && --index == 0) {
                const auto opcode = start + offset + instruction.displacement - 1;
                DecodeBranchOrCall(ReadMemory<5>(opcode), opcode, target);
                return target;
            }
            offset += instruction.length;
        }
#endif
        return 0;
    }
}

void Internal::LateInit(void *il2cpp_class_from_il2cpp_type_addr) {
    if (!il2cpp_class_from_il2cpp_type_addr) return;

#if defined(__ARM_ARCH_7A__) || defined(__aarch64__)
    const uint8_t count = 1;
#elif defined(__i386__) || defined(__x86_64__)
    // x86 has one add-on call at start
    const uint8_t count = 2;
#endif

    //! il2cpp::vm::Class::FromIl2CppType
    // Путь (Path):
    // il2cpp_class_from_il2cpp_type ->
    // il2cpp::vm::Class::FromIl2CppType
    auto from_il2cpp_type = AssemblerUtils::FindNextJump(reinterpret_cast<BNM_PTR>(il2cpp_class_from_il2cpp_type_addr), count);

    Internal::BNM_Class$$FromIl2CppType_origin = ::BasicHook(from_il2cpp_type, Internal::BNM_Class$$FromIl2CppType, Internal::old_BNM_Class$$FromIl2CppType);
}

#ifdef BNM_DEBUG
static void *OffsetInLib(void *offsetInMemory) {
    if (offsetInMemory == nullptr) return nullptr;
    Dl_info info{};
    if (!BNM_dladdr(offsetInMemory, &info) || !info.dli_fbase) return nullptr;
    return reinterpret_cast<void *>(reinterpret_cast<BNM_PTR>(offsetInMemory) - reinterpret_cast<BNM_PTR>(info.dli_fbase));
}

void *Utils::OffsetInLib(void *offsetInMemory) {
    return ::OffsetInLib(offsetInMemory);
}
#endif

bool Internal::SetupBNM() {
#if defined(__ARM_ARCH_7A__) || defined(__aarch64__)
    const uint8_t count = 1;
#elif defined(__i386__) || defined(__x86_64__)
    // x86 has one add-on call at start
    const uint8_t count = 2;
#endif

    //! il2cpp::vm::Class::Init
    // Path:
    // il2cpp_array_new_specific ->
    // il2cpp::vm::Array::NewSpecific ->
    // il2cpp::vm::Class::Init
    il2cppMethods.Class$$Init = reinterpret_cast<decltype(il2cppMethods.Class$$Init)>(AssemblerUtils::FindNextJump(AssemblerUtils::FindNextJump(reinterpret_cast<BNM_PTR>(GetIl2CppMethod(BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_array_new_specific))), count), count));
    if (!il2cppMethods.Class$$Init) return false;
    BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_Class_Init, OffsetInLib(reinterpret_cast<void *>(il2cppMethods.Class$$Init)));


#define INIT_IL2CPP_API(name) il2cppMethods.name = reinterpret_cast<decltype(il2cppMethods.name)>(GetIl2CppMethod(BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_##name))); BNM_LOG_DEBUG("[INIT_IL2CPP_API]: " #name " (" BNM_IL2CPP_API_##name ") in lib %p", OffsetInLib(reinterpret_cast<void *>(il2cppMethods.name)))

    INIT_IL2CPP_API(il2cpp_image_get_class);
    INIT_IL2CPP_API(il2cpp_get_corlib);
    INIT_IL2CPP_API(il2cpp_class_from_name);
    INIT_IL2CPP_API(il2cpp_assembly_get_image);
    INIT_IL2CPP_API(il2cpp_method_get_param_name);
    INIT_IL2CPP_API(il2cpp_class_from_il2cpp_type);
    INIT_IL2CPP_API(il2cpp_array_class_get);
    INIT_IL2CPP_API(il2cpp_type_get_object);
    INIT_IL2CPP_API(il2cpp_object_new);
    INIT_IL2CPP_API(il2cpp_value_box);
    INIT_IL2CPP_API(il2cpp_array_new);
    INIT_IL2CPP_API(il2cpp_field_static_get_value);
    INIT_IL2CPP_API(il2cpp_field_static_set_value);
    INIT_IL2CPP_API(il2cpp_string_new);
    INIT_IL2CPP_API(il2cpp_resolve_icall);
    INIT_IL2CPP_API(il2cpp_runtime_invoke);
    INIT_IL2CPP_API(il2cpp_domain_get);
    INIT_IL2CPP_API(il2cpp_thread_current);
    INIT_IL2CPP_API(il2cpp_thread_attach);
    INIT_IL2CPP_API(il2cpp_thread_detach);
    INIT_IL2CPP_API(il2cpp_gc_alloc_fixed);
    INIT_IL2CPP_API(il2cpp_gc_free_fixed);
    INIT_IL2CPP_API(il2cpp_gc_wbarrier_set_field);

#undef INIT_IL2CPP_API
    if (!il2cppMethods.il2cpp_get_corlib || !il2cppMethods.il2cpp_class_from_name ||
        !il2cppMethods.il2cpp_assembly_get_image || !il2cppMethods.il2cpp_method_get_param_name ||
        !il2cppMethods.il2cpp_class_from_il2cpp_type || !il2cppMethods.il2cpp_array_class_get ||
        !il2cppMethods.il2cpp_type_get_object || !il2cppMethods.il2cpp_object_new ||
        !il2cppMethods.il2cpp_value_box || !il2cppMethods.il2cpp_array_new ||
        !il2cppMethods.il2cpp_field_static_get_value || !il2cppMethods.il2cpp_field_static_set_value ||
        !il2cppMethods.il2cpp_string_new || !il2cppMethods.il2cpp_resolve_icall ||
        !il2cppMethods.il2cpp_runtime_invoke || !il2cppMethods.il2cpp_domain_get ||
        !il2cppMethods.il2cpp_thread_current || !il2cppMethods.il2cpp_thread_attach ||
        !il2cppMethods.il2cpp_thread_detach || !il2cppMethods.il2cpp_gc_wbarrier_set_field) return false;
#ifdef BNM_USE_IL2CPP_ALLOCATOR
    if (!il2cppMethods.il2cpp_gc_alloc_fixed || !il2cppMethods.il2cpp_gc_free_fixed) return false;
#endif

    //! il2cpp::vm::Image::GetTypes
    if (il2cppMethods.il2cpp_image_get_class == nullptr) {
        auto assemblyClass = il2cppMethods.il2cpp_class_from_name(il2cppMethods.il2cpp_get_corlib(), BNM_OBFUSCATE_TMP("System.Reflection"), BNM_OBFUSCATE_TMP("Assembly"));
        BNM_PTR GetTypesAdr = Class(assemblyClass).GetMethod(BNM_OBFUSCATE_TMP("GetTypes"), 1).GetOffset();

#if UNITY_VER >= 211
        const int sCount = count;
#elif UNITY_VER > 174
        const int sCount = count + 1;
#else
        const int sCount = count + 2;
#endif
        // Path:
        // System.Reflection.Assembly.GetTypes(bool) ->
        // il2cpp::icalls::mscorlib::System::Reflection::Assembly::GetTypes ->
        // il2cpp::icalls::mscorlib::System::Module::InternalGetTypes ->
        // il2cpp::vm::Image::GetTypes
        il2cppMethods.orig_Image$$GetTypes = reinterpret_cast<decltype(il2cppMethods.orig_Image$$GetTypes)>(AssemblerUtils::FindNextJump(AssemblerUtils::FindNextJump(AssemblerUtils::FindNextJump(GetTypesAdr, count), sCount), count));

        if (!il2cppMethods.orig_Image$$GetTypes) return false;
        BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_Image_GetTypes, OffsetInLib(reinterpret_cast<void *>(il2cppMethods.orig_Image$$GetTypes)));
    } else BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_image_get_class_exists);

#ifdef BNM_CLASSES_MANAGEMENT

    //! il2cpp::vm::Class::FromIl2CppType
    // Path:
    // il2cpp_class_from_type ->
    // il2cpp::vm::Class::FromIl2CppType
    auto from_type_adr = AssemblerUtils::FindNextJump(reinterpret_cast<BNM_PTR>(GetIl2CppMethod(BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_class_from_type))), count);
    if (!from_type_adr) return false;
    BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_Class_FromIl2CppType, OffsetInLib(reinterpret_cast<void *>(from_type_adr)));


    //! il2cpp::vm::Type::GetClassOrElementClass
    // Path:
    // il2cpp_type_get_class_or_element_class ->
    // il2cpp::vm::Type::GetClassOrElementClass
    auto type_get_class_adr = AssemblerUtils::FindNextJump(reinterpret_cast<BNM_PTR>(GetIl2CppMethod(BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_type_get_class_or_element_class))), count);
    if (!type_get_class_adr) return false;
    BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_Type_GetClassOrElementClass, OffsetInLib(reinterpret_cast<void *>(type_get_class_adr)));

    //! il2cpp::vm::Image::ClassFromName
    // Path:
    // il2cpp_class_from_name ->
    // il2cpp::vm::Class::FromName ->
    // il2cpp::vm::Image::ClassFromName
    auto from_name_adr = AssemblerUtils::FindNextJump(AssemblerUtils::FindNextJump(reinterpret_cast<BNM_PTR>(il2cppMethods.il2cpp_class_from_name), count), count);
    if (!from_name_adr) return false;
    BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_Image_FromName, OffsetInLib(reinterpret_cast<void *>(from_name_adr)));
#if UNITY_VER <= 174

    //! il2cpp::vm::MetadataCache::GetImageFromIndex
    // Path:
    // il2cpp_assembly_get_image ->
    // il2cpp::vm::Assembly::GetImage ->
    // il2cpp::vm::MetadataCache::GetImageFromIndex
    auto GetImageFromIndexOffset = AssemblerUtils::FindNextJump(AssemblerUtils::FindNextJump(reinterpret_cast<BNM_PTR>(il2cppMethods.il2cpp_assembly_get_image), count), count);
    if (!GetImageFromIndexOffset) return false;
    BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_MetadataCache_GetImageFromIndex, OffsetInLib(reinterpret_cast<void *>(GetImageFromIndexOffset)));

    //! il2cpp::vm::Assembly::Load
    // Path:
    // il2cpp_domain_assembly_open ->
    // il2cpp::vm::Assembly::Load
    BNM_PTR AssemblyLoadOffset = AssemblerUtils::FindNextJump(reinterpret_cast<BNM_PTR>(GetIl2CppMethod(BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_domain_assembly_open))), count);
    if (!AssemblyLoadOffset) return false;
    BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_Assembly_Load, OffsetInLib(reinterpret_cast<void *>(AssemblyLoadOffset)));

#endif
#endif

    //! il2cpp::vm::Assembly::GetAllAssemblies
    // Path:
    // il2cpp_domain_get_assemblies ->
    // il2cpp::vm::Assembly::GetAllAssemblies
    auto adr = reinterpret_cast<BNM_PTR>(GetIl2CppMethod(BNM_OBFUSCATE_TMP(BNM_IL2CPP_API_il2cpp_domain_get_assemblies)));
    il2cppMethods.Assembly$$GetAllAssemblies = reinterpret_cast<std::vector<IL2CPP::Il2CppAssembly *> *(*)()>(AssemblerUtils::FindNextJump(adr, count));
    if (!il2cppMethods.Assembly$$GetAllAssemblies) return false;
    BNM_LOG_DEBUG(DBG_BNM_MSG_SetupBNM_Assembly_GetAllAssemblies, OffsetInLib(reinterpret_cast<void *>(il2cppMethods.Assembly$$GetAllAssemblies)));

    auto mscorlib = il2cppMethods.il2cpp_get_corlib();

    // Get MakeGenericMethod_impl. Depending on the version of Unity, it may be in different classes.
    auto runtimeMethodInfoClassPtr = TryGetClassInImage(mscorlib, BNM_OBFUSCATE_TMP("System.Reflection"), BNM_OBFUSCATE_TMP("RuntimeMethodInfo"));
    if (runtimeMethodInfoClassPtr) {
        Internal::il2cppMethods.Class$$Init(runtimeMethodInfoClassPtr);
        vmData.RuntimeMethodInfo$$MakeGenericMethod_impl = BNM::MethodBase(IterateMethods(runtimeMethodInfoClassPtr, [methodName = BNM_OBFUSCATE_TMP("MakeGenericMethod_impl")](const MethodBase &methodBase) {
            return !strcmp(methodBase._data->name, methodName);
        }));
    }
    if (!vmData.RuntimeMethodInfo$$MakeGenericMethod_impl.IsValid())
        vmData.RuntimeMethodInfo$$MakeGenericMethod_impl = Class(BNM_OBFUSCATE_TMP("System.Reflection"), BNM_OBFUSCATE_TMP("MonoMethod"), mscorlib).GetMethod(BNM_OBFUSCATE_TMP("MakeGenericMethod_impl"));

    auto runtimeTypeClass = Class(BNM_OBFUSCATE_TMP("System"), BNM_OBFUSCATE_TMP("RuntimeType"), mscorlib);
    auto stringClass = Class(BNM_OBFUSCATE_TMP("System"), BNM_OBFUSCATE_TMP("String"), mscorlib);
    auto interlockedClass = Class(BNM_OBFUSCATE_TMP("System.Threading"), BNM_OBFUSCATE_TMP("Interlocked"), mscorlib);
    auto objectClass = Class(BNM_OBFUSCATE_TMP("System"), BNM_OBFUSCATE_TMP("Object"), mscorlib);
    if (!objectClass || !stringClass || !runtimeTypeClass || !interlockedClass) return false;
    for (uint16_t slot = 0; slot < objectClass._data->vtable_count; slot++) {
        const BNM::IL2CPP::MethodInfo* vMethod = objectClass._data->vtable[slot].method;
        if (!vMethod) continue;
        if (strcmp(vMethod->name, BNM_OBFUSCATE_TMP("Finalize")) != 0) continue;
        finalizerSlot = slot;
        break;
    }

    auto UnityEngineCoreModule = Image(BNM_OBFUSCATE_TMP("UnityEngine.CoreModule.dll"));

    vmData.Object = objectClass;
    vmData.UnityEngine$$Object = Class(BNM_OBFUSCATE_TMP("UnityEngine"), BNM_OBFUSCATE_TMP("Object"), UnityEngineCoreModule);
    vmData.Type$$GetType = Class(BNM_OBFUSCATE_TMP("System"), BNM_OBFUSCATE_TMP("Type"), mscorlib).GetMethod(BNM_OBFUSCATE_TMP("GetType"), 1);
    vmData.Interlocked$$CompareExchange = interlockedClass.GetMethod(BNM_OBFUSCATE_TMP("CompareExchange"), {objectClass, objectClass, objectClass});
    vmData.RuntimeType$$MakeGenericType = runtimeTypeClass.GetMethod(BNM_OBFUSCATE_TMP("MakeGenericType"), 2);
    vmData.RuntimeType$$MakePointerType = runtimeTypeClass.GetMethod(BNM_OBFUSCATE_TMP("MakePointerType"), 1);
    vmData.RuntimeType$$make_byref_type = runtimeTypeClass.GetMethod(BNM_OBFUSCATE_TMP("make_byref_type"), 0);
    vmData.String$$Empty = stringClass.GetField(BNM_OBFUSCATE_TMP("Empty")).cast<Structures::Mono::String *>().GetPointer();

    auto listClass = vmData.System$$List = Class(BNM_OBFUSCATE_TMP("System.Collections.Generic"), BNM_OBFUSCATE_TMP("List`1"));
    if (!listClass) return false;
#ifdef BNM_CLASSES_MANAGEMENT
    // Install only after every address and template has been validated.
    std::array<void *, 5> installed{};
    size_t installedCount = 0;
    auto install = [&](auto address, auto replacement, auto &original) {
        auto *token = ::BasicHook(address, replacement, original);
        if (!token) return false;
        installed[installedCount++] = token;
        return true;
    };
    bool hooksReady = install(from_type_adr, ClassesManagement::Class$$FromIl2CppType, ClassesManagement::old_Class$$FromIl2CppType)
        && install(type_get_class_adr, ClassesManagement::Type$$GetClassOrElementClass, ClassesManagement::old_Type$$GetClassOrElementClass)
        && install(from_name_adr, ClassesManagement::Class$$FromName, ClassesManagement::old_Class$$FromName);
#if UNITY_VER <= 174
    hooksReady = hooksReady && install(GetImageFromIndexOffset, ClassesManagement::new_GetImageFromIndex, ClassesManagement::old_GetImageFromIndex);
    void *unusedOriginal{};
    hooksReady = hooksReady && install(AssemblyLoadOffset, ClassesManagement::Assembly$$Load, unusedOriginal);
#endif
    if (!hooksReady) {
        while (installedCount) Unhook(installed[--installedCount]);
        return false;
    }
#endif
    return true;
}

void Loading::AddOnLoadedEvent(void (*event)()) {
    std::lock_guard lock(Internal::eventsMutex);
    if (event) Internal::onIl2CppLoaded.Add(event);
}

void Loading::ClearOnLoadedEvents() {
    std::lock_guard lock(Internal::eventsMutex);
    Internal::onIl2CppLoaded.Clear();
}