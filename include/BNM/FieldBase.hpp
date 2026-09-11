#pragma once

#include "UserSettings/GlobalSettings.hpp"
#include "Il2CppHeaders.hpp"
#include "Class.hpp"

// NOLINTBEGIN
namespace BNM {

#pragma pack(push, 1)
    template <typename T> struct Field;

    /**
        @brief Class base for working with il2cpp fields.

        This class provides fields and some basic functions to allow to get and set fields.
    */
    struct FieldBase {
        /**
            @brief Create empty field base.
        */
        inline constexpr FieldBase() = default;

        /**
            @brief Copy field.
            @param other Other field
        */
        inline FieldBase(const FieldBase &other) = default;

        /**
            @brief Copy field.
            @param other Other field
        */
        inline FieldBase &operator=(const FieldBase &other) = default;

        /**
            @brief Create event from il2cpp field.
            @param info Il2cpp field
        */
        FieldBase(IL2CPP::FieldInfo *info);

        /**
            @brief Set field instance if it's non-static.
            @param instance Instance
            @return Reference to current FieldBase
        */
        FieldBase &SetInstance(IL2CPP::Il2CppObject *val);

        /**
            @brief Get FieldInfo.
            @return FieldInfo
        */
        [[nodiscard]] inline IL2CPP::FieldInfo *GetInfo() const { return _data; }

        /**
            @brief Get field offset.
            @return Field offset if field is valid, otherwise zero.
        */
        [[nodiscard]] inline BNM_PTR GetOffset() const { return _data ? static_cast<BNM_PTR>(_data->offset) - (_isInStruct && !_isStatic && !_isThreadStatic ? sizeof(IL2CPP::Il2CppObject) : 0x0) : 0; }

        /**
            @brief Get field pointer.
            @warning Don't support thread static fields!

            It's useful for structs or if you want to save pointer to be able to use it directly without BNM's overhead.

            @return Field pointer if field is valid and instance is set for non-static field, otherwise null.
        */
        [[nodiscard]] void *GetFieldPointer() const;

        /**
            @brief Get field type.
            @return BNM::Class of field's type
        */
        [[nodiscard]] BNM::Class GetType() const;

        /**
            @brief Get class that contains this field.
            @return BNM::Class of field's class
        */
        [[nodiscard]] BNM::Class GetParentClass() const;

        /**
            @brief Operator for setting instance.
            @param instance Instance
            @return Reference to current FieldBase
        */
        inline FieldBase &operator[](void *instance) { SetInstance(reinterpret_cast<IL2CPP::Il2CppObject *>(instance)); return *this;}

        /**
            @brief Operator for setting instance.
            @param instance Instance
            @return Reference to current FieldBase
        */
        inline FieldBase &operator[](IL2CPP::Il2CppObject *instance) { SetInstance(instance); return *this;}

        /**
            @brief Operator for setting instance.
            @param instance Instance
            @return Reference to current FieldBase
        */
        inline FieldBase &operator[](UnityEngine::Object *instance) { SetInstance(reinterpret_cast<IL2CPP::Il2CppObject *>(instance)); return *this;}

        /**
            @brief Check if field is valid.
            @return State of field
        */
        [[nodiscard]] inline constexpr bool IsValid() const noexcept { return _data != nullptr; }

#ifdef BNM_ALLOW_STR_METHODS

        /**
            @brief Get full field name.

            Returns string with full field name: ClassFullName.(field name)

            @return Field's full name or "Dead field"
        */
        [[nodiscard]] inline std::string str() const {
            if (_data) return Class(_data->parent).str() + BNM_OBFUSCATE(".(") + _data->name + BNM_OBFUSCATE(")");
            return BNM_OBFUSCATE(DBG_BNM_MSG_FieldBase_str_nullptr);
        }
#endif

        /**
           @brief Cast field to be able to get and set it with specified type.
        */
        template<typename NewType> inline Field<NewType> &cast() const { return const_cast<Field<NewType> &>(static_cast<const Field<NewType> &>(*this)); }

        BNM::IL2CPP::FieldInfo *_data{};
        BNM::IL2CPP::Il2CppObject *_instance{};
        uint8_t _isStatic : 1 = false, _isThreadStatic : 1 = false, _isInStruct : 1 = false, _isConst : 1 = false;

        constexpr FieldBase(FieldBase &&other) noexcept
            : _data(other._data), _instance(other._instance),
              _isStatic(other._isStatic), _isThreadStatic(other._isThreadStatic),
              _isInStruct(other._isInStruct), _isConst(other._isConst) {
            other._data = nullptr;
            other._instance = nullptr;
            other._isStatic = false;
            other._isThreadStatic = false;
            other._isInStruct = false;
            other._isConst = false;
        }

        constexpr FieldBase &operator=(FieldBase &&other) noexcept {
            if (this != &other) {
                _data = other._data;
                _instance = other._instance;
                _isStatic = other._isStatic;
                _isThreadStatic = other._isThreadStatic;
                _isInStruct = other._isInStruct;
                _isConst = other._isConst;
                other._data = nullptr;
                other._instance = nullptr;
                other._isStatic = false;
                other._isThreadStatic = false;
                other._isInStruct = false;
                other._isConst = false;
            }
            return *this;
        }
    };

#pragma pack(pop)

}
// NOLINTEND
