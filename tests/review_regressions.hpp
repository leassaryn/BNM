// Included by core_regressions.cpp after its synthetic runtime fixtures.
#include <BNM/UnityStructures.hpp>
#include <X86Decoder.hpp>

static int ProxyAssignmentRegression() {
    using namespace Structures::Mono;
    auto *a = Array<int>::Create(2, true);
    a->m_Items[0] = 1; a->m_Items[1] = 2;
    (*a)[0] = (*a)[1];
    List<int> list{}; list.items = a; list.size = 2;
    list[1] = list[0];
    Utils::DataIterator<int> invalid;
    (*a)[0] = invalid;
    bool ok = a->m_Items[0] == 2 && a->m_Items[1] == 2;
    a->Destroy();
    auto *refs = Array<IL2CPP::Il2CppObject *>::Create(2, true);
    IL2CPP::Il2CppObject object{}; refs->m_Items[1] = &object;
    const auto barriers = barrierCalls;
    (*refs)[0] = (*refs)[1];
    ok = ok && refs->m_Items[0] == &object && barrierCalls == barriers + 1;
    refs->Destroy(); return ok ? 0 : 1;
}
static int FieldRebindRegression() {
    struct Object { IL2CPP::Il2CppObject base; int value; } object{{&fixtureClass, nullptr}, 42};
    IL2CPP::Il2CppType type{}; type.type = IL2CPP::IL2CPP_TYPE_I4;
    IL2CPP::FieldInfo info{}; info.name = "value"; info.type = &type;
    info.parent = &fixtureClass; info.offset = sizeof(IL2CPP::Il2CppObject);
    Field<int> field(&info); field.SetInstance(&object.base); field.SetInstance(nullptr); field.Set(99);
    if (object.value != 42 || field._instance) return 1;
#ifdef BNM_CHECK_INSTANCE_TYPE
    IL2CPP::Il2CppClass unrelated{}; unrelated.image = &fixtureImage; unrelated.name = "Other"; unrelated.namespaze = "Audit";
    IL2CPP::Il2CppObject other{&unrelated, nullptr};
    field.SetInstance(&object.base); field.SetInstance(&other); field.Set(99);
    if (object.value != 42 || field._instance) return 1;
#endif
    field.SetInstance(&object.base); field.Set(7); return object.value == 7 ? 0 : 1;
}
static int MathReviewRegression() {
    using namespace Structures::Unity;
    Matrix3x3 a(1,2,3,4,5,6,7,8,9); auto expected = a*a; a *= a;
    for (int i=0; i<9; ++i) if (a[i] != expected[i]) return 1;
    Quaternion q{0,0,std::sin(0.4f),std::cos(0.4f)};
    for (auto scale : {Vector3{2,3,4}, Vector3{-2,3,4}, Vector3{1,1,1}}) {
        Matrix4x4 m; m.SetTRS({3,4,5},q,scale); auto r=m.rotation();
        if (std::fabs(r.z-q.z)+std::fabs(r.w-q.w)>1e-5f) return 1;
    }
    Matrix4x4 singular; singular.SetScale({0,1,1});
    if (singular.rotation().w != 1) return 1;
    if (!std::isfinite(Vector3::NormalizeAngle(std::numeric_limits<float>::max()))) return 1;
    if (!std::isnan(Vector3::NormalizeAngle(std::numeric_limits<float>::infinity()))) return 1;
    for (float angle : {-720.f,-360.f,-180.f,0.f,180.f,360.f,720.f}) {
        auto r=Vector3::NormalizeAngle(angle); if (r < -180 || r > 180) return 1;
        r=Vector3::NormalizeAngle(angle,false); if (r < 0 || r > 360) return 1;
    }
    if (Vector3::NormalizeAngle(-180.f) != -180.f || Vector3::NormalizeAngle(180.f) != 180.f ||
        Vector3::NormalizeAngle(360.f, false) != 360.f) return 1;
    return 0;
}
static int RaycastResolverRegression() {
    Internal::il2cppMethods.il2cpp_resolve_icall = +[](const char *) -> void * { return nullptr; };
    Structures::Unity::RaycastHit a{}; a.m_Collider=1;
    Structures::Unity::RaycastHit2D b{}; b.m_Collider=1;
    if (a.GetCollider() || b.GetCollider()) return 1;
    Internal::il2cppMethods.il2cpp_resolve_icall = +[](const char *) -> void * {
        return reinterpret_cast<void *>(+[](int) -> void * { return nullptr; });
    };
    if (a.GetCollider() || b.GetCollider()) return 1;
#if UNITY_VER >= 232
    if (UnmarshalUnityObject(0) || UnmarshalUnityObject(1)) return 1;
#endif
    return 0;
}
static int EventDirtyRegression() {
    static int counter{};
    IL2CPP::MethodInfo method{}; method.flags=0x10; method.slot=65535;
    method.methodPointer=reinterpret_cast<IL2CPP::Il2CppMethodPointer>(+[](IL2CPP::MethodInfo*){++counter;});
    UnityEngine::UnityAction<> action{}; action.delegate.method=&method;
    UnityEngine::InvokableCallBase call{}; call.action=&action;
    Structures::Mono::List<UnityEngine::InvokableCallBase*> list{};
    list.items=Structures::Mono::Array<UnityEngine::InvokableCallBase*>::Create(1,true);
    list.items->m_Items[0]=&call; list.size=1;
    UnityEngine::InvokableCallList calls{}; calls.m_RuntimeCalls=&list;
    UnityEngine::UnityEvent<> event{}; event.m_Calls=&calls;
    // The dirty path must ignore stale cached persistent calls, but keep runtime calls.
    calls.m_PersistentCalls=&list;
    event.Invoke(); event.Invoke();
    if (counter != 2 || !event.m_CallsDirty) return 1;
    event.m_CallsDirty=false; event.Invoke();
    bool ok=counter==4;
    list.size=0; event.Invoke(); ok=ok && counter==4;
    list.items->Destroy(); return ok ? 0 : 1;
}
static bool delegateAllocationFails{};
static int DelegateMutationRegression() {
    using namespace Structures::Mono;
    static IL2CPP::Il2CppClass delegateClass{};
    delegateClass.instance_size=sizeof(MulticastDelegateBase);
    Internal::il2cppMethods.il2cpp_array_new=+[](const IL2CPP::Il2CppClass*, IL2CPP::il2cpp_array_size_t n)->IL2CPP::Il2CppArray* {
        return delegateAllocationFails ? nullptr : reinterpret_cast<IL2CPP::Il2CppArray*>(Array<DelegateBase*>::Create(n,true));
    };
    Internal::il2cppMethods.il2cpp_object_new=+[](const IL2CPP::Il2CppClass* cls)->IL2CPP::Il2CppObject* {
        auto *obj=static_cast<IL2CPP::Il2CppObject*>(std::calloc(1,cls->instance_size));
        if (obj) obj->klass=const_cast<IL2CPP::Il2CppClass*>(cls); return obj;
    };
    IL2CPP::MethodInfo first{},second{}; first.flags=second.flags=0x10;
    MulticastDelegateBase event{}; event.delegate.method=&first; event.delegate.object.klass=&delegateClass;
    DelegateBase added{}; added.method=&second;
    delegateAllocationFails=true;
    if (event.TryAdd(&added) || event.delegates) return 1;
    delegateAllocationFails=false;
    if (!event.TryAdd(&added) || event.GetMethods().size()!=2) return 1;
    auto *old=reinterpret_cast<Array<DelegateBase*>*>(event.delegates);
    auto *clone=old->m_Items[0];
    if (clone==reinterpret_cast<DelegateBase*>(&event) || clone->method!=&first) return 1;
    event.Remove(&added); old->Destroy();
    if (event.GetMethods().size()!=1) return 1;
    old=reinterpret_cast<Array<DelegateBase*>*>(event.delegates);
    event.Remove(clone); old->Destroy();
    if (!event.GetMethods().empty()) return 1;
    reinterpret_cast<Array<DelegateBase*>*>(event.delegates)->Destroy(); std::free(clone);
    MulticastDelegateBase single{}; single.delegate.method=&second; single.delegate.object.klass=&delegateClass;
    single.Remove(&added);
    if (!single.delegates || !single.GetMethods().empty()) return 1;
    reinterpret_cast<Array<DelegateBase*>*>(single.delegates)->Destroy();
    return 0;
}
static int StringViewLoggingRegression() {
    // Guard page immediately after an unterminated lookup name.
    const auto page=static_cast<size_t>(getpagesize());
    auto *memory=static_cast<char*>(mmap(nullptr,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    if (memory==MAP_FAILED || mprotect(memory+page,page,PROT_NONE)) return 1;
    memory[page-1]='X'; std::string_view name(memory+page-1,1);
    bool absent=!Class(name,name) && !Class(&fixtureClass).GetMethod(name,0).IsValid();
    munmap(memory,page*2); return absent ? 0 : 1;
}
static int DecoderRegression() {
    using Internal::DecodeX86;
    const uint8_t immediate[]={0xb8,0xe8,0,0,0,0x90,0xc3};
    if (DecodeX86(immediate,sizeof(immediate),true).length!=5 || DecodeX86(immediate,sizeof(immediate),true).call) return 1;
    const uint8_t memory[]={0x48,0x8b,0x84,0x24,0xe8,0,0,0};
    if (DecodeX86(memory,sizeof(memory),true).length!=8) return 1;
    const uint8_t call[]={0xe8,0xfb,0xff,0xff,0xff};
    auto result=DecodeX86(call,sizeof(call),true);
    if (!result.call || result.length!=5 || result.displacement!=1) return 1;
    for (size_t i=0;i<sizeof(call);++i) if (DecodeX86(call,i,true).length) return 1;
    const uint8_t ret[]={0xc3,0xe8,0,0,0,0};
    const uint8_t vex[]={0xc5,0xe8,0,0,0,0};
    if (!DecodeX86(ret,sizeof(ret),true).stop || DecodeX86(vex,sizeof(vex),true).length) return 1;
    return 0;
}
static int RuntimeListRegression() {
    using namespace Structures::Mono;
    static IL2CPP::Il2CppClass open{}, closed{};
    static MonoType reflected{};
    static bool failObject{};
    open.name="List`1"; open.namespaze="System.Collections.Generic";
    closed.name="List<int>"; closed.namespaze="Audit"; closed.instance_size=sizeof(List<int>);
    reflected.type=Class(&closed).GetIl2CppType();
    Internal::vmData.System$$List=&open;
    Internal::il2cppMethods.il2cpp_type_get_object=+[](const IL2CPP::Il2CppType*)->IL2CPP::Il2CppObject* {
        return reinterpret_cast<IL2CPP::Il2CppObject*>(&reflected);
    };
    Internal::il2cppMethods.il2cpp_class_from_il2cpp_type=+[](const IL2CPP::Il2CppType*) { return &closed; };
    IL2CPP::MethodInfo generic{}; generic.name="MakeGenericType"; generic.flags=0x10; generic.parameters_count=2;
    generic.methodPointer=reinterpret_cast<IL2CPP::Il2CppMethodPointer>(+[](MonoType*,Array<MonoType*>*,IL2CPP::MethodInfo*) { return &reflected; });
    Internal::vmData.RuntimeType$$MakeGenericType=Method<MonoType*>(&generic);
    IL2CPP::MethodInfo ctor{}; ctor.name=".ctor"; ctor.slot=65535;
    PRIVATE_INTERNAL::GetMethodClass(&ctor)=&closed;
    ctor.methodPointer=reinterpret_cast<IL2CPP::Il2CppMethodPointer>(+[](IL2CPP::Il2CppObject* obj,IL2CPP::MethodInfo*) {
        auto *list=static_cast<List<int>*>(obj); list->items=Array<int>::Create(1,true);
    });
    const IL2CPP::MethodInfo *methods[]={&ctor}; closed.methods=methods; closed.method_count=1;
    Internal::il2cppMethods.il2cpp_object_new=+[](const IL2CPP::Il2CppClass* cls)->IL2CPP::Il2CppObject* {
        if (failObject) return nullptr;
        auto *obj=static_cast<IL2CPP::Il2CppObject*>(std::calloc(1,cls->instance_size));
        if (obj) obj->klass=const_cast<IL2CPP::Il2CppClass*>(cls); return obj;
    };
    auto *list=Class(&fixtureClass).NewListBNM<int>();
    if (!list || list->klass!=&closed || !list->items || closed.methods[0]!=&ctor) return 1;
    list->Add(7); bool ok=list->get_Item(0)==7;
    list->items->Destroy(); std::free(list);
    failObject=true; if (Class(&fixtureClass).NewListBNM<int>()) return 1; failObject=false;
    failNextAllocation=true; if (Class(&fixtureClass).NewListBNM<int>() || failNextAllocation) return 1;
    Internal::vmData.RuntimeType$$MakeGenericType=Method<MonoType*>{};
    if (Class(&fixtureClass).NewListBNM<int>()) return 1;
    return ok ? 0 : 1;
}

static int ListGrowthLimitRegression() {
    using namespace Structures::Mono;
    static size_t requested{};
    Internal::il2cppMethods.il2cpp_gc_alloc_fixed=+[](size_t bytes)->void* { requested=bytes; return nullptr; };
    Array<int> header{}; header.capacity=INT32_MAX-1;
    List<int> list{}; list.items=&header; list.size=INT32_MAX-1;
    list.GrowIfNeeded(1);
    const auto maximumBytes=sizeof(Array<int>)+size_t{INT32_MAX}*sizeof(int);
    if (requested!=maximumBytes || list.items!=&header || list.size!=INT32_MAX-1) return 1;
    requested=0; list.GrowIfNeeded(2);
    return requested==0 ? 0 : 1;
}
