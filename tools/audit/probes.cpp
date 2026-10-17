// Diagnostic probes against actual BNM code with a synthetic IL2CPP API table.
// Separate processes are required: some cases intentionally expose UB/crashes.
#include <Internals.hpp>
#include <BNM/ComplexMonoStructures.hpp>
#include <BNM/Coroutine.hpp>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <atomic>

using namespace BNM;
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

static std::atomic<bool> entered{false}, releaseInit{false};

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
        (void)original.ToClass();
        (void)copy.ToClass(); // Shared descriptor was freed by original.ToClass().
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
            entered.store(true);
            while (!releaseInit.load()) std::this_thread::yield();
        };
        std::thread worker([] { (void)Class(&fixtureClass).GetClass(); });
        while (!entered.load()) std::this_thread::yield();
        // Insertion already completed; isolate early publication from a concurrent insertion race.
        (void)Class(&fixtureClass).GetClass();
        std::puts("second GetClass returned before first Class$$Init completed");
        releaseInit.store(true);
        worker.join();
        return 1;
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
        std::printf("capacity=%zu\n", static_cast<size_t>(array->capacity));
        array->m_Items[0] = 1; // Size calculation wrapped to header-only allocation.
        array->Destroy();
    }
    else if (mode == "coroutine-yield")
    {
        int progress = 0;
        auto coroutine = YieldNull(&progress);
        auto* object = coroutine.Get();
        std::printf("progress=%d klass=%p\n", progress, static_cast<void*>(object->klass));
        return progress == 1 && object->klass != nullptr ? 0 : 1;
    }
    else if (mode == "coroutine-temporary")
    {
        int progress = 0;
        auto* object = YieldNull(&progress).Get();
        volatile auto* klass = object->klass; // Temporary IEnumerator died at previous semicolon.
        std::printf("klass=%p\n", const_cast<IL2CPP::Il2CppClass*>(klass));
    }
    else if (mode == "missing-symbol")
    {
        Loading::SetMethodFinder(+[](const char*, void*) -> void* { return nullptr; }, nullptr);
        Internal::SetupBNM();
    }
    else if (mode == "custom-method-limit")
    {
        MANAGEMENT_STRUCTURES::CustomClass target{};
        target._targetType = CompileTimeClassBuilder("Audit", "Fixture").Build();
        MANAGEMENT_STRUCTURES::CustomMethod method{};
        method._name = "TooManyParameters";
        method._parameterTypes.resize(256);
        target._methods.push_back(&method);
        ClassesManagement::ProcessClassRuntime(&target);
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
    else return 64;
}
