// Compile-only API instantiations. Never run these against an uninitialized VM.
#include <BNM/Class.hpp>
#include <BNM/Field.hpp>
#include <BNM/Method.hpp>
#include <BNM/Property.hpp>
#include <BNM/Event.hpp>
#include <BNM/ComplexMonoStructures.hpp>
#include <BNM/ClassesManagement.hpp>
#include <BNM/Coroutine.hpp>

void Instantiate(BNM::Class cls, BNM::Structures::Mono::Array<int>* array,
                 BNM::Structures::Mono::List<int>* list,
                 const BNM::Structures::Mono::Dictionary<int, int>* dict,
                 BNM::MulticastDelegate<void>* delegate)
{
    list->Add(1);
    list->Insert(0, 2);
    list->RemoveAt(0);
    auto enumerator = list->GetEnumerator();
    (void)enumerator.MoveNext();
    (void)array->GetData();
    (void)dict->ContainsKey(1);
    (void)dict->ContainsValue(2);
    delegate->Invoke();
    cls.GetField("value").cast<int>().Set(1);
    (void)cls.GetMethod("Method", 0).cast<int>().Call();
    (void)cls.GetProperty("Property").cast<int>().Get();
}
