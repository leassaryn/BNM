#pragma once

#include <map>
#include <vector>

#include "UserSettings/GlobalSettings.hpp"
#include "Il2CppHeaders.hpp"
#include "BasicMonoStructures.hpp"
#include "Class.hpp"
#include "Method.hpp"
#include "Delegates.hpp"

namespace BNM::Structures::Mono {
    /**
        @brief System.Generic.Dictionary type implementation
        @tparam TKey Key type
        @tparam TValue Value type
    */
    template<typename TKey, typename TValue>
    struct Dictionary : BNM::IL2CPP::Il2CppObject {
#ifdef BNM_DOTNET35
        /**
            @brief System.Generic.Dictionary.Link type implementation
        */
        struct Link { int HashCode{}, Next{}; };
        Array<int> *table{};
        Array<Link> *linkSlots{};
        Array<TKey> *keys{};
        Array<TValue> *values{};
        int touchedSlots{};
        int emptySlot{};
        int count{};
        int threshold{};
        void *hcp{};
        void *serialization_info{};
        int version{};

        /**
            @brief Convert to std::map.
            @return Map of elements
        */
        std::map<TKey, TValue> ToMap() const {
            std::map<TKey, TValue> ret{};
            for (int i = 0; i < touchedSlots; i++)
                if ((linkSlots->m_Items[i].HashCode & -2147483648) != 0)
                    ret.insert(std::make_pair(keys->m_Items[i], values->m_Items[i]));
            return ret;
        }

        /**
            @brief Convert to std::vector.
            @return Vector of keys
        */
        std::vector<TKey> GetKeys() const {
            std::vector<TKey> ret{};
            for (int i = 0; i < touchedSlots; i++)
                if ((linkSlots->m_Items[i].HashCode & -2147483648) != 0)
                    ret.push_back(keys->m_Items[i]);
            return ret;
        }

        /**
            @brief Convert to std::vector.
            @return Vector of values
        */
        std::vector<TValue> GetValues() const {
            std::vector<TValue> ret{};
            for (int i = 0; i < touchedSlots; i++)
                if ((linkSlots->m_Items[i].HashCode & -2147483648) != 0)
                    ret.push_back(values->m_Items[i]);
            return ret;
        }
#else
        /**
            @brief System.Generic.Dictionary.Entry type implementation
        */
        struct Entry {
            int hashCode{}, next{};
            TKey key{};
            TValue value{};
        };
        Array<int> *buckets{};
        Array<Entry> *entries{};
        int count{};
        int version{};
        int freeList{};
        int freeCount{};
        void *comparer{};
        Array<TKey> *keys{};
        Array<TValue> *values{};
        void *syncRoot{};

        /**
            @brief Convert to std::map.
            @return Map of elements
        */
        std::map<TKey, TValue> ToMap() const {
            std::map<TKey, TValue> ret{};
            if (!entries) return ret;
            for (int i = 0; i < count && static_cast<size_t>(i) < entries->capacity; ++i)
                if (entries->m_Items[i].hashCode >= 0) ret.emplace(entries->m_Items[i].key, entries->m_Items[i].value);
            return ret;
        }

        /**
            @brief Convert to std::vector.
            @return Vector of keys
        */
        std::vector<TKey> GetKeys() const {
            std::vector<TKey> ret{};
            if (!entries) return ret;
            for (int i = 0; i < count && static_cast<size_t>(i) < entries->capacity; ++i)
                if (entries->m_Items[i].hashCode >= 0) ret.emplace_back(entries->m_Items[i].key);
            return ret;
        }

        /**
            @brief Convert to std::vector.
            @return Vector of values
        */
        std::vector<TValue> GetValues() const {
            std::vector<TValue> ret{};
            if (!entries) return ret;
            for (int i = 0; i < count && static_cast<size_t>(i) < entries->capacity; ++i)
                if (entries->m_Items[i].hashCode >= 0) ret.emplace_back(entries->m_Items[i].value);
            return ret;
        }
#endif

        /**
            @brief Get dictionary size.
            @return Dictionary entries count
        */
        [[nodiscard]] int GetSize() const {
#ifdef BNM_DOTNET35
            return count;
#else
            return count - freeCount;
#endif
        }

        /**
            @brief Get dictionary version.
            @return Dictionary version
        */
        [[nodiscard]] int GetVersion() const { return version; }

        /**
            @brief Try to get value by key.
            @param key Target key
            @param value Out value
            @return True if value is found by key.
        */
        bool TryGet(TKey key, TValue *value) const { return Class(reinterpret_cast<IL2CPP::Il2CppObject *>(this)).GetMethod(BNM_OBFUSCATE("TryGetValue"), 2).template cast<bool>()[reinterpret_cast<void *>(this)](key, value); }

        /**
            @brief Add value.
            @param key Target key
            @param value Target value
        */
        void Add(TKey key, TValue value) { return Class(reinterpret_cast<IL2CPP::Il2CppObject *>(this)).GetMethod(BNM_OBFUSCATE("Add"), 2).template cast<void>()[reinterpret_cast<void *>(this)](key, value); }

        /**
            @brief Insert value.
            @param key Target key
            @param value Target value
        */
        void Insert(TKey key, TValue value) { return Class(reinterpret_cast<IL2CPP::Il2CppObject *>(this)).GetMethod(BNM_OBFUSCATE("set_Item"), 2).template cast<void>()[reinterpret_cast<void *>(this)](key, value); }

        /**
            @brief Remove value.
            @param key Target key
            @return True if value was removed
        */
        bool Remove(TKey key) { return Class(reinterpret_cast<IL2CPP::Il2CppObject *>(this)).GetMethod(BNM_OBFUSCATE("Remove"), 1).template cast<bool>()[reinterpret_cast<void *>(this)](key); }

        /**
            @brief Check if key exists.
            @param key Target key
            @return True if key exists.
        */
        bool ContainsKey(TKey key) const { auto self = const_cast<Dictionary *>(this); return Class(reinterpret_cast<IL2CPP::Il2CppObject *>(self)).GetMethod(BNM_OBFUSCATE("ContainsKey"), 1).template cast<bool>()[reinterpret_cast<void *>(self)](key); }

        /**
            @brief Check if value exists.
            @param value Target value
            @return True if value exists.
        */
        bool ContainsValue(TValue value) const { auto self = const_cast<Dictionary *>(this); return Class(reinterpret_cast<IL2CPP::Il2CppObject *>(self)).GetMethod(BNM_OBFUSCATE("ContainsValue"), 1).template cast<bool>()[reinterpret_cast<void *>(self)](value); }

        /**
            @brief Clear dictionary.
        */
        void Clear() { return Class(reinterpret_cast<IL2CPP::Il2CppObject *>(this)).GetMethod(BNM_OBFUSCATE("Clear"), 0).template cast<void>()[reinterpret_cast<void *>(this)](); }

        /**
            @brief Get value by key.
            @return Value if it's found, otherwise default value.
        */
        TValue Get(TKey key) const {
            TValue ret;
            if (TryGet(key, &ret)) return ret;
            return {};
        }
        
        /**
            @brief Get value by key.

            Alias of Get().

            @return Value if it's found, otherwise default value.
        */
        TValue operator[](TKey key) const { return Get(key); }
    };
}