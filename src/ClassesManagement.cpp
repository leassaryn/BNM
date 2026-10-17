#include <cstring>

#include <BNM/ClassesManagement.hpp>

#ifdef BNM_CLASSES_MANAGEMENT

#include <Internals.hpp>

#define BNM_CLASS_ALLOCATED_METHODS_FLAG 0x01000000
#define BNM_CLASS_ALLOCATED_FIELDS_FLAG 0x02000000

// il2cpp MethodAttributes flags
#define BNM_METHOD_PUBLIC           0x0006
#define BNM_METHOD_STATIC           0x0010
#define BNM_METHOD_HIDE_BY_SIG      0x0080
#define BNM_METHOD_SPECIAL_NAME     0x0800
#define BNM_METHOD_RT_SPECIAL_NAME  0x1000
#define BNM_CLASS_ALLOCATED_INNER_FLAG 0x04000000
#define BNM_CLASS_ALLOCATED_HIERARCHY_FLAG 0x08000000

using namespace BNM;

namespace {
    std::mutex registrationMutex;
    void *AllocateMetadata(size_t size) {
        auto *memory = BNM_malloc(size ? size : 1);
        if (!memory) throw std::bad_alloc();
        std::memset(memory, 0, size);
        return memory;
    }
}

void MANAGEMENT_STRUCTURES::AddClass(CustomClass *_class) {
    if (!_class) return;
    std::lock_guard lock(registrationMutex);
    if (!Internal::ClassesManagement::classesManagementVector)
        Internal::ClassesManagement::classesManagementVector = new (AllocateMetadata(sizeof(std::vector<MANAGEMENT_STRUCTURES::CustomClass *>))) std::vector<MANAGEMENT_STRUCTURES::CustomClass *>();

    Internal::ClassesManagement::classesManagementVector->push_back(_class);
}


#define BNM_I2C_NEW(type) reinterpret_cast<IL2CPP::type *>(AllocateMetadata(sizeof(IL2CPP::type)))

struct CustomClassInfo {
    const char *_namespace{}, *_name{}, *_imageName{};
    Class _class{};
};

// The code for changing the data of an existing class
static void ModifyClass(MANAGEMENT_STRUCTURES::CustomClass *customClass, Class target);
// Code for creating new classes
static void CreateClass(MANAGEMENT_STRUCTURES::CustomClass *customClass, const CustomClassInfo &classInfo);

// Find the desired class or information about it
static CustomClassInfo GetClassInfo(const CompileTimeClass &compileTimeClass);

// The code for creating a new image and assembling it
static IL2CPP::Il2CppImage *MakeImage(std::string_view imageName);
// The code for processing a new method for its subsequent creation/modification
static IL2CPP::MethodInfo *ProcessCustomMethod(MANAGEMENT_STRUCTURES::CustomMethod *method, Class target, bool *hooked = nullptr);
// The code for creating new methods
static IL2CPP::MethodInfo *CreateMethod(MANAGEMENT_STRUCTURES::CustomMethod *method);
// The code for setting data in fields
static void SetupField(IL2CPP::FieldInfo *newField, MANAGEMENT_STRUCTURES::CustomField *field);

// Code for getting all interfaces of class parent and interfaces
static void GetAllInterfaces(IL2CPP::Il2CppClass *parent, IL2CPP::Il2CppClass *interface, std::vector<IL2CPP::Il2CppClass *> &outInterfaces);
// The code for changing the owner of the nested class
static void SetupClassOwner(IL2CPP::Il2CppClass *target, IL2CPP::Il2CppClass *owner);
// The code for changing the parent of the class
static void SetupClassParent(IL2CPP::Il2CppClass *target, IL2CPP::Il2CppClass *parent);
// Code for checking whether a class and its parents have an interface
static bool HasInterface(IL2CPP::Il2CppClass *parent, IL2CPP::Il2CppClass *interface);
// Class type setup code
static void SetupTypes(IL2CPP::Il2CppClass *target);

bool Internal::ClassesManagement::ProcessCustomClasses() {
    std::vector<MANAGEMENT_STRUCTURES::CustomClass *> pending;
    {
        std::lock_guard lock(registrationMutex);
        if (!classesManagementVector) return true;
        pending.swap(*classesManagementVector);
        classesManagementVector->~vector();
        BNM_free(classesManagementVector);
        classesManagementVector = nullptr;
    }
    bool success = true;
    for (auto *customClass : pending) {
        BNM::ClassesManagement::ProcessClassRuntime(customClass);
        success = success && customClass && customClass->myClass;
    }
    return success;
}

void ClassesManagement::ProcessClassRuntime(MANAGEMENT_STRUCTURES::CustomClass *customClass) try {
    // All metadata writers must run at a consumer-controlled runtime safe point.
    static std::recursive_mutex mutationMutex;
    std::lock_guard lock(mutationMutex);
    if (!customClass || customClass->_methods.size() > UINT16_MAX ||
        customClass->_fields.size() > UINT16_MAX || customClass->_interfaces.size() > UINT16_MAX ||
        customClass->_size > INT32_MAX) {
        BNM_LOG_ERR("Custom class descriptor exceeds metadata limits");
        return;
    }
    customClass->myClass = nullptr;
    customClass->type = nullptr;
    for (auto *method : customClass->_methods) {
        if (!method || method->_parameterTypes.size() > UINT8_MAX || !method->_address || !method->_invoker) {
            BNM_LOG_ERR("Invalid custom method descriptor");
            return;
        }
    }
    for (auto *field : customClass->_fields) {
        if (!field || field->offset > customClass->_size || field->_size > customClass->_size - field->offset) {
            BNM_LOG_ERR("Custom field exceeds declared instance size");
            return;
        }
    }
    auto &type = customClass->_targetType;
    if (!type._stack.IsEmpty()) type._RemoveBit();
    auto info = GetClassInfo(type);
    if (info._class) ModifyClass(customClass, info._class);
    else if (info._name) CreateClass(customClass, info);
    else {
#ifdef BNM_DEBUG
        BNM_LOG_ERR(DBG_BNM_MSG_ClassesManagement_ProcessCustomClasses_Error);
        Utils::LogCompileTimeClass(type);
#endif
    }
    type.Free();
    customClass->_interfaces.clear();
    customClass->_interfaces.shrink_to_fit();
    customClass->_fields.clear();
    customClass->_fields.shrink_to_fit();
    customClass->_methods.clear();
    customClass->_methods.shrink_to_fit();
} catch (const std::bad_alloc &) {
    BNM_LOG_ERR("Custom metadata allocation failed; stop using this descriptor and do not retry mutation");
}

static void ModifyClass(MANAGEMENT_STRUCTURES::CustomClass *customClass, Class target) {
    BNM_LOG_DEBUG(DBG_BNM_MSG_ClassesManagement_ModifyClasses_Target, target.str().data());

    auto klass = target._data;

    auto baseType = customClass->_baseType.ToClass();

    IL2CPP::Il2CppClass *owner = customClass->_owner;

    // Existing managed instances and derived layouts cannot be resized safely.
    if (!customClass->_fields.empty() || (baseType && baseType._data != klass->parent) ||
        customClass->_methods.size() > static_cast<size_t>(UINT16_MAX - klass->method_count)) {
        BNM_LOG_ERR("Cannot grow an existing class layout or exceed its method limit");
        return;
    }
    SetupClassOwner(klass, owner);

    auto newMethodsCount = customClass->_methods.size();

    if (newMethodsCount) {
        auto oldCount = klass->method_count;
        auto oldMethods = klass->methods;

        std::vector<IL2CPP::MethodInfo *> methodsToAdd{};

        for (size_t i = 0; i < newMethodsCount; ++i) {
            auto method = customClass->_methods[i];

            [[maybe_unused]] auto paramCount = method->_parameterTypes.size();

            bool isHooked = false;
            method->myInfo = ProcessCustomMethod(method, target, &isHooked);

            if (!method->myInfo) return;
            PRIVATE_INTERNAL::GetMethodClass(method->myInfo) = klass;

            if (!isHooked) methodsToAdd.push_back(method->myInfo);
            BNM_LOG_DEBUG_IF(isHooked, DBG_BNM_MSG_ClassesManagement_ModifyClasses_Hooked_Method, method->_isStatic == 1 ? DBG_BNM_MSG_ClassesManagement_Method_Static : "", method->_name.data(), (unsigned long) paramCount);
            BNM_LOG_DEBUG_IF(!isHooked, DBG_BNM_MSG_ClassesManagement_ModifyClasses_Added_Method, method->_isStatic == 1 ? DBG_BNM_MSG_ClassesManagement_Method_Static : "", method->_name.data(), (unsigned long) paramCount);
            BNM_LOG_DEBUG_IF(method->_origin && method->_origin != method->myInfo, DBG_BNM_MSG_ClassesManagement_ModifyClasses_Overridden_Method, MethodBase(method->_origin).str().c_str());
        }

        if (!methodsToAdd.empty()) {
            auto newMethods = reinterpret_cast<IL2CPP::MethodInfo **>(AllocateMetadata((oldCount + methodsToAdd.size()) * sizeof(IL2CPP::MethodInfo *)));

            auto oldSize = oldCount * sizeof(IL2CPP::MethodInfo *);

            if (oldCount) memcpy(newMethods, oldMethods, oldSize);

            memcpy(newMethods + oldCount, methodsToAdd.data(), methodsToAdd.size() * sizeof(IL2CPP::MethodInfo *));

            if ((klass->flags & BNM_CLASS_ALLOCATED_METHODS_FLAG) == BNM_CLASS_ALLOCATED_METHODS_FLAG) BNM_free(klass->methods);
            klass->flags |= BNM_CLASS_ALLOCATED_METHODS_FLAG;

            klass->methods = (const IL2CPP::MethodInfo **)newMethods;
            klass->method_count += methodsToAdd.size();
        }
    }


    customClass->myClass = klass;
    customClass->type = Class(klass);
}

static char forEmptyString = '\0';
static const char *baseImageName = BNM_OBFUSCATE("Assembly-CSharp.dll");
static void CreateClass(MANAGEMENT_STRUCTURES::CustomClass *customClass, const CustomClassInfo &classInfo) {
    Image image{};
    if (classInfo._imageName) {
        auto assembliesPtr = Internal::il2cppMethods.Assembly$$GetAllAssemblies();
        if (assembliesPtr) {
            auto &assemblies = *assembliesPtr;
            for (auto assembly: assemblies) {
                auto currentImage = Internal::il2cppMethods.il2cpp_assembly_get_image(assembly);
                if (!Internal::CompareImageName(currentImage, classInfo._imageName)) continue;
                image = currentImage;
                break;
            }
        }
    } else image = Image(baseImageName);
    if (!image) image = MakeImage(classInfo._imageName ? classInfo._imageName : baseImageName);

    BNM_LOG_DEBUG(DBG_BNM_MSG_ClassesManagement_CreateClass_Target, classInfo._namespace ? classInfo._namespace : &forEmptyString, classInfo._name, image._data->name);
    
    IL2CPP::Il2CppClass *parent = customClass->_baseType;
    if (!parent) parent = Internal::vmData.Object;
    if (!parent || customClass->_size < parent->instance_size || parent->typeHierarchyDepth == UINT8_MAX) {
        BNM_LOG_ERR("Invalid custom class parent or instance size");
        return;
    }

    IL2CPP::Il2CppClass *owner = customClass->_owner;


    std::vector<IL2CPP::Il2CppRuntimeInterfaceOffsetPair> newInterOffsets{};
    if (parent->interfaceOffsets)
        for (uint16_t i = 0; i < parent->interface_offsets_count; ++i)
            newInterOffsets.push_back(parent->interfaceOffsets[i]);

    auto allInterfaces = customClass->_interfaces;
    std::vector<IL2CPP::Il2CppClass *> interfaces{};
    for (auto &interface : allInterfaces)
        if (auto cls = interface.ToClass(); cls)
            GetAllInterfaces(parent, cls, interfaces);

    // Required to override virtual methods
    size_t newVtableSize = parent->vtable_count;
    std::vector<IL2CPP::VirtualInvokeData> newVTable(newVtableSize);
    for (uint16_t i = 0; i < parent->vtable_count; ++i) newVTable[i] = parent->vtable[i];
    for (auto interface : interfaces) {
        if (interface->method_count > UINT16_MAX - newVtableSize) return;
        newInterOffsets.push_back({interface, static_cast<int32_t>(newVtableSize)});
        for (uint16_t i = 0; i < interface->method_count; ++i) {
            auto v = interface->methods[i];
            ++newVtableSize;
            newVTable.push_back({nullptr, v});
        }
    }

    if (interfaces.size() > UINT16_MAX || newInterOffsets.size() > UINT16_MAX) return;
    // Create all new methods
    uint8_t hasFinalize = 0;
    auto methods = reinterpret_cast<const IL2CPP::MethodInfo **>(AllocateMetadata(customClass->_methods.size() * sizeof(IL2CPP::MethodInfo *)));

    for (size_t i = 0; i < customClass->_methods.size(); ++i) {
        auto method = customClass->_methods[i];

        method->myInfo = ProcessCustomMethod(method, {});


        BNM_LOG_DEBUG(DBG_BNM_MSG_ClassesManagement_CreateClass_Added_Method, method->_isStatic == 1 ? DBG_BNM_MSG_ClassesManagement_Method_Static : "", method->_name.data(), (unsigned long) method->_parameterTypes.size());
        
        // Replacing non-static methods in the virtual methods table
        if (!method->_isStatic) for (uint16_t v = 0; v < newVtableSize; ++v) {
            auto &vTable = newVTable[v];
            if (!vTable.method) continue;
            auto count = vTable.method->parameters_count;

            if (!strcmp(vTable.method->name, method->myInfo->name) && count == method->myInfo->parameters_count) {
                if (!method->_skipTypeMatch) for (uint8_t p = 0; p < count; ++p) {
                    auto type = BNM::PRIVATE_INTERNAL::GetMethodParam(vTable.method, p);
                    auto type2 = BNM::PRIVATE_INTERNAL::GetMethodParam(method->myInfo, p);
                    if (Class(type).GetClass() != Class(type2).GetClass()) goto NEXT;
                }

                if (!hasFinalize) hasFinalize = v == Internal::finalizerSlot;
                method->_origin = const_cast<IL2CPP::MethodInfo *>(vTable.method);
                method->_originalAddress = reinterpret_cast<void *>(vTable.method ? vTable.method->methodPointer : nullptr);
                method->myInfo->slot = v;
                vTable.method = method->myInfo;
                vTable.methodPtr = method->myInfo->methodPointer;

                BNM_LOG_DEBUG(DBG_BNM_MSG_ClassesManagement_CreateClass_Overridden_Method, MethodBase(method->_origin).str().c_str());

                break;

            }
            NEXT:
            continue;
        }

        methods[i] = method->myInfo;

    }

    auto klass = reinterpret_cast<IL2CPP::Il2CppClass *>(AllocateMetadata(sizeof(IL2CPP::Il2CppClass) + newVTable.size() * sizeof(IL2CPP::VirtualInvokeData)));
    memset(klass, 0, sizeof(IL2CPP::Il2CppClass) + newVTable.size() * sizeof(IL2CPP::VirtualInvokeData));

    klass->image = image;

    auto len = strlen(classInfo._name);
    auto namePtr = reinterpret_cast<char *>(AllocateMetadata(len + 1));
    memcpy(namePtr, classInfo._name, len);
    namePtr[len] = 0;
    klass->name = namePtr;

    if (!owner && classInfo._namespace) {
        len = strlen(classInfo._namespace);
        auto nsPtr = reinterpret_cast<char *>(AllocateMetadata(len + 1));
        memcpy(nsPtr, classInfo._namespace, len);
        nsPtr[len] = 0;
        klass->namespaze = nsPtr;
    } else klass->namespaze = &forEmptyString;

    SetupTypes(klass);

    klass->element_class = klass;
    klass->castClass = klass;

    SetupClassParent(klass, parent);

    // BNM does not support the creation of generic classes
    klass->generic_class = nullptr;

#if UNITY_VER < 202
    klass->typeDefinition = nullptr;
#else
    klass->typeMetadataHandle = nullptr;
#endif


#if UNITY_VER > 174
    klass->klass = klass;
#endif

    klass->field_count = static_cast<uint16_t>(customClass->_fields.size());
    if (klass->field_count > 0) {
        // Create a list of fields
        auto fields = reinterpret_cast<IL2CPP::FieldInfo *>(AllocateMetadata(klass->field_count * sizeof(IL2CPP::FieldInfo)));

        // Get the first field
        IL2CPP::FieldInfo *newField = fields;
        for (auto field : customClass->_fields) {
            SetupField(newField, field);
            newField->parent = klass;

            newField->offset = static_cast<int32_t>(field->offset);

            newField++;
        }
        klass->fields = fields;
    } else klass->fields = nullptr;

    // Add Interfaces
    if (!interfaces.empty()) {
        klass->interfaces_count = static_cast<uint16_t>(interfaces.size());
        klass->implementedInterfaces = reinterpret_cast<IL2CPP::Il2CppClass **>(AllocateMetadata(interfaces.size() * sizeof(IL2CPP::Il2CppClass *)));
        memcpy(klass->implementedInterfaces, interfaces.data(), interfaces.size() * sizeof(IL2CPP::Il2CppClass *));
    } else {
        klass->interfaces_count = 0;
        klass->implementedInterfaces = nullptr;
    }

    // Completing the creation of methods
    for (auto method : customClass->_methods) PRIVATE_INTERNAL::GetMethodClass(method->myInfo) = klass;
    klass->method_count = static_cast<uint16_t>(customClass->_methods.size());
    klass->methods = methods;
    klass->has_finalize = hasFinalize;

    // Copy the parent flags, remove the ABSTRACT and INTERFACE flag if it exists and make type PUBLIC
    // TYPE_ATTRIBUTE_ABSTRACT, TYPE_ATTRIBUTE_INTERFACE, TYPE_ATTRIBUTE_PUBLIC
    klass->flags = static_cast<uint32_t>(static_cast<uint32_t>(klass->parent->flags) & ~(0x00000080u | 0x00000020u)) | 0x0000001u;

    // Initialize sizes
    klass->native_size = -1;
    klass->element_size = 0;
    klass->instance_size = klass->actualSize = static_cast<uint32_t>(customClass->_size);

    // Install a table of virtual methods
    klass->vtable_count = static_cast<uint16_t>(newVTable.size());
    for (size_t i = 0; i < newVTable.size(); ++i) klass->vtable[i] = newVTable[i];

    // Set interface addresses
    klass->interface_offsets_count = static_cast<uint16_t>(newInterOffsets.size());
    klass->interfaceOffsets = reinterpret_cast<IL2CPP::Il2CppRuntimeInterfaceOffsetPair *>(AllocateMetadata(newInterOffsets.size() * sizeof(IL2CPP::Il2CppRuntimeInterfaceOffsetPair)));
    memcpy(klass->interfaceOffsets, newInterOffsets.data(), newInterOffsets.size() * sizeof(IL2CPP::Il2CppRuntimeInterfaceOffsetPair));

    klass->interopData = nullptr;
    klass->events = nullptr; // Creation is not supported
    klass->properties = nullptr; // Creation is not supported
    klass->nestedTypes = nullptr;
#if UNITY_VER > 6003 || (UNITY_VER == 6003 && UNITY_PATCH_VER >= 3)
    klass->init_data.rgctx_data = nullptr; // Required for generic
#else
    klass->rgctx_data = nullptr; // Required for generic
#endif

    klass->static_fields = nullptr; // Creation is not supported
    klass->static_fields_size = 0;

    klass->genericRecursionDepth = 0;

#if UNITY_VER < 202
    klass->genericContainerIndex = -1;
#else
    klass->genericContainerHandle = nullptr;
#endif


#if UNITY_VER < 211
    // BNM создаёт только классы, т.к. новые struct особо не нужны для модификаций
    // BNM creates only classes, because new structs are not particularly needed for modifications
    klass->valuetype = 0;
#endif

    // BNM does not support creating a static field constructor (.cctor). Il2Cpp is designed so that it will never be called, and BNM does not allow you to create static fields
    klass->has_cctor = 0;

    // Useful for non Unity's Object delivered classes
    klass->has_references = 1;

    klass->size_inited = 1;
    klass->is_vtable_initialized = 1;

#if UNITY_VER > 182

#   if UNITY_VER < 222
    klass->naturalAligment = 1;
#   else
    klass->stack_slot_size = sizeof(void *);
#   endif

#endif
    klass->enumtype = 0;
    klass->token = 0;
    klass->minimumAlignment = 1;
    klass->is_generic = 0;
    klass->rank = 0;
    klass->nested_type_count = 0;
    klass->thread_static_fields_offset = 0;
    klass->thread_static_fields_size = 0;
    klass->cctor_started = 0;
    klass->packingSize = 0;

#if UNITY_VER >= 203 && (UNITY_VER != 211 || UNITY_PATCH_VER >= 24)
    klass->size_init_pending = 0;
#endif

#if UNITY_VER >= 212
    klass->cctor_finished_or_no_cctor = 1;
    klass->nullabletype = 0;
#else
    klass->cctor_finished = 1;
#endif

    klass->cctor_thread = 0;


    klass->initialized = 0;
    klass->init_pending = 0;
#if UNITY_VER > 182

    klass->initialized_and_no_error = 0;
    klass->initializationExceptionGCHandle = 0;

#if UNITY_VER < 212
    klass->has_initialization_error = 0;
#endif

#endif

    klass->gc_desc = nullptr;

    // Use Class::Init, to setup GC desc
    Internal::il2cppMethods.Class$$Init(klass);

    // Publish only completely initialized metadata.
    SetupClassOwner(klass, owner);
    Internal::ClassesManagement::bnmClassesMap.AddClass(image.GetInfo(), klass);
    customClass->type = Class(klass);
    customClass->myClass = klass;
}



namespace CompileTimeClassProcessors {
    typedef void (*ProcessorType)(CompileTimeClass &target, CompileTimeClass::_BaseInfo *info);
    extern ProcessorType processors[BNM::to_underlying(CompileTimeClass::_BaseType::MaxCount)];
}

static CustomClassInfo GetClassInfo(const CompileTimeClass &compileTimeClass) {
    CompileTimeClass tmp{};

    auto &stack = compileTimeClass._stack;

    if (stack.IsEmpty()) return {};

    auto lastElement = stack.lastElement;
    auto current = lastElement->next;
    do {
        auto info = current->value;

        auto index = BNM::to_underlying(info->_baseType);

        // Protection from other types. We are not interested in Generic, Modifier, and other things that do not relate to the class hierarchy.
        if (index != BNM::to_underlying(CompileTimeClass::_BaseType::Class)) {
            current = current->next;
            continue;
        }

        CompileTimeClassProcessors::processors[index](tmp, static_cast<CompileTimeClass::_BaseInfo *>(info));

        if (tmp._loadedClass) {
            current = current->next;
            continue;
        }

        bool found = true;
        auto next = current->next;
        while (next != lastElement->next) {
            if (next->value->_baseType == CompileTimeClass::_BaseType::Class) {
                found = false;
                break;
            }
            next = next->next;
        }

        auto classInfo = static_cast<CompileTimeClass::_ClassInfo *>(info);

        if (found) return {classInfo->_namespace, classInfo->_name, classInfo->_imageName};
        return {};

    } while (current != lastElement->next);
    return {._class = compileTimeClass.ToClass()};
}


static const char *dotDllString = BNM_OBFUSCATE(".dll");
static IL2CPP::Il2CppImage *MakeImage(std::string_view imageName) {
    auto newImg = BNM_I2C_NEW(Il2CppImage);

    if (imageName.ends_with(dotDllString)) imageName.remove_suffix(4);

    auto nameLen = imageName.size();
#if UNITY_VER >= 171
    auto nameNoExtPtr = reinterpret_cast<char *>(AllocateMetadata(nameLen + 1));
    memcpy(nameNoExtPtr, imageName.data(), nameLen);
    nameNoExtPtr[nameLen] = 0;
    newImg->nameNoExt = nameNoExtPtr;
#endif
    auto namePtr = reinterpret_cast<char *>(AllocateMetadata(nameLen + 5));
    memcpy(namePtr, imageName.data(), nameLen);
    auto nameEnd = namePtr + nameLen;
    nameEnd[0] = '.'; nameEnd[1] = 'd'; nameEnd[2] = 'l'; nameEnd[3] = 'l'; nameEnd[4] = 0;
    newImg->name = namePtr;

#if UNITY_VER > 182
    newImg->assembly = nullptr;
    newImg->customAttributeCount = 0;

#   if UNITY_VER < 201
    newImg->customAttributeStart = -1;
#   endif

#endif

#if UNITY_VER > 201
    // Create an empty Il2CppImageDefinition
    auto handle = reinterpret_cast<IL2CPP::Il2CppImageDefinition *>(AllocateMetadata(sizeof(IL2CPP::Il2CppImageDefinition)));
    memset(handle, 0, sizeof(IL2CPP::Il2CppImageDefinition));
    handle->typeStart = -1;
    handle->entryPointIndex = -1;
    handle->exportedTypeStart = -1;
    handle->typeCount = 0;
    handle->exportedTypeCount = 0;
    handle->nameIndex = -1;
    handle->assemblyIndex = -1;
    handle->customAttributeCount = 0;
    handle->customAttributeStart = -1;
    newImg->metadataHandle = reinterpret_cast<decltype(newImg->metadataHandle)>(handle);
#else
    // -1 для отключения анализа от il2cpp
    newImg->typeStart = -1;

#   if UNITY_VER >= 171
    newImg->exportedTypeStart = -1;
#   endif
#endif
    // Initialize variables
    newImg->typeCount = 0;
#if UNITY_VER >= 171
    newImg->exportedTypeCount = 0;
#endif
    newImg->token = 1;

    // Create a new build for the image
    auto newAsm = BNM_I2C_NEW(Il2CppAssembly);
    auto &aName = newAsm->aname;
#if UNITY_VER > 174
    // Set image and assembly
    newAsm->image = newImg;
    newAsm->image->assembly = newAsm;
    aName.name = newImg->name;
    aName.culture = nullptr;
    aName.public_key = nullptr;
#else
    // Negative value to disable analysis from il2cpp
    static int newAsmCount = 1;
    newImg->assemblyIndex = newAsm->imageIndex = -newAsmCount;
    newAsmCount++;
    aName.publicKeyIndex = aName.hashValueIndex = aName.nameIndex = 0;
    aName.cultureIndex = -1;
#endif
    // Initialize these variables just in case
    aName.revision = aName.build = aName.minor = aName.major = 0;
#if UNITY_VER > 174
    aName.public_key_token[0] = aName.hash_len = 0;
#else
    aName.publicKeyToken[0] = aName.hash_len = 0;
#endif

    aName.hash_alg = aName.flags = 0;
    newAsm->referencedAssemblyStart = -1;
    newAsm->referencedAssemblyCount = 0;

    // Using this, BNM can check if it has created this assembly
    newImg->nameToClassHashTable = reinterpret_cast<decltype(newImg->nameToClassHashTable)>(-0x424e4d);

    // Add an assembly to the list
    auto assembliesPtr = Internal::il2cppMethods.Assembly$$GetAllAssemblies();
    if (assembliesPtr) assembliesPtr->push_back(newAsm);

    BNM_LOG_INFO(DBG_BNM_MSG_ClassesManagement_MakeImage_Added_Image, imageName.data());

    return newImg;
}

static IL2CPP::VirtualInvokeData *TryFindVirtualMethod(Class target, IL2CPP::MethodInfo *targetMethod, uint16_t *outSlot = nullptr) {
    for (uint16_t i = 0; i < target._data->vtable_count; ++i) {
        auto &it = target._data->vtable[i];
        if (it.method != targetMethod) continue;
        if (outSlot) *outSlot = i;
        return &it;
    }
    if (outSlot) *outSlot = 65535;
    return nullptr;
}

static IL2CPP::MethodInfo *ProcessCustomMethod(MANAGEMENT_STRUCTURES::CustomMethod *method, Class target, bool *hooked) {
    if (!target) return CreateMethod(method);

    // Free _parameterTypes in any case
    struct _ClearMethod { // NOLINT
        MANAGEMENT_STRUCTURES::CustomMethod *method;
        ~_ClearMethod() {
            method->_parameterTypes.clear();
            method->_parameterTypes.shrink_to_fit();
        }
    } _ClearMethod{method}; // NOLINT

    if (method->_parameterTypes.size() > 255) {
        BNM_LOG_ERR("ProcessCustomMethod: parameter count %zu exceeds uint8_t max (255)", method->_parameterTypes.size());
        return nullptr;
    }
    auto parameters = static_cast<uint8_t>(method->_parameterTypes.size());

    auto originalMethod = Internal::IterateMethods(target, [method, parameters](IL2CPP::MethodInfo *klassMethod) {
        if (method->_name != klassMethod->name || klassMethod->parameters_count != parameters) return false;
        if (method->_skipTypeMatch) return true;

        for (uint8_t i = 0; i < parameters; ++i) {
            auto param = BNM::PRIVATE_INTERNAL::GetMethodParam(klassMethod, i);
            if (Class(param).GetClass() != method->_parameterTypes[i].ToIl2CppClass()) return false;
        }
        return true;
    });

    if (!originalMethod || PRIVATE_INTERNAL::GetMethodClass(originalMethod) != target._data) {
        bool isVirtual = originalMethod != nullptr && (originalMethod->flags & 0x0040) == 0x0040;
        auto parent = originalMethod;
        originalMethod = CreateMethod(method);
        if (!originalMethod) return nullptr;

        // Copy token to copy attributes
        if (!method->_copyTarget.empty()) {
            auto copyTarget = Internal::IterateMethods(target, [targetName = method->_copyTarget](IL2CPP::MethodInfo *method) { return targetName == method->name; });
            if (copyTarget) originalMethod->token = copyTarget->token;
        }

        bool hasNonVirtualParent = !isVirtual && parent;

        if (hasNonVirtualParent) {
            method->_origin = parent;
            method->_originalAddress = reinterpret_cast<void *>(parent->methodPointer);
            return originalMethod;
        }

        // Check if we're modifying class
        bool canDoVirtualHook = hooked && isVirtual /* Can be true only if parent not null */;

        if (!canDoVirtualHook) return originalMethod;

        // Trying to override virtual method
        uint16_t slot{};
        if (auto vTable = TryFindVirtualMethod(target, parent, &slot); vTable != nullptr) {
            method->_origin = parent;
            method->_originalAddress = reinterpret_cast<void *>(parent->methodPointer);
            originalMethod->slot = slot;
            vTable->methodPtr = reinterpret_cast<void (*)()>(method->_address);
            vTable->method = originalMethod;
            return originalMethod;
        }
    }


    if (method->_isInvokeHook) {
        method->_origin = originalMethod;
        method->_originalAddress = reinterpret_cast<void *>(originalMethod->methodPointer);
        originalMethod->methodPointer = reinterpret_cast<void(*)()>(method->_address);
        if (hooked) *hooked = true;
        return originalMethod;
    }

#ifdef BNM_AUTO_HOOK_DISABLE_VIRTUAL_HOOK
    goto SKIP_NON_VIRTUAL;
#endif

    if ((originalMethod->flags & 0x0040) == 0 || method->_isBasicHook) goto SKIP_NON_VIRTUAL;

    if (auto vTable = TryFindVirtualMethod(target, originalMethod); vTable != nullptr) {
        method->_origin = const_cast<IL2CPP::MethodInfo *>(vTable->method);
        method->_originalAddress = reinterpret_cast<void *>(method->_origin->methodPointer);
        vTable->methodPtr = reinterpret_cast<void(*)()>(method->_address);
        if (hooked) *hooked = true;
        return originalMethod;
    }

    SKIP_NON_VIRTUAL:

    method->_origin = originalMethod;
    if (!::BasicHook(reinterpret_cast<void *>(originalMethod->methodPointer), method->_address, method->_originalAddress)) {
        method->_origin = nullptr;
        method->_originalAddress = nullptr;
        BNM_LOG_ERR("Custom method hook installation failed");
        return nullptr;
    }

    if (hooked) *hooked = true;

    return originalMethod;
}

static IL2CPP::MethodInfo *CreateMethod(MANAGEMENT_STRUCTURES::CustomMethod *method) {
    if (method->_parameterTypes.size() > 255) {
        BNM_LOG_ERR("CreateMethod: parameter count %zu exceeds uint8_t max (255)", method->_parameterTypes.size());
        return nullptr;
    }
    auto *myInfo = BNM_I2C_NEW(MethodInfo);
    myInfo->methodPointer = reinterpret_cast<decltype(myInfo->methodPointer)>(method->_address);
    myInfo->invoker_method = reinterpret_cast<decltype(myInfo->invoker_method)>(method->_invoker);
    myInfo->parameters_count = static_cast<uint8_t>(method->_parameterTypes.size());

#if UNITY_VER >= 212
    myInfo->virtualMethodPointer = reinterpret_cast<decltype(myInfo->virtualMethodPointer)>(method->_address);
#endif

    auto name = reinterpret_cast<char *>(AllocateMetadata(method->_name.size() + 1));
    memcpy(reinterpret_cast<void *>(name), method->_name.data(), method->_name.size());
    name[method->_name.size()] = 0;
    myInfo->name = name;

    // Set method flags
    myInfo->flags = BNM_METHOD_PUBLIC | BNM_METHOD_HIDE_BY_SIG;
    if (method->_isStatic) myInfo->flags |= BNM_METHOD_STATIC;
    if (method->_name == Internal::constructorName) myInfo->flags |= BNM_METHOD_SPECIAL_NAME | BNM_METHOD_RT_SPECIAL_NAME;

    // BNM does not support the creation of generic methods
    myInfo->is_generic = false;
    myInfo->is_inflated = false;

    // If method is virtual BNM will set valid slot index later
    myInfo->slot = 65535;

    // Removes crash if code trying to do metadata lookup for that method
    myInfo->token = 0;

    // Set the return type
    auto methodType = method->_returnType.ToClass();
    if (!methodType) methodType = Internal::vmData.Object;
    myInfo->return_type = methodType.GetIl2CppType();

    // Create arguments
    auto argsCount = myInfo->parameters_count;
    if (argsCount) {
        auto &types = method->_parameterTypes;
#if UNITY_VER < 212
        myInfo->parameters = reinterpret_cast<IL2CPP::ParameterInfo *>(AllocateMetadata(argsCount * sizeof(IL2CPP::ParameterInfo)));

        auto parameter = const_cast<IL2CPP::ParameterInfo *>(reinterpret_cast<const IL2CPP::ParameterInfo *>(myInfo->parameters));
        for (uint8_t p = 0; p < argsCount; ++p) {
            // The name of parameter does not interest the engine in any way at all
            parameter->name = nullptr;
            parameter->position = p;

            // Set the type anyway
            auto type = p < types.size() ? types[p].ToClass() : Internal::vmData.Object;
            if (!type) type = Internal::vmData.Object;
            parameter->parameter_type = type.GetIl2CppType();
            parameter->token = parameter->parameter_type->attrs | p;

            ++parameter;
        }
#else

        auto parameters = reinterpret_cast<IL2CPP::Il2CppType **>(AllocateMetadata(argsCount * sizeof(IL2CPP::Il2CppType *)));

        for (uint8_t p = 0; p < argsCount; ++p) {
            auto parameter = BNM_I2C_NEW(Il2CppType);

            // Set the type anyway
            auto type = p < types.size() ? types[p].ToClass() : Internal::vmData.Object;
            if (!type) type = Internal::vmData.Object;
            *parameter = *type.GetIl2CppType();

            parameters[p] = parameter;
        }
        myInfo->parameters = const_cast<const IL2CPP::Il2CppType **>(parameters);
#endif
    }
    myInfo->rgctx_data = nullptr;
    myInfo->genericMethod = nullptr;

    method->_parameterTypes.clear();
    method->_parameterTypes.shrink_to_fit();

    return myInfo;
}

static void SetupField(IL2CPP::FieldInfo *newField, MANAGEMENT_STRUCTURES::CustomField *field) {
    auto name = field->_name;
    auto len = name.size();
    auto namePtr = reinterpret_cast<char *>(AllocateMetadata(len + 1));
    memcpy(namePtr, name.data(), len);
    namePtr[len] = 0;
    newField->name = namePtr;

    // Copy type
    auto newType = BNM_I2C_NEW(Il2CppType);
    auto fieldType = field->_type.ToClass();
    if (!fieldType) fieldType = Internal::vmData.Object;

    *newType = *fieldType.GetIl2CppType();
    newType->attrs = newField->token = 0x0006;
    newField->type = const_cast<const IL2CPP::Il2CppType *>(newType);
}

static void SetupClassOwner(IL2CPP::Il2CppClass *target, IL2CPP::Il2CppClass *owner) {
    if (!owner || target->declaringType == owner || owner->nested_type_count == UINT16_MAX) return;
    for (auto *ancestor = owner; ancestor; ancestor = ancestor->declaringType) if (ancestor == target) return;
    auto *oldOwner = target->declaringType;
    auto *oldList = Internal::PRIVATE_INTERNAL::ClassNestedTypes(owner);
    auto **newList = static_cast<IL2CPP::Il2CppClass **>(AllocateMetadata(sizeof(IL2CPP::Il2CppClass *) * (owner->nested_type_count + 1)));
    if (!newList) return;
    if (owner->nested_type_count) memcpy(newList, oldList, sizeof(IL2CPP::Il2CppClass *) * owner->nested_type_count);
    newList[owner->nested_type_count] = target;
    // Validate and allocate both sides before publishing either one.
    IL2CPP::Il2CppClass **replacement = nullptr;
    size_t removeIndex = 0;
    if (oldOwner) {
        auto **old = Internal::PRIVATE_INTERNAL::ClassNestedTypes(oldOwner);
        while (removeIndex < oldOwner->nested_type_count && old[removeIndex] != target) ++removeIndex;
        if (removeIndex == oldOwner->nested_type_count) { BNM_free(newList); return; }
        if (oldOwner->nested_type_count > 1) {
            replacement = static_cast<IL2CPP::Il2CppClass **>(AllocateMetadata(sizeof(*replacement) * (oldOwner->nested_type_count - 1)));
            if (!replacement) { BNM_free(newList); return; }
            size_t destination = 0;
            for (size_t i = 0; i < oldOwner->nested_type_count; ++i) if (i != removeIndex) replacement[destination++] = old[i];
        }
        oldOwner->nestedTypes = replacement;
        --oldOwner->nested_type_count;
        if (oldOwner->flags & BNM_CLASS_ALLOCATED_INNER_FLAG) BNM_free(old);
        oldOwner->flags |= BNM_CLASS_ALLOCATED_INNER_FLAG;
    }
    owner->nestedTypes = newList;
    ++owner->nested_type_count;
    target->declaringType = owner;
    if (owner->flags & BNM_CLASS_ALLOCATED_INNER_FLAG) BNM_free(oldList);
    owner->flags |= BNM_CLASS_ALLOCATED_INNER_FLAG;
}

static void SetupClassParent(IL2CPP::Il2CppClass *target, IL2CPP::Il2CppClass *parent) {
    // All C# classes should have parent. Only structs don't have it, but BNM don't allow to define valid structs
    if (!parent || parent->typeHierarchyDepth == UINT8_MAX) [[unlikely]] return;

    auto **hierarchy = static_cast<IL2CPP::Il2CppClass **>(AllocateMetadata((parent->typeHierarchyDepth + 1) * sizeof(IL2CPP::Il2CppClass *)));
    if (parent->typeHierarchyDepth) memcpy(hierarchy, parent->typeHierarchy, parent->typeHierarchyDepth * sizeof(IL2CPP::Il2CppClass *));
    hierarchy[parent->typeHierarchyDepth] = target;
    if (target->flags & BNM_CLASS_ALLOCATED_HIERARCHY_FLAG) BNM_free(target->typeHierarchy);
    target->flags |= BNM_CLASS_ALLOCATED_HIERARCHY_FLAG;
    target->typeHierarchyDepth = parent->typeHierarchyDepth + 1;
    target->typeHierarchy = hierarchy;
    target->parent = parent;
}

static void GetAllInterfaces(IL2CPP::Il2CppClass *parent, IL2CPP::Il2CppClass *interface, std::vector<IL2CPP::Il2CppClass *> &outInterfaces) { // NOLINT
    if (!interface || std::find(outInterfaces.begin(), outInterfaces.end(), interface) != outInterfaces.end()) return;
    Internal::il2cppMethods.Class$$Init(interface);
    if (HasInterface(parent, interface)) return;
    outInterfaces.push_back(interface);
    if (!interface->interfaces_count || interface->interfaces_count == static_cast<uint16_t>(-1)) return;

    for (uint16_t i = 0; i < interface->interfaces_count; ++i) GetAllInterfaces(parent, interface->implementedInterfaces[i], outInterfaces);
}

static bool HasInterface(IL2CPP::Il2CppClass *parent, IL2CPP::Il2CppClass *interface) { // NOLINT
    if (!parent || !interface) return false;
    for (uint16_t i = 0; i < parent->interfaces_count; ++i) if (parent->implementedInterfaces[i] == interface) return true;
    if (parent->parent) return HasInterface(parent->parent, interface);
    return false;
}

static void SetupTypes(IL2CPP::Il2CppClass *target) {
    IL2CPP::Il2CppType classType;
    memset(&classType, 0, sizeof(IL2CPP::Il2CppType));
    classType.type = IL2CPP::Il2CppTypeEnum::IL2CPP_TYPE_CLASS;
    classType.attrs = 0x0;
    classType.pinned = 0;
    classType.byref = 0;
    classType.num_mods = 31;
    classType.data.dummy = target;

#if UNITY_VER > 174
    target->this_arg = target->byval_arg = classType;
    target->this_arg.byref = 1;
#else
    target->byval_arg = BNM_I2C_NEW(Il2CppType);
    // byval_arg/this_arg are const pointers in il2cpp <= 2017; the memory is BNM-allocated, so writing through it is safe
    *const_cast<IL2CPP::Il2CppType *>(target->byval_arg) = classType;
    classType.byref = 1;
    target->this_arg = BNM_I2C_NEW(Il2CppType);
    *const_cast<IL2CPP::Il2CppType *>(target->this_arg) = classType;
#endif
}

#undef BNM_I2C_NEW
#endif
