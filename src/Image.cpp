#include <BNM/Image.hpp>
#include <BNM/Class.hpp>
#include <Internals.hpp>

BNM::Image::Image(const std::string_view &name) {
    _data = Internal::TryGetImage(name);

    BNM_LOG_WARN_IF(!_data, DBG_BNM_MSG_Image_Constructor_NotFound, name.data());
}

BNM::Image::Image(const BNM::IL2CPP::Il2CppAssembly *assembly) {
    _data = Internal::il2cppMethods.il2cpp_assembly_get_image(assembly);
}

std::vector<BNM::Class> BNM::Image::GetClasses(bool includeInner) const {
    std::vector<IL2CPP::Il2CppClass *> classes{};

    if (!_data) return {};

    if (_data->nameToClassHashTable == reinterpret_cast<decltype(_data->nameToClassHashTable)>(-0x424e4d)) goto NEW_CLASSES;


    if (Internal::il2cppMethods.il2cpp_image_get_class) {
        size_t typeCount = _data->typeCount;

        for (uint32_t i = 0; i < typeCount; ++i) {
            auto cls = Internal::il2cppMethods.il2cpp_image_get_class(_data, i);
            if (!cls) continue;
            if ((!includeInner && cls->declaringType) || (!cls->flags && strcmp(cls->name, BNM_OBFUSCATE("<Module>")) == 0)) continue;
            classes.push_back(cls);
        }

    } else {
        Internal::Image$$GetTypes(_data, false, &classes);

        if (includeInner) goto NEW_CLASSES;

        for (auto it = classes.begin(); it != classes.end();) {
            if ((*it)->declaringType) {
                it = classes.erase(it);
                continue;
            }
            ++it;
        }
    }

    NEW_CLASSES:

#ifdef BNM_CLASSES_MANAGEMENT
    Internal::ClassesManagement::bnmClassesMap.ForEachByImage(_data, [&classes, includeInner](IL2CPP::Il2CppClass *BNMClass) -> bool {
        if (!includeInner && BNMClass->declaringType) return false;

        classes.push_back(BNMClass);
        return false;
    });
#endif

    std::vector<Class> result;
    result.reserve(classes.size());
    for (auto *klass : classes) result.emplace_back(klass);
    return result;
}

std::vector<BNM::Image> BNM::Image::GetImages() {
    auto assembliesPtr = Internal::il2cppMethods.Assembly$$GetAllAssemblies();
    if (!assembliesPtr) return {};

    auto &assemblies = *assembliesPtr;

    std::vector<Image> ret{};
    ret.reserve(assemblies.size());

    for (auto assembly : assemblies) ret.emplace_back(Internal::il2cppMethods.il2cpp_assembly_get_image(assembly));

    return ret;
}
