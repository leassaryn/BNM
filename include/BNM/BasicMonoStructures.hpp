#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-length-array"

#include <array>
#include <vector>
#include <string>
#include <string_view>
#include <limits>

#include "UserSettings/GlobalSettings.hpp"
#include "Il2CppHeaders.hpp"
#include "DebugMessages.hpp"
#include "Utils.hpp"
#include "Defaults.hpp"

// NOLINTBEGIN
namespace BNM::Utils {
    /**
        @brief Helper struct for checking values that passed from Mono::Array.
        @tparam T Data type
    */
    template<typename T>
    struct DataIterator {
        T *value{};
        IL2CPP::Il2CppObject *owner{};
        IL2CPP::Il2CppClass *valueClass{};
        /// @cond
        constexpr DataIterator() = default;
        constexpr DataIterator(const DataIterator &) = default;
        // Assignment changes the referenced element; construction copies the view.
        DataIterator &operator=(const DataIterator &other) {
            if (other.value) *this = T(*other.value);
            return *this;
        }
        const DataIterator &operator=(const DataIterator &other) const {
            if (other.value) *this = T(*other.value);
            return *this;
        }
        constexpr DataIterator(const T *value, IL2CPP::Il2CppObject *owner = nullptr,
                               IL2CPP::Il2CppClass *valueClass = nullptr)
            : value(const_cast<T *>(value)), owner(owner), valueClass(valueClass) {}
        inline T& operator *() {
            BNM_LOG_ERR_IF(!value, DBG_BNM_MSG_DataIterator_Error);
            return *value;
        }
        inline T& operator *() const {
            BNM_LOG_ERR_IF(!value, DBG_BNM_MSG_DataIterator_Error);
            return *value;
        }
        inline operator T&() {
            BNM_LOG_ERR_IF(!value, DBG_BNM_MSG_DataIterator_Error);
            return *value;
        }
        inline operator T&() const {
            BNM_LOG_ERR_IF(!value, DBG_BNM_MSG_DataIterator_Error);
            return *value;
        }
        inline T& operator ->() {
            BNM_LOG_ERR_IF(!value, DBG_BNM_MSG_DataIterator_Error);
            return *value;
        }
        inline T& operator ->() const {
            BNM_LOG_ERR_IF(!value, DBG_BNM_MSG_DataIterator_Error);
            return *value;
        }
        inline DataIterator &operator=(T t) {
            if (value) (void)StoreManagedValue(owner, value, t, valueClass);
            return *this;
        }
        inline const DataIterator &operator=(T t) const {
            if (value) (void)StoreManagedValue(owner, value, t, valueClass);
            return *this;
        }
        /// @endcond
    };
}

/**
    @brief Namespace that holds some basic and the most commonly used C# classes.

    It has common C# classes: string, array, List, Dictionary, Action.
*/
namespace BNM::Structures::Mono {

    /**
        @brief Struct of decimal type implementation.
    */
    struct decimal {
        int flags{};
        int hi{};
        int lo{};
        int mid{};
    };

    template<typename T> struct List;

    /// @cond
    namespace PRIVATE_MonoListData {
        void *CompareExchange4List(void *syncRoot);
    }
    /// @endcond

    /**
        @brief String type implementation.
    */
    struct String : BNM::IL2CPP::Il2CppObject {
        int length{};
        IL2CPP::Il2CppChar chars[0];
        /**
            @brief Convert Mono::String to std::string.

            Converts C#'s UTF16 string to C++'s UTF8 string.

            @return Converted string if it's valid, otherwise "ERROR: Mono::String dead".
        */
        [[nodiscard]] std::string str();

        /**
            @brief Calculate string hash.

            Calculates string's hash like it's done in il2cpp VM.

            @return Hash of string if it's valid, otherwise zero.
        */
        [[nodiscard]] unsigned int GetHash() const;

        /**
            @brief Get empty string.

            Gets C# empty string.

            @return Empty string if it's found, otherwise null.
        */
        [[nodiscard]] static String *Empty();

#ifdef BNM_ALLOW_SELF_CHECKS
        /**
            @brief Check if string isn't null.

            Checks if string isn't null and logs if it's null.
            It's used mostly internally in debug builds to crash proof code.

            @return True if string isn't null.
        */
        [[nodiscard]] bool SelfCheck() const;
#endif

        /**
            @brief Check if string is not null or not empty.
            @return True if string is not null and not empty.
        */
        inline bool IsNullOrEmpty() const { return !BNM::CheckForNull(this) || !length; }
    private:
        inline constexpr String() : BNM::IL2CPP::Il2CppObject() {}
    };

    /// @cond
    namespace __Internal_Array {
        void *ArrayFromClass(IL2CPP::Il2CppClass *, IL2CPP::il2cpp_array_size_t);
    }
    /// @endcond

    /**
        @brief Array type implementation.
        @tparam T Element type
    */
    template<typename T>
    struct Array : BNM::IL2CPP::Il2CppObject  {
        IL2CPP::Il2CppArrayBounds *bounds{};
        IL2CPP::il2cpp_array_size_t capacity{};
        T m_Items[0];
        /**
            @brief Get array size.

            @return Array size if it's valid, otherwise zero.
        */
        [[nodiscard]] inline IL2CPP::il2cpp_array_size_t GetCapacity() const { BNM_CHECK_SELF(0); return capacity; }

        /**
            @brief Get array size.

            Alias of GetCapacity().

            @return Array size if it's valid, otherwise zero.
        */
        [[nodiscard]] inline IL2CPP::il2cpp_array_size_t GetSize() const { return GetCapacity(); }

        /**
            @brief Get array data pointer.
            @return Array data pointer if array is valid, otherwise null.
        */
        [[nodiscard]] inline T *GetData() const { BNM_CHECK_SELF(nullptr); return const_cast<T *>(&m_Items[0]); }

        /**
            @brief Convert array to std::vector.
            @return Vector of elements if array is valid, otherwise empty vector.
        */
        [[nodiscard]] std::vector<T> ToVector() const {
            std::vector<T> ret;
            BNM_CHECK_SELF(ret);
            for (IL2CPP::il2cpp_array_size_t i = 0; i < capacity; i++) ret.push_back(m_Items[i]);
            return ret;
        }

        /**
            @brief Copy data from vector to current array.
            @param vec Source vector
            @return True if vector size is the same or less than array's size.
        */
        inline bool CopyFrom(const std::vector<T> &vec) { BNM_CHECK_SELF(false); if (vec.empty() || vec.size() > static_cast<size_t>(std::numeric_limits<IL2CPP::il2cpp_array_size_t>::max())) return false; return CopyFrom(const_cast<T *>(vec.data()), static_cast<IL2CPP::il2cpp_array_size_t>(vec.size())); }

        /**
            @brief Copy data from pointer to current array.

            @param arr Source pointer
            @param size Source pointer size

            @return True if size is the same or less than array's size.
        */
        bool CopyFrom(T *arr, IL2CPP::il2cpp_array_size_t size) {
            BNM_CHECK_SELF(false);
            return CopyRange(arr, size);
        }

        // All writes performed by BNM pass through this path, including overlapping copies.
        bool CopyRange(const T *source, IL2CPP::il2cpp_array_size_t count,
                       IL2CPP::il2cpp_array_size_t destination = 0) {
            if (std::cmp_less(count, 0) || std::cmp_less(destination, 0) || destination > capacity ||
                count > capacity - destination) return false;
            return CopyManagedRange(this, m_Items + destination, source, static_cast<size_t>(count),
                                    klass ? klass->element_class : nullptr);
        }

        bool Set(IL2CPP::il2cpp_array_size_t index, const T &value) {
            if (std::cmp_less(index, 0) || index >= capacity) return false;
            return StoreManagedValue(this, m_Items + index, value, klass ? klass->element_class : nullptr);
        }

        /**
            @brief Copy data from current array to some pointer.
        */
        inline void CopyTo(T *arr) const { BNM_CHECK_SELF(); if (!IsAllocated(m_Items)) return; memcpy(arr, m_Items, sizeof(T) * capacity); }

        /**
            @brief Get element at index.
            @return DataIterator of target element if array is valid and index isn't out of bounds, otherwise empty DataIterator.
        */
        inline Utils::DataIterator<T> At(IL2CPP::il2cpp_array_size_t index) const { BNM_CHECK_SELF({}); if (std::cmp_less(index, 0) || index >= GetCapacity()) return {}; return {&m_Items[index], const_cast<Array *>(this), klass ? klass->element_class : nullptr}; }
        inline Utils::DataIterator<T> operator[] (IL2CPP::il2cpp_array_size_t index) const { return At(index); }

        /**
            @brief Check if array is empty.
            @return True if array isn't empty.
        */
        [[nodiscard]] inline bool Empty() const { BNM_CHECK_SELF(false); return GetCapacity() <= 0;}

        /**
            @brief Creates new empty array of set capacity.

            @param capacity Size of new array
            @param _forceUseAlloc Forces code to create array using BNM::Allocate instead of BNM::Class::NewArray (Advanced usage only).

            @return New array
        */
        static Array<T> *Create(size_t capacity, bool _forceUseAlloc = false) {
            if (capacity > (SIZE_MAX - sizeof(Array)) / sizeof(T) ||
                capacity > static_cast<size_t>(std::numeric_limits<IL2CPP::il2cpp_array_size_t>::max())) return nullptr;
            auto cls = _forceUseAlloc ? BNM::Defaults::DefaultTypeRef{} : BNM::Defaults::Get<T>();
#ifndef  BNM_USE_IL2CPP_ALLOCATOR
            auto monoArr = reinterpret_cast<Array<T> *>(cls.IsValid() ? __Internal_Array::ArrayFromClass(cls, capacity) : BNM_malloc(sizeof(Array) + sizeof(T) * capacity));
#else
            auto monoArr = reinterpret_cast<Array<T> *>(cls.IsValid() ? __Internal_Array::ArrayFromClass(cls, capacity) : BNM::Allocate(sizeof(Array) + sizeof(T) * capacity));
#endif
            if (!monoArr) return nullptr;
            // Only clear the header for BNM-allocated arrays. il2cpp-allocated arrays (cls.IsValid())
            // already have klass/bounds set and elements zeroed by the GC; memset would wipe the klass.
            if (!cls.IsValid()) {
                memset(monoArr, 0, sizeof(Array) + sizeof(T) * capacity);
                monoArr->klass = nullptr;
            }
            monoArr->capacity = capacity;
            return monoArr;
        }

        /**
            @brief Creates new array using data from set vector.

            @param vec Source vector
            @param _forceUseAlloc Forces code to create array using BNM::Allocate instead of BNM::Class::NewArray (Advanced usage only).

            @return New array
        */
        static Array<T> *Create(const std::vector<T> &vec, bool _forceUseAlloc = false) { return Create((T *)vec.data(), vec.size(), _forceUseAlloc); }

        /**
            @brief Creates new array using data from set pointer.

            @param arr Source pointer
            @param size Source pointer size
            @param _forceUseAlloc Forces code to create array using BNM::Allocate instead of BNM::Class::NewArray (Advanced usage only).

            @return New array
        */
        static Array<T> *Create(T *arr, size_t size, bool _forceUseAlloc = false) {
            Array<T> *monoArr = Create(size, _forceUseAlloc);
            if (monoArr && !monoArr->CopyFrom(arr, static_cast<IL2CPP::il2cpp_array_size_t>(size))) {
                monoArr->Destroy();
                return nullptr;
            }
            return monoArr;
        }

        /**
            @brief Destroy array.

            Frees allocated memory for array.

            @warning Only for arrays created via BNM!
        */
        inline void Destroy() {
#ifndef  BNM_USE_IL2CPP_ALLOCATOR
            if (!klass) BNM_free(this);
#else
            if (!klass) BNM::Free(this);
#endif
        }

#ifdef BNM_ALLOW_SELF_CHECKS
        /**
            @brief Check if array isn't null.

            Checks if array is not null and logs if it's null.
            It's used mostly internally in debug builds to crash proof code.

            @return True if string isn't null.
        */
        [[nodiscard]] bool SelfCheck() const {
            if (CheckForNull(this)) return true;
            BNM_LOG_ERR(DBG_BNM_MSG_Array_SelfCheck_Error);
            return false;
        }
#endif

        /**
            @brief Create empty array.
        */
        inline constexpr Array() : BNM::IL2CPP::Il2CppObject() {
            klass = nullptr;
            monitor = nullptr;
        }
    };

    /**
        @brief System.Generic.List type implementation
        @tparam T Element type
    */
    template<typename T>
    struct List : BNM::IL2CPP::Il2CppObject {

        /**
            @brief System.Generic.List.Enumerator struct implementation

            Can be used to iterate list in foreach.
        */
        struct Enumerator {
            List<T> *list{};
            int index{};
            int version{};
            T current{};
            constexpr Enumerator() = default;
            explicit Enumerator(List<T> *list) : Enumerator() { 
                this->list = list; 
                if (list) {
                    this->version = list->version;
                    this->index = 0;
                }
            }

            inline bool MoveNext() {
                if (!list || index >= list->size) return false;
                current = list->items->m_Items[index];
                index++;
                return true;
            }
            inline void Reset() { index = 0; }
            [[nodiscard]] inline T& Current() { return current; }
        };

        Array<T> *items{};
        int size{};
        int version{};
        void *syncRoot{};

        /**
            @brief Get list data pointer.
            @return List data pointer if list is valid, otherwise null.
        */
        [[nodiscard]] inline T *GetData() const { return items->GetData(); }

        /**
            @brief Get list size.
            @return List size
        */
        [[nodiscard]] inline int GetSize() const { return size; }

        /**
            @brief Get list capacity.
            @return List capacity if list is valid, otherwise zero.
        */
        [[nodiscard]] inline int GetCapacity() const { return items ? items->GetCapacity() : 0; }

        /**
            @brief Get list version.
            @return List version
        */
        [[nodiscard]] inline int GetVersion() const { return version; }

        /**
            @brief Convert list to std::vector.
            @return Vector of elements if list is valid, otherwise empty vector.
        */
        [[nodiscard]] std::vector<T> ToVector() const {
            std::vector<T> ret{};
            BNM_CHECK_SELF(ret);
            for (int i = 0; i < size; i++) ret.push_back(GetData()[i]);
            return ret;
        }

        /**
            @brief Add element to list.
        */
        void Add(T val) {
            GrowIfNeeded(1);
            if (!items || size < 0 || std::cmp_greater_equal(size, items->capacity)) return;
            if (!items->Set(size, val)) return;
            size++;
            version++;
        }

        /**
            @brief Get element index.
        */
        [[nodiscard]] int IndexOf(T val) const {
            for (int i = 0; i < size; i++) if (items->m_Items[i] == val) return i;
            return -1;
        }

        /**
            @brief Remove element at index.
        */
        void RemoveAt(int index) {
            if (index < 0 || index >= size) return;
            Shift(index, -1);
            version++;
        }

        /**
            @brief Remove element.
            @return True if element is removed.
        */
        bool Remove(T val) {
            int i = IndexOf(val);
            if (i == -1) return false;
            RemoveAt(i);
            return true;
        }

        /**
            @brief Resize list.
            @param newCapacity New capacity for target list.
            @return True if newCapacity is bigger than list's capacity.
        */
        bool Resize(int newCapacity) {
            BNM_CHECK_SELF(false);
            if (!items || newCapacity < 0 || std::cmp_less(items->capacity, 0) || std::cmp_less_equal(newCapacity, items->capacity)) return false;
            auto *nItems = NewStorage(newCapacity);
            if (!nItems) return false;
            if (!nItems->CopyRange(items->m_Items, size)) { nItems->Destroy(); return false; }
            auto *previous = items;
            if (!StoreManagedValue(this, &items, nItems)) { nItems->Destroy(); return false; }
            previous->Destroy();
            return true;
        }

        /**
            @brief Get element at index.

            @param index Element index

            @return DataIterator of target element if index isn't out of bounds, otherwise empty DataIterator.
        */
        [[nodiscard]] Utils::DataIterator<T> At(int index) const { if (index < 0 || index >= size) return {}; return items->At(index); }
        Utils::DataIterator<T> operator[] (int index) const { return At(index); }

        /**
            @brief Copy data from vector to current array.
            @param vec Source vector
            @return True if vector size is the same or less than array's size.
        */
        inline bool CopyFrom(const std::vector<T> &vec) { if (vec.size() > INT32_MAX) return false; return CopyFrom(const_cast<T *>(vec.data()), static_cast<int>(vec.size())); }

        /**
            @brief Copy data from pointer to current list.

            @param arr Source pointer
            @param size Source pointer size

            @return True if list is valid.
        */
        bool CopyFrom(T *arr, int arrSize) {
            BNM_CHECK_SELF(false);
            if (!items || arrSize < 0 || (arrSize && !arr)) return false;
            if (static_cast<size_t>(arrSize) > static_cast<size_t>(items->capacity)) {
                auto *nItems = NewStorage(arrSize);
                if (!nItems) return false;
                // Copy before releasing old storage: arr may point inside it.
                if (!nItems->CopyRange(arr, arrSize)) { nItems->Destroy(); return false; }
                auto *previous = items;
                if (!StoreManagedValue(this, &items, nItems)) { nItems->Destroy(); return false; }
                previous->Destroy();
            } else {
                if (!items->CopyRange(arr, arrSize)) return false;
                for (int i = arrSize; i < size; ++i) if (!items->Set(i, T{})) return false;
            }
            size = arrSize;
            ++version;
            return true;
        }

        /**
            @brief Remove all list elements.
        */
        void Clear() {
            for (int i = 0; i < size; ++i) if (!items->Set(i, T{})) return;
            ++version; size = 0;
        }

        /**
            @brief Check if element is in list.

            It's not quite like in C# because of its features.

            @param item Element to check

            @return True if element was found.
        */
        [[nodiscard]] bool Contains(T item) const {
            for (int i = 0; i < size; i++) if (items->m_Items[i] == item) return true;
            return false;
        }
        Enumerator GetEnumerator() { return Enumerator(this); }

        /**
            @brief Get element at index.

            @param index Element index

            @return Element if index isn't out of bounds, otherwise default value.
        */
        [[nodiscard]] T get_Item(int index) const {
            if (index < 0 || index >= size) return {};
            return items->m_Items[index];
        }

        /**
            @brief Set element at index.
            @param index Element index
            @param item New element value
        */
        void set_Item(int index, T item) {
            if (index < 0 || index >= size) return;
            if (!items->Set(index, item)) return;
            ++version;
        }

        /**
            @brief Insert element to index.
            @param index Element index
            @param item New element value
        */
        void Insert(int index, T item) {
            if (!items || index < 0 || index > size) return;
            GrowIfNeeded(1);
            if (std::cmp_greater_equal(size, items->capacity)) return;
            if (index < size && !items->CopyRange(items->m_Items + index, size - index, index + 1)) return;
            if (!items->Set(index, item)) return;
            ++size;
            ++version;
        }

        /// @cond
        void *get_SyncRoot() {
            if (!syncRoot) { auto *value = PRIVATE_MonoListData::CompareExchange4List(syncRoot); (void)StoreManagedValue(this, &syncRoot, value); }
            return syncRoot;
        }
        bool get_false() const { return false; }
        /// @endcond

        /**
            @brief Copy data from current list to some array.
        */
        void CopyTo(Array<T>* arr, int arrIndex) const {
            if (arr && arrIndex >= 0) (void)arr->CopyRange(items->m_Items, size, arrIndex);
        }

        /**
            @brief Check if list has capacity for n elements.
            @param n Number of additional elements
        */
        Array<T> *NewStorage(int capacity) const {
            if (items && items->klass)
                return static_cast<Array<T> *>(__Internal_Array::ArrayFromClass(items->klass->element_class, capacity));
            return Array<T>::Create(static_cast<size_t>(capacity), true);
        }

        void GrowIfNeeded(int n) {
            if (!items || size < 0 || n < 0 || n > INT32_MAX - size || std::cmp_less(items->capacity, 0)) return;
            const auto required = static_cast<size_t>(size + n);
            const auto currentCapacity = static_cast<size_t>(items->capacity);
            if (required > currentCapacity) {
                // currentCapacity < required <= INT32_MAX: arithmetic fits even on 32-bit hosts.
                auto newCap = currentCapacity > 0 ? currentCapacity + currentCapacity / 2 : size_t{4};
                if (newCap < required) newCap = required;
                if (newCap > INT32_MAX) newCap = INT32_MAX;
                Resize(static_cast<int>(newCap));
            }
        }

        /**
            @brief Shift data in list.
            @param start Start index
            @param delta Move delta
        */
        void Shift(int start, int delta) {
            if (!items || size < 0 || start < 0 || start > size) return;
            const auto newSize = static_cast<int64_t>(size) + delta;
            const auto source = static_cast<int64_t>(start) - (delta < 0 ? static_cast<int64_t>(delta) : 0);
            const auto destination = source + delta;
            if (newSize < 0 || newSize > INT32_MAX ||
                static_cast<uint64_t>(newSize) > static_cast<uint64_t>(items->capacity) ||
                source > size || destination < 0) return;
            if (source < size && !items->CopyRange(items->m_Items + source,
                static_cast<IL2CPP::il2cpp_array_size_t>(size - source),
                static_cast<IL2CPP::il2cpp_array_size_t>(destination))) return;
            const auto previousSize = size;
            size = static_cast<int>(newSize);
            for (int i = size; i < previousSize; ++i) (void)items->Set(i, T{});
        }

#ifdef BNM_ALLOW_SELF_CHECKS
        /**
            @brief Check if list isn't null.

            Checks if list is not null and logs if it's null.
            It's used mostly internally in debug builds to crash proof code.

            @return True if list isn't null.
        */
        [[nodiscard]] bool SelfCheck() const {
            if (CheckForNull(this)) return true;
            BNM_LOG_ERR(DBG_BNM_MSG_List_SelfCheck_Error);
            return false;
        }
#endif
        /**
            @brief Create empty list.
        */
        inline constexpr List() : BNM::IL2CPP::Il2CppObject() {
            klass = nullptr;
            monitor = nullptr;
        }
    };

}
// NOLINTEND

#pragma clang diagnostic pop
