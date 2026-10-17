#if __cplusplus < 202002L
static_assert(false, "ByNameModding requires C++20 and above!");
#endif

#pragma once

/**
 * @brief Unity version selection (uncomment exactly one).
 *
 * 1. Uncomment the `#define UNITY_VER` matching your game's Unity version.
 * 2. If your game is 2022.x or newer (including Unity 6 / 6000.x), set
 *    `#define UNITY_PATCH_VER` to your game's patch number
 *    (example: 2022.3.20f1 -> 20, 6000.0.25f1 -> 25).
 *
 * @note How the Il2Cpp header is chosen:
 * - Unity < 2022: one header per minor version. All released f builds of that minor
 *   share the same runtime layout, so `UNITY_PATCH_VER` is not needed
 *   (only exception: 2021.1 - set `UNITY_PATCH_VER` to 24 for 2021.1.24f1 and above).
 * - Unity >= 2022 and Unity 6 (6000.x): full per-patch headers. The runtime layout
 *   changed in a few patches, so set `UNITY_PATCH_VER` to your game's patch.
 *   The table below shows the covered range and which `Il2CppHeaders` file is included.
 *
 * | UNITY_VER | Supported range          | Header file (selected by `UNITY_PATCH_VER`)                                  |
 * |-----------|--------------------------|-------------------------------------------------------------------------------|
 * | 56        | 5.6.x (5.6.0f1 - 5.6.4f1) | 5.6.4f1.h                                                                    |
 * | 171       | 2017.1.x                 | 2017.1.h                                                                      |
 * | 172       | 2017.2.x - 2017.4.x      | 2017.4.h                                                                      |
 * | 181       | 2018.1.x                 | 2018.1.h                                                                      |
 * | 182       | 2018.2.x                 | 2018.2.h                                                                      |
 * | 183       | 2018.3.x - 2018.4.x      | 2018.4.h                                                                      |
 * | 191       | 2019.1.x - 2019.2.x      | 2019.2.h                                                                      |
 * | 193       | 2019.3.x                 | 2019.3.h                                                                      |
 * | 194       | 2019.4.x                 | 2019.4.h                                                                      |
 * | 201       | 2020.1.x                 | 2020.1.h                                                                      |
 * | 202       | 2020.2.x - 2020.3.19f1   | 2020.2.h                                                                      |
 * | 203       | 2020.3.20f1 - 2020.3.x   | 2020.3.h                                                                      |
 * | 211       | 2021.1.x                 | 2021.1.h (internal switch at patch 24: <24 / >=24)                            |
 * | 212       | 2021.2.x                 | 2021.2.h                                                                      |
 * | 213       | 2021.3.x                 | 2021.3.h                                                                      |
 * | 221       | 2022.1.0f1 - 2022.1.25f1  | 2022.1.0f1.h (<8) or 2022.1.8f1.h (>=8)                                     |
 * | 222       | 2022.2.0f1 - 2022.2.23f1  | 2022.2.0f1.h (<9) or 2022.2.9f1.h (>=9)                                     |
 * | 223       | 2022.3.0f1 - 2022.3.76f1  | 2022.3.0f1.h (<8), 2022.3.8f1.h (8-32) or 2022.3.33f1.h (>=33)              |
 * | 231       | 2023.1.0f1 - 2023.1.22f1  | 2023.1.0f1.h (<10) or 2023.1.10f1.h (>=10)                                  |
 * | 232       | 2023.2.x                 | 2023.2.22f1.h                                                                 |
 * | 6000      | 6000.0.0f1 - 6000.0.79f1  | 6000.0.0f1.h (<10), 6000.0.10f1.h (10-43), 6000.0.44f1.h (44-56) or 6000.0.57f1.h (>=57) |
 * | 6001      | 6000.1.x                 | 6000.1.17f1.h                                                                 |
 * | 6002      | 6000.2.0f1 - 6000.2.15f1  | 6000.2.0f1.h (<2) or 6000.2.2f1.h (>=2)                                     |
 * | 6003      | 6000.3.0f1 - 6000.3.19f1  | 6000.3.0f1.h (<3) or 6000.3.3f1.h (>=3, `Il2CppClass` uses the `init_data` union) |
 * | 6004      | 6000.4.x                 | 6000.4.12f1.h                                                                 |
 * | 6005      | 6000.5.x                 | 6000.5.0f1.h (<1) or 6000.5.1f1.h (>=1)                                      |
 *
 * @note f2/f3 hotfix builds (e.g. 2022.3.62f2, 6000.0.58f2) use the same layout as the
 *       f1 range they belong to - just set `UNITY_PATCH_VER` to their patch number.
 * @note Unity 6 versions are 6000.x: `UNITY_VER` 6000 = 6000.0.x, 6001 = 6000.1.x, and so on.
 * @note New Unity minor versions (6000.6+, ...) are not mapped yet - the build will fail
 *       with `static_assert(false, "Setup UNITY_VER in GlobalSettings.hpp")`.
 *
 * Выбор версии Unity (раскомментируйте ровно один вариант):
 * 1. Раскомментируйте `#define UNITY_VER`, соответствующий версии Unity в игре.
 * 2. Если игра на 2022.x или новее (включая Unity 6 / 6000.x), установите
 *    `#define UNITY_PATCH_VER` в номер патча игры (например: 2022.3.20f1 -> 20).
 *
 * Таблица диапазонов и выбираемых файлов `Il2CppHeaders` приведена выше.
 */
//#define UNITY_VER 56 // 5.6.4f1
//#define UNITY_VER 171 // 2017.1.x
//#define UNITY_VER 172 // 2017.2.x - 2017.4.x
//#define UNITY_VER 181 // 2018.1.x
//#define UNITY_VER 182 // 2018.2.x
//#define UNITY_VER 183 // 2018.3.x - 2018.4.x
//#define UNITY_VER 191 // 2019.1.x - 2019.2.x
//#define UNITY_VER 193 // 2019.3.x
//#define UNITY_VER 194 // 2019.4.x
//#define UNITY_VER 201 // 2020.1.x
//#define UNITY_VER 202 // 2020.2.x - 2020.3.19
//#define UNITY_VER 203 // 2020.3.20 - 2020.3.xx
//#define UNITY_VER 211 // 2021.1.x (Need set UNITY_PATCH_VER to 24 if x (2021.1.x) >= 24)
//#define UNITY_VER 212 // 2021.2.x
//#define UNITY_VER 213 // 2021.3.x
//#define UNITY_VER 221 // 2022.1.x (Need set UNITY_PATCH_VER to 8 if 2022.1.x >= 8)
#define UNITY_VER 222 // 2022.2.x (Need set UNITY_PATCH_VER to 9 if 2022.2.x >= 9)
//#define UNITY_VER 223 // 2022.3.x (Need set UNITY_PATCH_VER to 8 if 2022.3.x >= 8, or to 33 if 2022.3.x >= 33)
//#define UNITY_VER 231 // 2023.1.x (Need set UNITY_PATCH_VER to 10 if 2023.1.x >= 10)
//#define UNITY_VER 232 // 2023.2.x
//#define UNITY_VER 6000 // 6000.0.x (Need set UNITY_PATCH_VER to 10 if 6000.0.x >= 10, or to 44 if >= 44, or to 57 if >= 57)
//#define UNITY_VER 6001 // 6000.1.x
//#define UNITY_VER 6002 // 6000.2.x (Need set UNITY_PATCH_VER to 2 if 6000.2.x >= 2)
//#define UNITY_VER 6003 // 6000.3.x (Need set UNITY_PATCH_VER to 3 if 6000.3.x >= 3)
//#define UNITY_VER 6004 // 6000.4.x
//#define UNITY_VER 6005 // 6000.5.x (Need set UNITY_PATCH_VER to 1 if 6000.5.x >= 1)

#define UNITY_PATCH_VER 32 // Для особых случаев (For special cases)

//! Включить устаревший код (если есть)
//! Allow to use deprecated methods (if any)
// #define BNM_DEPRECATED

//! Разрешить использование кода для синхронизации потоков
//! Рекомендуется отключать при внутреннем использовании
//! Allow the use of code to synchronize streams
//! It is recommended to disable it during internal use
// #define BNM_ALLOW_MULTI_THREADING_SYNC

//! Для System.Collections.Generic.Dictionary (Dictionary)
//! Если игра использует .NET 3.5 раскомментируйте этот define
//! .NET 3.5 устарел, но часть старых игр используют его
//! For the System.Collections.Generic.Dictionary (Dictionary)
//! If game uses .NET 3.5 uncomment this define
//! .NET 3.5 is outdated, but some older games use it
// #define BNM_DOTNET35

//! Включить код создания новых классов и модификации существующих
//! Allow code for creating new classes and modifying existing ones
#define BNM_CLASSES_MANAGEMENT

//! Включить код создания coroutine. ТРЕБУЕТСЯ ClassesManagement!
//! Enable coroutine creation code. REQUIRES ClassesManagement!
#define BNM_COROUTINE

//! Отключить авто подмену через таблицу виртуальных методов в ClassesManagement
//! Disable auto hook via virtual method table in ClassesManagement
// #define BNM_AUTO_HOOK_DISABLE_VIRTUAL_HOOK

//! Старые добрые времена...
//! The good old days...
// #define BNM_OLD_GOOD_DAYS

//! Использовать аллокатор il2cpp для Mono массивов вместо базового
//! Use il2cpp's allocator for Mono arrays instead of basic
#define BNM_USE_IL2CPP_ALLOCATOR

#ifndef NDEBUG

//! Методы str() в структурах
//! str() methods in structures
#define BNM_ALLOW_STR_METHODS

//! Использовать signal в IsAllocated
//! Use signal in IsAllocated
#define BNM_ALLOW_SAFE_IS_ALLOCATED

//! Проверять объекты mono в их методах
//! Check mono's objects in their methods
#define BNM_ALLOW_SELF_CHECKS

//! Проверять классы при установке объекта полям и методам
//! Check classes when setting an instance to fields and methods
#define BNM_CHECK_INSTANCE_TYPE

#define BNM_DEBUG

#define BNM_INFO

#define BNM_ERROR

#define BNM_WARNING

#endif

//! Добавьте ваш шифровщик строк
//! Add your string encryptor
#define BNM_OBFUSCATE(str) str // const char *
//! Data obfuscated using this macro, can be freed, after BNM loaded. Only for advanced users! If you don't know what and how, just use your basic macro here.
//! Данные, защифрованне этим define, могут быть удалены после загрузки BNM. Только для опытных пользователей! Если вы не знаете, что и как, просто используйте свой базовый define.
#define BNM_OBFUSCATE_TMP(str) str // const char *

// Shadowhook
/*
#include <shadowhook.h>
template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T ptr, NEW_T newMethod, T_OLD &oldBytes) {
    if (reinterpret_cast<void *>(ptr) != nullptr) return shadowhook_hook_func_addr(reinterpret_cast<void *>(ptr), reinterpret_cast<void *>(newMethod), reinterpret_cast<void **>(&oldBytes));
    return nullptr;
}

template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T ptr, NEW_T newMethod, T_OLD &&oldBytes) {
    if (reinterpret_cast<void *>(ptr) != nullptr) return shadowhook_hook_func_addr(reinterpret_cast<void *>(ptr), reinterpret_cast<void *>(newMethod), reinterpret_cast<void **>(&oldBytes));
    return nullptr;
}

template<typename PTR_T>
inline void Unhook(PTR_T ptr) {
    if (reinterpret_cast<void *>(ptr) != nullptr) shadowhook_unhook(reinterpret_cast<void *>(ptr));
}
*/


// Dobby
/*
#include <dobby.h>

template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T ptr, NEW_T newMethod, T_OLD &oldBytes) {
    if (!reinterpret_cast<void *>(ptr) || !reinterpret_cast<void *>(newMethod)) return nullptr;
    if (DobbyHook(reinterpret_cast<void *>(ptr), reinterpret_cast<void *>(newMethod), reinterpret_cast<void **>(&oldBytes)) != 0) return nullptr;
    return reinterpret_cast<void *>(ptr);
}

template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T ptr, NEW_T newMethod, T_OLD &&oldBytes) {
    return BasicHook(ptr, newMethod, oldBytes);
}

template<typename PTR_T>
inline void Unhook(PTR_T ptr) {
    if (reinterpret_cast<void *>(ptr) != nullptr) DobbyDestroy(reinterpret_cast<void *>(ptr));
}
*/

// Dummy
#include <cassert>

#error "No hooking backend selected! Uncomment Dobby or ShadowHook in GlobalSettings.hpp above."

template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T, NEW_T, T_OLD &) {
    return nullptr;
}

template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T, NEW_T, T_OLD &&) {
    return nullptr;
}

template<typename PTR_T>
inline void Unhook(PTR_T) {
}

#include <dlfcn.h>

// Если вам нужно скрыть вызовы dlfcn или использовать ваш dl для загрузки BNM в игре извне
// If you need to hide dlfcn calls or use your dl to load BNM in the game from the outside
#define BNM_dlopen dlopen
#define BNM_dlsym dlsym
#define BNM_dlclose dlclose
#define BNM_dladdr dladdr

#include <cstdlib>

// Если вам нужно скрыть методы работы с памятью
// If you need to hide memory management methods
#define BNM_malloc malloc
#define BNM_free free

#include <android/log.h>

#define BNM_TAG "ByNameModding"

#ifdef BNM_ALLOW_SELF_CHECKS
#define BNM_CHECK_SELF(returnValue) if (!SelfCheck()) return returnValue
#else
#define BNM_CHECK_SELF(returnValue) ((void)0)
#endif

#ifdef BNM_INFO
#define BNM_LOG_INFO(...) ((void)__android_log_print(4, BNM_TAG, __VA_ARGS__))
#else
#define BNM_LOG_INFO(...) ((void)0)
#endif

#ifdef BNM_DEBUG
#define BNM_LOG_DEBUG(...) ((void)__android_log_print(3, BNM_TAG, __VA_ARGS__))
#define BNM_LOG_DEBUG_IF(condition, ...) if (condition) ((void)__android_log_print(3, BNM_TAG, __VA_ARGS__))
#else
#define BNM_LOG_DEBUG(...) ((void)0)
#define BNM_LOG_DEBUG_IF(...) ((void)0)
#endif

#ifdef BNM_ERROR
#define BNM_LOG_ERR(...) ((void)__android_log_print(6, BNM_TAG, __VA_ARGS__))
#define BNM_LOG_ERR_IF(condition, ...) if (condition) ((void)__android_log_print(6, BNM_TAG, __VA_ARGS__))
#else
#define BNM_LOG_ERR(...) ((void)0)
#define BNM_LOG_ERR_IF(condition, ...) ((void)0)
#endif

#ifdef BNM_WARNING
#define BNM_LOG_WARN(...) ((void)__android_log_print(5, BNM_TAG, __VA_ARGS__))
#define BNM_LOG_WARN_IF(condition, ...) if (condition) ((void)__android_log_print(5, BNM_TAG, __VA_ARGS__))
#else
#define BNM_LOG_WARN(...) ((void)0)
#define BNM_LOG_WARN_IF(condition, ...) ((void)0)
#endif

namespace BNM {
#if defined(__LP64__)
    using BNM_INT_PTR = long;
    using BNM_PTR = unsigned long;
#else
    using BNM_INT_PTR = int;
    using BNM_PTR = unsigned int;
#endif
}

#define BNM_VER "2.5.5-rynshy"
