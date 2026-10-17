// Regression tests of actual BNM implementations with synthetic runtime fixtures.
// Each case must exit successfully without ASan/UBSan reports.
#include <Internals.hpp>
#include <BNM/ComplexMonoStructures.hpp>
#include <BNM/Coroutine.hpp>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <BNM/Field.hpp>

using namespace BNM;
static std::atomic<bool> failNextAllocation{false};
extern "C" void *__real_malloc(size_t);
extern "C" void *__wrap_malloc(size_t size) {
    if (failNextAllocation.exchange(false)) return nullptr;
    return __real_malloc(size);
}
static IL2CPP::Il2CppClass fixtureClass{};
static IL2CPP::Il2CppImage fixtureImage{};
static IL2CPP::Il2CppAssembly fixtureAssembly{};
static IL2CPP::Il2CppObject yieldedObject{};
static std::vector<IL2CPP::Il2CppAssembly*> assemblies{&fixtureAssembly};

static void Setup()
{
    fixtureClass.name = "Fixture";
    fixtureClass.namespaze = "Audit";
    fixtureClass.image = &fixtureImage;
    fixtureImage.name = "Audit.dll";
    fixtureImage.nameNoExt = "Audit";
    fixtureImage.typeCount = 1;
    Internal::il2cppMethods.Assembly$$GetAllAssemblies = +[]() { return &assemblies; };
    Internal::il2cppMethods.il2cpp_assembly_get_image = +[](const IL2CPP::Il2CppAssembly*) { return &fixtureImage; };
    Internal::il2cppMethods.il2cpp_image_get_class = +[](const IL2CPP::Il2CppImage*, unsigned int) { return &fixtureClass; };
    Internal::il2cppMethods.Class$$Init = +[](IL2CPP::Il2CppClass*) {};
    Internal::il2cppMethods.il2cpp_gc_alloc_fixed = +[](size_t size) { return std::malloc(size); };
    Internal::il2cppMethods.il2cpp_gc_free_fixed = +[](void* ptr) { std::free(ptr); };
}

static Coroutine::IEnumerator YieldNull(int* progress)
{
    ++*progress;
    co_yield Coroutine::YieldInstruction(nullptr);
    ++*progress;
    co_yield Coroutine::YieldInstruction(&yieldedObject);
}

static std::atomic<bool> entered{false}, releaseInit{false}, secondEntered{false}, secondReturned{false};
static std::mutex runtimeInitMutex;
static bool javaException = false;
static int framesPushed = 0, framesPopped = 0, invalidJavaCalls = 0;
static int javaFailureStage = 0;
static int utfAcquired = 0, utfReleased = 0, splitRefsDeleted = 0;
static JNINativeInterface jniTable{};
static JNIEnv MakeJniFixture() {
    jniTable.ExceptionCheck = +[](JNIEnv*) -> jboolean { return javaException ? JNI_TRUE : JNI_FALSE; };
    jniTable.PushLocalFrame = +[](JNIEnv*, jint) -> jint { ++framesPushed; return 0; };
    jniTable.PopLocalFrame = +[](JNIEnv*, jobject) -> jobject { ++framesPopped; return nullptr; };
    jniTable.GetObjectClass = +[](JNIEnv*, jobject) -> jclass {
        if (javaFailureStage == 1) { javaException = true; return nullptr; }
        return reinterpret_cast<jclass>(uintptr_t{2});
    };
    jniTable.GetMethodID = +[](JNIEnv*, jclass, const char*, const char*) -> jmethodID {
        if (javaFailureStage == 2) { javaException = true; return nullptr; }
        return reinterpret_cast<jmethodID>(uintptr_t{3});
    };
    jniTable.CallObjectMethodV = +[](JNIEnv*, jobject, jmethodID method, va_list) -> jobject {
        if (!method || javaException) ++invalidJavaCalls;
        if (javaFailureStage == 3) javaException = true;
        return javaFailureStage >= 5 ? reinterpret_cast<jobject>(uintptr_t{4}) : nullptr;
    };
    jniTable.GetFieldID = +[](JNIEnv*, jclass, const char *name, const char*) -> jfieldID {
        if (javaFailureStage == 5) { javaException = true; return nullptr; }
        return reinterpret_cast<jfieldID>(uintptr_t{std::strcmp(name, "splitSourceDirs") == 0 ? 6u : 5u});
    };
    jniTable.GetIntField = +[](JNIEnv*, jobject, jfieldID) -> jint { return 0; };
    jniTable.GetObjectField = +[](JNIEnv*, jobject, jfieldID field) -> jobject { return reinterpret_cast<jobject>(field); };
    jniTable.GetStringUTFChars = +[](JNIEnv*, jstring, jboolean*) -> const char* { ++utfAcquired; return "/bnm-regression-nonexistent/apk"; };
    jniTable.ReleaseStringUTFChars = +[](JNIEnv*, jstring, const char*) { ++utfReleased; };
    jniTable.GetArrayLength = +[](JNIEnv*, jarray) -> jsize { return 3; };
    jniTable.GetObjectArrayElement = +[](JNIEnv*, jobjectArray, jsize) -> jobject { return reinterpret_cast<jobject>(uintptr_t{7}); };
    jniTable.DeleteLocalRef = +[](JNIEnv*, jobject) { ++splitRefsDeleted; };
    JNIEnv env{}; env.functions = &jniTable; return env;
}
static MANAGEMENT_STRUCTURES::CustomClass *enumeratorDescriptor{};
static IL2CPP::Il2CppClass enumeratorClass{};
static void SetupCoroutineFixture() {
    Internal::SetupCoroutine();
    enumeratorDescriptor = Internal::ClassesManagement::classesManagementVector->at(0);
    enumeratorClass.instance_size = static_cast<uint32_t>(enumeratorDescriptor->_size);
    enumeratorDescriptor->myClass = &enumeratorClass;
    Internal::il2cppMethods.il2cpp_object_new = +[](const IL2CPP::Il2CppClass *klass) {
        auto *object = static_cast<IL2CPP::Il2CppObject *>(std::calloc(1, klass->instance_size));
        if (object) object->klass = const_cast<IL2CPP::Il2CppClass *>(klass);
        return object;
    };
}
template<class Ret> Ret CoroutineCall(const char *name, Coroutine::IEnumerator *object) {
    for (auto *method : enumeratorDescriptor->_methods) {
        if (method->_name == name) return reinterpret_cast<Ret(*)(Coroutine::IEnumerator *)>(method->_address)(object);
    }
    std::abort();
}
static void DestroyCoroutine(Coroutine::IEnumerator *object) {
    CoroutineCall<void>("Finalize", object);
    // Managed finalizer must be idempotent.
    CoroutineCall<void>("Finalize", object);
    std::free(object);
}

int main(int argc, char** argv)
{
    if (argc != 2) return 64;
    Setup();
    const std::string mode(argv[1]);
    if (mode == "positive")
    {
        Structures::Mono::List<int> list{};
        list.items = Structures::Mono::Array<int>::Create(4, true);
        list.Add(7);
        list.Insert(0, 3);
        list.RemoveAt(1);
        const bool ok = list.GetSize() == 1 && list.get_Item(0) == 3 && Class("Audit", "Fixture").GetClass() == &fixtureClass;
        list.items->Destroy();
        std::printf("list and class lookup control=%d\n", ok);
        return ok ? 0 : 1;
    }
    else if (mode == "copy-lifetime")
    {
        auto original = CompileTimeClassBuilder("Audit", "Fixture").Build();
        auto copy = original;
        if (original.ToClass()._data != &fixtureClass || copy.ToClass()._data != &fixtureClass) return 1;
    }
    else if (mode == "const-generic")
    {
        const auto value = CompileTimeClassBuilder("Audit", "Fixture").Generic({CompileTimeClass{}}).Build();
        auto* info = static_cast<CompileTimeClass::_GenericInfo*>(value._stack.lastElement->value);
        const auto before = info->_types.size();
        (void)value.ToClass();
        std::printf("generic arguments before=%zu after=%zu\n", before, info->_types.size());
        return info->_types.size() == before ? 0 : 1;
    }
    else if (mode == "class-publication")
    {
        Internal::il2cppMethods.Class$$Init = +[](IL2CPP::Il2CppClass*) {
            if (entered.exchange(true)) secondEntered.store(true);
            std::lock_guard lock(runtimeInitMutex);
            while (!releaseInit.load()) std::this_thread::yield();
        };
        std::thread first([] { (void)Class(&fixtureClass).GetClass(); });
        while (!entered.load()) std::this_thread::yield();
        std::thread second([] { (void)Class(&fixtureClass).GetClass(); secondReturned.store(true); });
        // Wait for entry OR a premature cached return. A bounded wait prevents deadlock.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (!secondEntered.load() && !secondReturned.load() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::yield();
        const bool blockedCorrectly = secondEntered.load() && !secondReturned.load();
        releaseInit.store(true);
        first.join(); second.join();
        return blockedCorrectly && secondReturned.load() ? 0 : 1;
    }
    else if (mode == "dictionary-holes")
    {
        using Dict = Structures::Mono::Dictionary<int,int>;
        auto* entries = Structures::Mono::Array<Dict::Entry>::Create(2, true);
        entries->m_Items[0] = {1, -1, 11, 111};
        entries->m_Items[1] = {-1, -1, 0, 0}; // Removed entry in the represented layout.
        Dict dict{};
        dict.entries = entries;
        dict.count = 2;
        dict.freeCount = 1;
        auto keys = dict.GetKeys();
        std::printf("live=1 GetSize=%d GetKeys=%zu\n", dict.GetSize(), keys.size());
        entries->Destroy();
        return dict.GetSize() == 1 && keys.size() == 1 ? 0 : 1;
    }
    else if (mode == "array-overflow")
    {
        const size_t count = SIZE_MAX / sizeof(uint64_t) + 1;
        auto* array = Structures::Mono::Array<uint64_t>::Create(count, true);
        if (array) { array->Destroy(); return 1; }
        Internal::il2cppMethods.il2cpp_gc_alloc_fixed = +[](size_t) -> void* { return nullptr; };
        if (Structures::Mono::Array<int>::Create(4, true)) return 1;
        return 0;
    }
    else if (mode == "coroutine-yield")
    {
        SetupCoroutineFixture();
        int progress = 0;
        auto coroutine = YieldNull(&progress);
        auto *object = coroutine.Get();
        if (!object || object->klass != &enumeratorClass || progress != 0 || coroutine.Get() != object) return 1;
        if (!CoroutineCall<bool>("MoveNext", object) || progress != 1 || CoroutineCall<IL2CPP::Il2CppObject *>("get_Current", object)) return 1;
        if (!CoroutineCall<bool>("MoveNext", object) || progress != 2 || CoroutineCall<IL2CPP::Il2CppObject *>("get_Current", object) != &yieldedObject) return 1;
        if (CoroutineCall<bool>("MoveNext", object) || CoroutineCall<bool>("MoveNext", object)) return 1;
        DestroyCoroutine(object);
    }
    else if (mode == "coroutine-temporary") {
        SetupCoroutineFixture();
        int progress = 0;
        auto *object = YieldNull(&progress).Get();
        if (!object || object->klass != &enumeratorClass) return 1;
        if (!CoroutineCall<bool>("MoveNext", object) || progress != 1) return 1;
        CoroutineCall<void>("Reset", object);
        if (CoroutineCall<bool>("MoveNext", object)) return 1;
        DestroyCoroutine(object);
    }
    else if (mode == "missing-symbol")
    {
        Loading::SetMethodFinder(+[](const char*, void*) -> void* { return nullptr; }, nullptr);
        if (Internal::SetupBNM()) return 1;
        Loading::TrySetupByUsersFinder();
        if (IsLoaded() || Internal::isLoading.load()) return 1;
        Loading::SetMethodFinder(nullptr, nullptr);
        if (Loading::TryLoadByUsersFinder() || Loading::TryLoadByDlfcnHandle(nullptr)) return 1;
    }
    else if (mode == "custom-method-limit")
    {
        MANAGEMENT_STRUCTURES::CustomClass target{};
        target._targetType = CompileTimeClassBuilder("Audit", "Fixture").Build();
        MANAGEMENT_STRUCTURES::CustomMethod method{};
        method._name = "TooManyParameters";
        method._address = reinterpret_cast<void *>(+[]() {});
        method._invoker = method._address;
        method._parameterTypes.resize(256);
        target._methods.push_back(&method);
        ClassesManagement::ProcessClassRuntime(&target);
        return !method.myInfo && fixtureClass.method_count == 0 ? 0 : 1;
    }
    else if (mode == "mapped-not-readable")
    {
        const auto size = static_cast<size_t>(getpagesize());
        auto* memory = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (memory == MAP_FAILED) return 77;
        const bool accepted = IsAllocated(memory);
        std::printf("PROT_NONE mapping accepted=%d\n", accepted);
        munmap(memory, size);
        return accepted ? 1 : 0;
    }
    else if (mode == "hook-failure")
    {
        Loading::SetMethodFinder(+[](const char*, void*) -> void* { return reinterpret_cast<void*>(+[]() {}); }, nullptr);
        const bool result = Loading::TryLoadByUsersFinder();
        std::printf("stub hook fails; loader result=%d original=%p\n", result, reinterpret_cast<void*>(Internal::old_BNM_il2cpp_init));
        return !result ? 0 : 1;
    }
    else if (mode == "reflection") {
        IL2CPP::MethodInfo method{};
        method.name = "Sum";
        method.flags = 0x0010;
        method.parameters_count = 2;
        method.methodPointer = reinterpret_cast<IL2CPP::Il2CppMethodPointer>(+[](int a, int b, IL2CPP::MethodInfo*) { return a + b; });
        PRIVATE_INTERNAL::GetMethodClass(&method) = &fixtureClass;
        const IL2CPP::MethodInfo *methods[] = {&method};
        fixtureClass.methods = methods;
        fixtureClass.method_count = 1;
        auto base = Class(&fixtureClass).GetMethod("Sum", 2);
        auto typed = base.cast<int>();
        if (typed(7, 9) != 16 || typed(7) != 0 || Class(&fixtureClass).GetMethod("Absent", 0).IsValid()) return 1;
        struct Instance : IL2CPP::Il2CppObject { int value; } instance{};
        instance.klass = &fixtureClass;
        IL2CPP::Il2CppType type{};
        IL2CPP::FieldInfo field{};
        field.name = "value"; field.parent = &fixtureClass; field.type = &type;
        field.offset = sizeof(IL2CPP::Il2CppObject);
        fixtureClass.fields = &field; fixtureClass.field_count = 1;
        auto value = Class(&fixtureClass).GetField("value").cast<int>();
        value.SetInstance(&instance);
        value.Set(42);
        if (value.Get() != 42 || instance.value != 42) return 1;
        auto classes = Image(&fixtureImage).GetClasses();
        return classes.size() == 1 && classes[0]._data == &fixtureClass ? 0 : 1;
    }
    else if (mode == "copy-move") {
        auto first = CompileTimeClassBuilder("Audit", "Fixture", nullptr, false).Build();
        auto second = first;
        second = second;
        first.Free();
        auto moved = std::move(second);
        if (moved.ToClass()._data != &fixtureClass || !second._stack.IsEmpty()) return 1;
        auto assigned = CompileTimeClassBuilder("Other", "Unused").Build();
        assigned = moved;
        moved.Free();
        return assigned.ToClass()._data == &fixtureClass ? 0 : 1;
    }
    else if (mode == "list-boundaries") {
        Structures::Mono::List<int> list{};
        list.items = Structures::Mono::Array<int>::Create(1, true);
        list.Add(4);
        Internal::il2cppMethods.il2cpp_gc_alloc_fixed = +[](size_t) -> void* { return nullptr; };
        list.Add(8); list.Insert(0, 9);
        if (list.size != 1 || list.items->m_Items[0] != 4 || list.Resize(-1) || list.CopyFrom(nullptr, -1)) return 1;
        list.items->Destroy();
    }
    else if (mode == "layout-rejection") {
        fixtureClass.instance_size = sizeof(IL2CPP::Il2CppObject);
        MANAGEMENT_STRUCTURES::CustomClass target{};
        target._targetType = CompileTimeClassBuilder("Audit", "Fixture").Build();
        target._size = fixtureClass.instance_size + sizeof(int);
        MANAGEMENT_STRUCTURES::CustomField field{};
        field._size = sizeof(int); field.offset = fixtureClass.instance_size;
        target._fields.push_back(&field);
        ClassesManagement::ProcessClassRuntime(&target);
        return fixtureClass.instance_size == sizeof(IL2CPP::Il2CppObject) && fixtureClass.field_count == 0 ? 0 : 1;
    }
    else if (mode == "jni-errors") {
        if (Loading::TryLoadByJNI(nullptr, nullptr)) return 1;
        auto env = MakeJniFixture();
        auto context = reinterpret_cast<jobject>(uintptr_t{1});
        javaException = true;
        if (Loading::TryLoadByJNI(&env, context) || framesPushed != 0) return 1;
        for (int stage = 1; stage <= 6; ++stage) {
            javaException = false; javaFailureStage = stage;
            if (Loading::TryLoadByJNI(&env, context)) return 1;
            if (framesPushed != framesPopped || invalidJavaCalls) return 1;
            if ((stage <= 3 || stage == 5) && !javaException) return 1;
        }
        if (utfAcquired != 4 || utfReleased != 4 || splitRefsDeleted != 3) return 1;
    }
    else if (mode == "invoke-hook") {
        IL2CPP::MethodInfo method{};
        method.name = "Answer"; method.flags = 0x0010;
        PRIVATE_INTERNAL::GetMethodClass(&method) = &fixtureClass;
        method.methodPointer = reinterpret_cast<IL2CPP::Il2CppMethodPointer>(+[](IL2CPP::MethodInfo*) { return 7; });
        auto replacement = +[](IL2CPP::MethodInfo*) { return 9; };
        void *original{};
        if (!InvokeHookImpl(&method, reinterpret_cast<void *>(replacement), &original)) return 1;
        if (Method<int>(&method)() != 9 || reinterpret_cast<int(*)(IL2CPP::MethodInfo*)>(original)(&method) != 7) return 1;
        return InvokeHookImpl(nullptr, reinterpret_cast<void *>(replacement), &original) ? 1 : 0;
    }
    else if (mode == "custom-class") {
        fixtureClass.instance_size = sizeof(IL2CPP::Il2CppObject);
        fixtureClass.typeHierarchyDepth = 1;
        IL2CPP::Il2CppClass *hierarchy[] = {&fixtureClass};
        fixtureClass.typeHierarchy = hierarchy;
        Internal::vmData.Object = &fixtureClass;
        Internal::il2cppMethods.il2cpp_type_get_object = +[](const IL2CPP::Il2CppType*) -> IL2CPP::Il2CppObject* { return &yieldedObject; };
        MANAGEMENT_STRUCTURES::CustomClass target{};
        target._targetType = CompileTimeClassBuilder("Audit", "NewClass", "Audit.dll").Build();
        target._size = sizeof(IL2CPP::Il2CppObject) + sizeof(int);
        MANAGEMENT_STRUCTURES::CustomField field{};
        field._name = "value"; field._size = sizeof(int); field.offset = sizeof(IL2CPP::Il2CppObject);
        field._type = Class(&fixtureClass).GetCompileTimeClass();
        target._fields.push_back(&field);
        MANAGEMENT_STRUCTURES::CustomMethod method{};
        method._name = "Answer"; method._isStatic = true;
        method._returnType = Class(&fixtureClass).GetCompileTimeClass();
        method._address = reinterpret_cast<void *>(+[](IL2CPP::MethodInfo*) { return 17; });
        method._invoker = method._address;
        target._methods.push_back(&method);
        ClassesManagement::ProcessClassRuntime(&target);
        if (!target.myClass || target.myClass->field_count != 1 || target.myClass->method_count != 1) return 1;
        if (target.myClass->fields[0].offset != sizeof(IL2CPP::Il2CppObject)) return 1;
        return Class(target.myClass).GetMethod("Answer", 0).cast<int>()() == 17 ? 0 : 1;
    }
    else if (mode == "metadata-oom") {
        MANAGEMENT_STRUCTURES::CustomClass target{};
        target._targetType = CompileTimeClassBuilder("Audit", "Fixture").Build();
        MANAGEMENT_STRUCTURES::CustomMethod method{};
        method._name = "AllocationFailure";
        method._address = reinterpret_cast<void *>(+[]() {});
        method._invoker = method._address;
        target._methods.push_back(&method);
        failNextAllocation.store(true);
        ClassesManagement::ProcessClassRuntime(&target);
        return !failNextAllocation.load() && !target.myClass && !method.myInfo && fixtureClass.method_count == 0 ? 0 : 1;
    }
    else return 64;
}
