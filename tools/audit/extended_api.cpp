// Additional compile-only checks; no methods below are run by the audit.
#include <BNM/Class.hpp>
#include <BNM/Field.hpp>
#include <BNM/Method.hpp>
#include <BNM/Event.hpp>
#include <BNM/Property.hpp>
#include <BNM/UnityStructures.hpp>
#include <BNM/ClassesManagement.hpp>
#include <BNM/Coroutine.hpp>

#ifdef BNM_CLASSES_MANAGEMENT
struct AuditCustom : BNM::IL2CPP::Il2CppObject
{
    int value{};
    void Tick() {}
    BNM_CustomClass(AuditCustom, BNM::CompileTimeClassBuilder("Audit", "Custom").Build(), {}, {});
    BNM_CustomField(value, BNM::Defaults::Get<int>(), "value");
    BNM_CustomMethod(Tick, false, BNM::Defaults::Get<void>(), "Tick");
};
#endif

void InstantiateEvents(BNM::UnityEngine::UnityEvent<int>* event, BNM::Class cls)
{
    event->Invoke(1);
    (void)cls.NewList<int>();
}

#if defined(BNM_CLASSES_MANAGEMENT) && defined(BNM_COROUTINE)
BNM::Coroutine::IEnumerator InstantiateCoroutine()
{
    co_yield BNM::Coroutine::YieldInstruction(nullptr);
}
#endif
