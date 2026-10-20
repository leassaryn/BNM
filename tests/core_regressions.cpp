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
#include <memory>
#include <BNM/Field.hpp>

using namespace BNM;
static std::atomic<bool> failNextAllocation{false};
static int failAllocationAfter = -1;
static bool allocationFailed = false;
extern "C" void *__real_malloc(size_t);
extern "C" void *__wrap_malloc(size_t size) {
    if (failNextAllocation.exchange(false)) return nullptr;
    if (failAllocationAfter >= 0 && failAllocationAfter-- == 0) {
        allocationFailed = true;
        return nullptr;
    }
    return __real_malloc(size);
}
static IL2CPP::Il2CppClass fixtureClass{};
static IL2CPP::Il2CppImage fixtureImage{};
static IL2CPP::Il2CppAssembly fixtureAssembly{};
static IL2CPP::Il2CppObject yieldedObject{};
static std::vector<IL2CPP::Il2CppAssembly*> assemblies{&fixtureAssembly};

static size_t barrierCalls{};
static IL2CPP::Il2CppObject *lastBarrierOwner{};
static void *lastBarrierSlot{};
static void BarrierFixture(IL2CPP::Il2CppObject *owner, void **slot, void *value) {
    ++barrierCalls; lastBarrierOwner = owner; lastBarrierSlot = slot;
    std::memcpy(slot, &value, sizeof(value));
}

static void Setup()
{
    Internal::il2cppMethods.il2cpp_gc_wbarrier_set_field = BarrierFixture;
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

static IL2CPP::Il2CppThread attachedThread{};
static IL2CPP::Il2CppDomain threadDomain{};
static bool attached{}, attachFails{};
static int attachCalls{}, detachCalls{};
static int ThreadScopeRegression() {
    auto &api = Internal::il2cppMethods;
    api.il2cpp_domain_get = +[] { return &threadDomain; };
    api.il2cpp_thread_current = +[](IL2CPP::Il2CppDomain *) { return attached ? &attachedThread : nullptr; };
    api.il2cpp_thread_attach = +[](IL2CPP::Il2CppDomain *) -> IL2CPP::Il2CppThread * {
        ++attachCalls; attached = !attachFails; return attached ? &attachedThread : nullptr;
    };
    api.il2cpp_thread_detach = +[](IL2CPP::Il2CppThread *thread) {
        if (thread != &attachedThread) std::abort();
        ++detachCalls; attached = false;
    };
    static_assert(!std::is_copy_constructible_v<ScopedIl2CppThread>);
    static_assert(!std::is_move_constructible_v<ScopedIl2CppThread>);
    {
        ScopedIl2CppThread outer;
        if (!outer || !outer.OwnsAttachment() || outer.Get() != &attachedThread) return 1;
        { ScopedIl2CppThread inner; if (!inner || inner.OwnsAttachment()) return 1; }
        if (detachCalls || attachCalls != 1) return 1;
    }
    if (attached || detachCalls != 1) return 1;
    attached = true;
    { ScopedIl2CppThread borrowed; if (!borrowed || borrowed.OwnsAttachment()) return 1; }
    if (!attached || detachCalls != 1 || attachCalls != 1) return 1;
    attached = false;
    try { ScopedIl2CppThread guard; throw std::runtime_error("unwind"); }
    catch (const std::runtime_error &) {}
    if (attached || detachCalls != 2) return 1;
    attachFails = true;
    { ScopedIl2CppThread failed; if (failed) return 1; }
    if (detachCalls != 2) return 1;
    api.il2cpp_thread_attach = nullptr;
    { ScopedIl2CppThread unavailable; if (unavailable) return 1; }
    api.il2cpp_domain_get = +[]() -> IL2CPP::Il2CppDomain * { return nullptr; };
    { ScopedIl2CppThread noDomain; if (noDomain) return 1; }
    return 0;
}

static int BarrierRegression() {
    using namespace Structures::Mono;
    auto *array = Array<IL2CPP::Il2CppObject *>::Create(6, true);
    if (!array) return 1;
    IL2CPP::Il2CppObject a{}, b{}, c{};
    IL2CPP::Il2CppObject *values[] = {&a, &b, &c};
    if (!array->CopyRange(values, 3) || barrierCalls != 3 || lastBarrierOwner != array) return 1;
    if (!array->CopyRange(array->m_Items, 3, 1)) return 1;
    if (array->m_Items[1] != &a || array->m_Items[2] != &b || array->m_Items[3] != &c) return 1;
    (*array)[0] = &c;
    if (array->m_Items[0] != &c || lastBarrierSlot != &array->m_Items[0]) return 1;
    List<IL2CPP::Il2CppObject *> list{};
    list.items = array; list.size = 4;
    list.Insert(1, &b);
    if (list.size != 5 || list.get_Item(1) != &b || list.get_Item(2) != &a) return 1;
    list.RemoveAt(1);
    if (list.size != 4 || list.get_Item(1) != &a || list.items->m_Items[4]) return 1;
    if (!list.Resize(10) || lastBarrierOwner != &list || lastBarrierSlot != &list.items) return 1;
    if (!list.CopyFrom(values, 1) || list.size != 1 || list.items->m_Items[1]) return 1;
    list.Clear();
    if (list.size || list.items->m_Items[0]) return 1;
    auto barrier = Internal::il2cppMethods.il2cpp_gc_wbarrier_set_field;
    Internal::il2cppMethods.il2cpp_gc_wbarrier_set_field = nullptr;
    if (list.items->Set(0, &a) || list.items->m_Items[0]) return 1;
    Internal::il2cppMethods.il2cpp_gc_wbarrier_set_field = barrier;
    list.items->Destroy();

    // A managed backing array must grow through the runtime's allocator.
    static IL2CPP::Il2CppClass arrayClass{};
    static int managedAllocations{};
    arrayClass.element_class = &fixtureClass;
    Internal::il2cppMethods.il2cpp_array_new = +[](const IL2CPP::Il2CppClass *element,
        IL2CPP::il2cpp_array_size_t capacity) -> IL2CPP::Il2CppArray * {
        if (element != &fixtureClass) return nullptr;
        ++managedAllocations;
        auto *result = static_cast<Array<IL2CPP::Il2CppObject *> *>(
            std::calloc(1, sizeof(Array<IL2CPP::Il2CppObject *>) + capacity * sizeof(void *)));
        if (result) { result->klass = &arrayClass; result->capacity = capacity; }
        return reinterpret_cast<IL2CPP::Il2CppArray *>(result);
    };
    auto *oldManaged = static_cast<Array<IL2CPP::Il2CppObject *> *>(
        __Internal_Array::ArrayFromClass(&fixtureClass, 1));
    if (!oldManaged) return 1;
    list.items = oldManaged; list.size = 0;
    list.Add(&a); list.Add(&b);
    if (managedAllocations != 2 || list.items->klass != &arrayClass || list.size != 2 ||
        list.get_Item(0) != &a || list.get_Item(1) != &b) return 1;
    std::free(oldManaged); // Fixture owns the mock GC allocations.
    std::free(list.items);

    // Value-type copies must also mark embedded reference fields.
    struct Value { int number; IL2CPP::Il2CppObject *reference; };
    IL2CPP::Il2CppClass valueClass{};
    valueClass.has_references = 1;
    IL2CPP::Il2CppType referenceType{};
    referenceType.type = IL2CPP::IL2CPP_TYPE_OBJECT;
    IL2CPP::FieldInfo referenceField{};
    referenceField.type = &referenceType;
    referenceField.offset = static_cast<int32_t>(sizeof(IL2CPP::Il2CppObject) + offsetof(Value, reference));
    valueClass.field_count = 1; valueClass.fields = &referenceField;
    Value source{42, &b}, destination{};
    auto before = barrierCalls;
    if (!StoreManagedValue(&a, &destination, source, &valueClass) || barrierCalls != before + 1 ||
        destination.number != 42 || destination.reference != &b || lastBarrierSlot != &destination.reference) return 1;

    struct Outer { Value nested; } outerSource{{17, &c}}, outerDestination{};
    IL2CPP::Il2CppClass outerClass{};
    outerClass.has_references = 1;
    IL2CPP::Il2CppType nestedType{};
    nestedType.type = IL2CPP::IL2CPP_TYPE_VALUETYPE;
    nestedType.data.dummy = &valueClass;
    valueClass.byval_arg.valuetype = 1;
    IL2CPP::FieldInfo nestedField{};
    nestedField.type = &nestedType;
    nestedField.offset = sizeof(IL2CPP::Il2CppObject);
    outerClass.fields = &nestedField; outerClass.field_count = 1;
    Internal::il2cppMethods.il2cpp_class_from_il2cpp_type = +[](const IL2CPP::Il2CppType *type) {
        return static_cast<IL2CPP::Il2CppClass *>(type->data.dummy);
    };
    before = barrierCalls;
    if (!StoreManagedValue(&a, &outerDestination, outerSource, &outerClass) ||
        barrierCalls != before + 1 || outerDestination.nested.reference != &c ||
        lastBarrierSlot != &outerDestination.nested.reference) return 1;

    // Instance fields go through the barrier; static fields go through the VM setter.
    struct Object { IL2CPP::Il2CppObject header; IL2CPP::Il2CppObject *reference; } object{};
    object.header.klass = &fixtureClass;
    referenceField.offset = offsetof(Object, reference);
    referenceField.parent = &fixtureClass;
    Field<IL2CPP::Il2CppObject *> field(&referenceField);
    field[&object.header].Set(&c);
    if (object.reference != &c || lastBarrierOwner != &object.header) return 1;
    referenceType.attrs = 0x0010;
    static void *staticValue{};
    Internal::il2cppMethods.il2cpp_field_static_set_value = +[](const IL2CPP::FieldInfo *, void *value) {
        std::memcpy(&staticValue, value, sizeof(staticValue));
    };
    Field<IL2CPP::Il2CppObject *> staticField(&referenceField);
    staticField.Set(&b);
    return staticValue == &b ? 0 : 1;
}

static int MetadataHookFailureRegression() {
    using namespace MANAGEMENT_STRUCTURES;
    IL2CPP::MethodInfo first{}, second{};
    const auto original = +[]() {};
    first.name = "First"; second.name = "Second";
    first.methodPointer = second.methodPointer = original;
    PRIVATE_INTERNAL::GetMethodClass(&first) = PRIVATE_INTERNAL::GetMethodClass(&second) = &fixtureClass;
    const IL2CPP::MethodInfo *methods[] = {&first, &second};
    fixtureClass.methods = methods; fixtureClass.method_count = 2;
    CustomClass target{};
    target._targetType = CompileTimeClassBuilder("Audit", "Fixture", "Audit.dll").Build();
    CustomMethod invoke{}, basic{};
    invoke._name = "First"; invoke._isInvokeHook = true;
    basic._name = "Second"; basic._isBasicHook = true;
    invoke._address = basic._address = reinterpret_cast<void *>(+[]() {});
    invoke._invoker = basic._invoker = invoke._address;
    target._methods = {&invoke, &basic};
    // The fixture backend deliberately fails; the preceding invoke hook must be undone.
    ClassesManagement::ProcessClassRuntime(&target);
    return !target.myClass && !invoke.myInfo && !invoke._origin && !basic.myInfo &&
           first.methodPointer == original && second.methodPointer == original &&
           fixtureClass.methods == methods && fixtureClass.method_count == 2 ? 0 : 1;
}

static int MetadataRollbackRegression(bool modify) {
    using namespace MANAGEMENT_STRUCTURES;
    fixtureClass.instance_size = sizeof(IL2CPP::Il2CppObject);
    fixtureClass.typeHierarchyDepth = 1;
    IL2CPP::Il2CppClass *hierarchy[] = {&fixtureClass};
    fixtureClass.typeHierarchy = hierarchy;
    Internal::vmData.Object = &fixtureClass;
    Internal::il2cppMethods.il2cpp_type_get_object = +[](const IL2CPP::Il2CppType *) -> IL2CPP::Il2CppObject * { return &yieldedObject; };
    CustomClass target{};
    target._targetType = CompileTimeClassBuilder("Audit", modify ? "Fixture" : "Rollback", modify ? "Audit.dll" : "Rollback.dll").Build();
    target._size = sizeof(IL2CPP::Il2CppObject);
    IL2CPP::Il2CppClass owner{};
    owner.name = "Owner"; owner.namespaze = "Audit"; owner.image = &fixtureImage;
    target._owner = Class(&owner).GetCompileTimeClass();
    CustomMethod method{}, added{};
    method._name = "Existing"; method._isInvokeHook = true;
    method._address = reinterpret_cast<void *>(+[]() {}); method._invoker = method._address;
    method._returnType = Class(&fixtureClass).GetCompileTimeClass();
    added._name = "Added"; added._address = method._address; added._invoker = method._invoker;
    added._returnType = Class(&fixtureClass).GetCompileTimeClass();
    added._parameterTypes = {Class(&fixtureClass).GetCompileTimeClass()};
    target._methods = {&method, &added};
    IL2CPP::MethodInfo original{};
    original.name = "Existing";
    const auto oldPointer = +[]() {};
    original.methodPointer = oldPointer;
    PRIVATE_INTERNAL::GetMethodClass(&original) = &fixtureClass;
    const IL2CPP::MethodInfo *oldMethods[] = {&original};
    if (modify) { fixtureClass.methods = oldMethods; fixtureClass.method_count = 1; }
    int failures = 0;
    for (int allocation = 0; allocation < 200; ++allocation) {
        allocationFailed = false;
        failAllocationAfter = allocation;
        ClassesManagement::ProcessClassRuntime(&target);
        failAllocationAfter = -1;
        if (target.myClass) {
            if (allocationFailed || owner.nested_type_count != 1 || !method.myInfo || !added.myInfo) return 1;
            if (modify && (original.methodPointer != reinterpret_cast<IL2CPP::Il2CppMethodPointer>(method._address) || fixtureClass.method_count != 2)) return 1;
            std::printf("%s rollback: %d injected allocation failures recovered before commit\n", modify ? "modify" : "create", failures);
            return failures >= (modify ? 3 : 8) ? 0 : 1;
        }
        if (!allocationFailed) return 1;
        ++failures;
        if (method.myInfo || method._origin || method._originalAddress || added.myInfo ||
            target.type || owner.nestedTypes || owner.nested_type_count || assemblies.size() != 1 ||
            target._methods.size() != 2 || added._parameterTypes.size() != 1) return 1;
        if (modify && (original.methodPointer != oldPointer || fixtureClass.method_count != 1 ||
                       fixtureClass.methods != oldMethods || fixtureClass.declaringType)) return 1;
        bool published = false;
        Internal::ClassesManagement::bnmClassesMap.ForEach([&](auto *, const auto &classes) {
            published = published || !classes.empty(); return false;
        });
        if (published) return 1;
    }
    return 1;
}

static int VirtualDispatchRegression()
{
    // Il2CppClass ends with a variable-length vtable. Own the entire allocation.
    std::unique_ptr<IL2CPP::Il2CppClass, decltype(&std::free)> storage(
        static_cast<IL2CPP::Il2CppClass *>(std::calloc(
            1, sizeof(IL2CPP::Il2CppClass) + 3 * sizeof(IL2CPP::VirtualInvokeData))), &std::free);
    if (!storage) return 1;
    auto *derived = storage.get();
    derived->name = "Derived";
    derived->namespaze = "Audit";
    derived->parent = &fixtureClass;
    derived->vtable_count = 3;
    IL2CPP::MethodInfo base{}, hidden{}, overridden{}, interfaceMethod{}, implementation{};
    base.name = hidden.name = overridden.name = interfaceMethod.name = "Run";
    base.flags = hidden.flags = overridden.flags = interfaceMethod.flags = implementation.flags = 0x0040;
    base.slot = overridden.slot = 1;
    hidden.slot = interfaceMethod.slot = 0;
    implementation.slot = 2;
    implementation.name = "Audit.IRunnable.Run";
    PRIVATE_INTERNAL::GetMethodClass(&base) = &fixtureClass;
    PRIVATE_INTERNAL::GetMethodClass(&hidden) = derived;
    PRIVATE_INTERNAL::GetMethodClass(&overridden) = derived;
    PRIVATE_INTERNAL::GetMethodClass(&implementation) = derived;
    IL2CPP::Il2CppClass interfaceClass{};
    interfaceClass.flags = 0x0020;
    PRIVATE_INTERNAL::GetMethodClass(&interfaceMethod) = &interfaceClass;
    IL2CPP::Il2CppRuntimeInterfaceOffsetPair offset{&interfaceClass, 2};
    derived->interfaceOffsets = &offset;
    derived->interface_offsets_count = 1;
    const auto original = +[]() {};
    const auto replacement = +[]() {};
    derived->vtable[0] = {original, &hidden};
    derived->vtable[1] = {original, &overridden};
    derived->vtable[2] = {original, &implementation};
    IL2CPP::Il2CppObject instance{};
    instance.klass = derived;
    MethodBase method(&base);
    method.SetInstance(&instance);
    auto resolved = method.GetOverride();
    if (resolved.GetInfo() != &overridden || resolved._instance != &instance) return 1;
    MethodBase interfaceWrapper(&interfaceMethod);
    interfaceWrapper.SetInstance(&instance);
    if (interfaceWrapper.GetOverride().GetInfo() != &implementation) return 1;
    void *old{};
    if (!VirtualHookImpl(Class(derived), &base, reinterpret_cast<void *>(replacement), &old) ||
        old != reinterpret_cast<void *>(original)) return 1;
    if (derived->vtable[0].methodPtr != original || derived->vtable[1].methodPtr != replacement ||
        derived->vtable[2].methodPtr != original) return 1;
    if (!VirtualHookImpl(Class(derived), &interfaceMethod, reinterpret_cast<void *>(replacement), nullptr) ||
        derived->vtable[2].methodPtr != replacement) return 1;

    // Every rejected request must leave the original-output pointer untouched.
    const auto rejected = [&](IL2CPP::MethodInfo *info, void *hook) {
        old = reinterpret_cast<void *>(original);
        return !VirtualHookImpl(Class(derived), info, hook, &old) &&
               old == reinterpret_cast<void *>(original);
    };
    const auto hook = reinterpret_cast<void *>(replacement);
    if (!rejected(nullptr, hook) || !rejected(&base, nullptr)) return 1;
    base.slot = 65535;
    if (method.GetOverride().IsValid() || !rejected(&base, hook)) return 1;
    base.slot = 3;
    if (method.GetOverride().IsValid() || !rejected(&base, hook)) return 1;
    base.slot = 1;
    base.flags = 0x0010;
    if (!rejected(&base, hook)) return 1;
    base.flags = 0;
    if (!rejected(&base, hook)) return 1;
    base.flags = 0x0040;
    IL2CPP::Il2CppClass unrelated{};
    PRIVATE_INTERNAL::GetMethodClass(&base) = &unrelated;
    if (method.GetOverride().IsValid() || !rejected(&base, hook)) return 1;
    PRIVATE_INTERNAL::GetMethodClass(&base) = &fixtureClass;
    derived->vtable[1].method = nullptr;
    if (method.GetOverride().IsValid() || !rejected(&base, hook)) return 1;
    derived->vtable[1].method = &overridden;
    derived->vtable[1].methodPtr = nullptr;
    if (!rejected(&base, hook)) return 1;
    offset.offset = -1;
    if (interfaceWrapper.GetOverride().IsValid() || !rejected(&interfaceMethod, hook)) return 1;
    offset.offset = INT32_MAX;
    if (interfaceWrapper.GetOverride().IsValid() || !rejected(&interfaceMethod, hook)) return 1;
    derived->interfaceOffsets = nullptr;
    if (interfaceWrapper.GetOverride().IsValid() || !rejected(&interfaceMethod, hook)) return 1;
    method.SetInstance(nullptr);
    return method._instance == nullptr ? 0 : 1;
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

#include "review_regressions.hpp"
#include "gc_handle_regressions.hpp"

int main(int argc, char** argv)
{
    if (argc != 2) return 64;
    Setup();
    const std::string mode(argv[1]);
    if (mode == "list-growth-limit") return ListGrowthLimitRegression();
    if (mode == "proxy-assignment") return ProxyAssignmentRegression();
    if (mode == "field-rebind") return FieldRebindRegression();
    if (mode == "math-review") return MathReviewRegression();
    if (mode == "raycast-resolver") return RaycastResolverRegression();
    if (mode == "event-dirty") return EventDirtyRegression();
    if (mode == "delegate-mutation") return DelegateMutationRegression();
    if (mode == "string-view-logging") return StringViewLoggingRegression();
    if (mode == "x86-decoder") return DecoderRegression();
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
    else if (mode == "runtime-list") return RuntimeListRegression();
    else if (mode == "metadata-hook-rollback") return MetadataHookFailureRegression();
    else if (mode == "thread-scope") return ThreadScopeRegression();
    else if (mode == "gc-handle") return GCHandleRegression();
    else if (mode == "write-barriers") return BarrierRegression();
    else if (mode == "metadata-rollback-create") return MetadataRollbackRegression(false);
    else if (mode == "metadata-rollback-modify") return MetadataRollbackRegression(true);
    else if (mode == "virtual-dispatch") return VirtualDispatchRegression();
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
