#if __cplusplus < 202002L
static_assert(false, "ByNameModding requires C++20 and above!");
#endif

#pragma once

/**
 * @brief Configure the Unity ENGINE version used to build the target game.
 *
 * Use the full engine version, such as 2022.3.50f1. An IL2CPP metadata version
 * (e.g. 29 or 31), the game's own version, or just "Unity 2022" is not enough.
 * `UNITY_VER` is BNM's version selector, NOT the IL2CPP metadata version.
 *
 * Quick setup for a game built with Unity 2022.3.50f1:
 * @code
 * #define UNITY_VER 223
 * #define UNITY_PATCH_VER 50
 * @endcode
 * Edit the existing definitions below; do not add a second active definition.
 * 1. Uncomment exactly one `UNITY_VER` from the table below.
 *    For 2022.3.x, 223 means 22 (year suffix) + 3 (minor).
 *    Unity 6 uses a different code: 6000.0.x -> 6000, 6000.1.x -> 6001, etc.
 *    Older versions have grouped selectors and exceptions; use the table.
 * 2. Set `UNITY_PATCH_VER` to the actual patch: the number after the second dot.
 *    For 2022.3.50f1 it is 50, NOT 3, 1, 33, or the metadata version.
 *    Keep the actual patch even when the selector uses a lower threshold.
 *    The f1/f2/f3 suffix is not part of either setting.
 *
 * | Game's Unity version | UNITY_VER | UNITY_PATCH_VER |
 * |----------------------|-----------|-----------------|
 * | 2022.2.12f1          | 222       | 12              |
 * | 2022.3.50f1          | 223       | 50              |
 * | 2022.3.62f2          | 223       | 62              |
 * | 6000.0.58f2          | 6000      | 58              |
 * | 6000.3.3f1           | 6003      | 3               |
 * | 2021.1.28f1          | 211       | 28              |
 * | 2020.3.19f1          | 202       | 19              |
 * | 2020.3.20f1          | 203       | 20              |
 *
 * @note Why does 223 / 50 include `2022.3.33f1.h` instead of `2022.3.50f1.h`?
 * The bundled files have identical contents within each mapped patch group.
 * `Il2CppHeaders.hpp` includes one representative file per group. In this checkout,
 * `2022.3.33f1.h` and `2022.3.50f1.h` are byte-for-byte identical. This is expected;
 * keep your settings at 223 / 50. Do not rename files or edit the selector to match
 * the game's filename. Matching headers does not mean entire Unity releases are identical.
 *
 * All distinct bundled headers already have a valid selection branch. For example,
 * 2022.3.8 and 2022.3.33 are different and both are selected in their own patch ranges;
 * 2022.3.50 is identical to 2022.3.33 and is covered by that representative.
 * The single files selected for 2023.2, 6000.1 and 6000.4 also cover earlier bundled
 * patches in those branches: their filenames are not minimum patch requirements.
 * See Il2CppHeaders.hpp for the concrete declaration changes at each threshold.
 *
 * @note Older-version exceptions:
 * - Unity 2020.3.0-19 uses `UNITY_VER 202`; 2020.3.20+ uses `UNITY_VER 203`.
 *   `UNITY_PATCH_VER` alone does not switch between these two headers.
 * - Unity 2021.1 uses `UNITY_VER 211` and the actual patch; `2021.1.h` has
 *   internal conditions at patch 24. Other pre-2022 header selections ignore the patch.
 *
 * The table describes the current mapping and bundled versions. Open-ended thresholds
 * such as >=33 are selection rules, not a guarantee for future or unbundled patches.
 * A mapped header or successful build alone does not certify runtime support:
 * new versions also need BNM field-access review and testing in the target game.
 *
 * | UNITY_VER | Header mapping range     | Header file (selected by `UNITY_PATCH_VER`)                                  |
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
 * @note Bundled f2/f3 files match their mapped group; enter the patch number only.
 * @note New Unity minor versions (6000.6+, ...) are not mapped yet - the build will fail
 *       with `static_assert(false, "Setup UNITY_VER in GlobalSettings.hpp")`.
 *
 * Panduan memilih versi ENGINE Unity yang digunakan untuk membangun game:
 * Gunakan versi lengkap, misalnya 2022.3.50f1. Versi metadata IL2CPP (29, 31, dll.)
 * atau versi rilis game tidak cukup. `UNITY_VER` adalah kode pemilih versi milik BNM.
 * 1. Aktifkan tepat satu `UNITY_VER` dari tabel di atas: untuk 2022.3.x gunakan 223.
 *    Angka 223 berasal dari 22 (dua digit akhir tahun) dan 3 (versi minor).
 *    Unity 6 memakai kode berbeda: 6000.0.x -> 6000, 6000.1.x -> 6001, dan seterusnya.
 *    Versi lama memiliki pengelompokan dan pengecualian; ikuti tabel di atas.
 * 2. Isi `UNITY_PATCH_VER` dengan patch game sebenarnya, yaitu angka setelah titik kedua:
 *    untuk 2022.3.50f1 gunakan 50, bukan 3, 1, 33, atau versi metadata IL2CPP.
 *    Akhiran f1/f2/f3 tidak dimasukkan. Edit definisi yang ada; jangan menambah definisi aktif kedua.
 *
 * Konfigurasi 223 / 50 memang memilih `2022.3.33f1.h`: isinya identik dengan
 * `2022.3.50f1.h`. Tetap gunakan patch 50, jangan menggantinya dengan angka batas 33.
 * Tidak perlu mengganti nama file atau mengubah include secara manual.
 * Kesamaan header tidak berarti seluruh isi rilis Unity tersebut sama.
 * Semua header dengan isi berbeda di repo sudah memiliki cabang pemilihan.
 * Contohnya, 2022.3.8 dan 2022.3.33 berbeda dan dipilih untuk rentang patch masing-masing.
 * Untuk 2023.2, 6000.1, dan 6000.4, nama header terpilih bukan batas patch minimum:
 * header patch sebelumnya yang tersedia pada cabang tersebut juga memiliki isi identik.
 * Rincian perubahan pada batas patch dijelaskan di Il2CppHeaders.hpp.
 *
 * Pengecualian versi lama:
 * - Unity 2020.3.0-19 memakai kode 202; mulai 2020.3.20 gunakan kode 203.
 *   Mengubah `UNITY_PATCH_VER` saja tidak mengganti header antara kedua kelompok ini.
 * - Unity 2021.1 memakai kode 211 dan patch sebenarnya; header memiliki kondisi pada patch 24.
 *   Pemilihan header pra-2022 lainnya tidak memakai angka patch.
 * Nilai aktif 222 / 32 di bawah hanya nilai awal; sesuaikan keduanya dengan game kamu.
 * Tabel menjelaskan pemetaan header saat ini. Batas terbuka seperti >=33 tidak menjamin
 * kompatibilitas patch baru atau patch yang belum tersedia di repo.
 * Versi minor yang belum dipetakan, seperti 6000.6+, sengaja gagal dikompilasi melalui static_assert.
 * Header terpilih dan kompilasi yang berhasil belum membuktikan dukungan runtime:
 * versi baru tetap memerlukan pemeriksaan akses field BNM dan pengujian di game target.
 */
// Select one entry below. The active 222 and patch 32 are placeholders; configure both for your game.
// Pilih satu versi di bawah. Ganti nilai aktif 222 dan patch 32 sesuai versi Unity game kamu.
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
//#define UNITY_VER 211 // 2021.1.x
//#define UNITY_VER 212 // 2021.2.x
//#define UNITY_VER 213 // 2021.3.x
//#define UNITY_VER 221 // 2022.1.x
#define UNITY_VER 222 // 2022.2.x
//#define UNITY_VER 223 // 2022.3.x
//#define UNITY_VER 231 // 2023.1.x
//#define UNITY_VER 232 // 2023.2.x
//#define UNITY_VER 6000 // 6000.0.x
//#define UNITY_VER 6001 // 6000.1.x
//#define UNITY_VER 6002 // 6000.2.x
//#define UNITY_VER 6003 // 6000.3.x
//#define UNITY_VER 6004 // 6000.4.x
//#define UNITY_VER 6005 // 6000.5.x

//! Replace with your game's actual patch. 32 is a placeholder, not a detected version.
//! Ganti dengan patch game sebenarnya. 32 hanya nilai awal, bukan hasil deteksi otomatis.
#define UNITY_PATCH_VER 32

//! Aktifkan metode yang sudah deprecated, jika tersedia.
//! Allow to use deprecated methods (if any)
// #define BNM_DEPRECATED

//! Izinkan penggunaan kode sinkronisasi antar-thread.
//! Disarankan dinonaktifkan untuk penggunaan internal.
//! Allow the use of code to synchronize threads
//! It is recommended to disable it during internal use
// #define BNM_ALLOW_MULTI_THREADING_SYNC

//! Untuk System.Collections.Generic.Dictionary (Dictionary).
//! Aktifkan define ini jika game menggunakan .NET 3.5.
//! .NET 3.5 sudah usang, tetapi masih digunakan oleh beberapa game lama.
//! For the System.Collections.Generic.Dictionary (Dictionary)
//! If game uses .NET 3.5 uncomment this define
//! .NET 3.5 is outdated, but some older games use it
// #define BNM_DOTNET35

//! Aktifkan pembuatan class baru dan modifikasi class yang sudah ada.
//! Allow code for creating new classes and modifying existing ones
#define BNM_CLASSES_MANAGEMENT

//! Aktifkan pembuatan coroutine. Memerlukan BNM_CLASSES_MANAGEMENT!
//! Enable coroutine creation code. REQUIRES ClassesManagement!
#define BNM_COROUTINE

//! Nonaktifkan hook otomatis melalui tabel metode virtual di ClassesManagement.
//! Disable auto hook via virtual method table in ClassesManagement
// #define BNM_AUTO_HOOK_DISABLE_VIRTUAL_HOOK

//! Masa-masa indah dahulu...
//! The good old days...
// #define BNM_OLD_GOOD_DAYS

//! Gunakan allocator IL2CPP untuk array Mono sebagai pengganti allocator dasar.
//! Use il2cpp's allocator for Mono arrays instead of basic
#define BNM_USE_IL2CPP_ALLOCATOR

#ifndef NDEBUG

//! Aktifkan metode str() pada struktur.
//! str() methods in structures
#define BNM_ALLOW_STR_METHODS

//! Gunakan signal di IsAllocated.
//! Use signal in IsAllocated
#define BNM_ALLOW_SAFE_IS_ALLOCATED

//! Periksa objek Mono di dalam metodenya.
//! Check mono's objects in their methods
#define BNM_ALLOW_SELF_CHECKS

//! Periksa tipe class saat menetapkan instance objek untuk field dan metode.
//! Check classes when setting an instance to fields and methods
#define BNM_CHECK_INSTANCE_TYPE

#define BNM_DEBUG

#define BNM_INFO

#define BNM_ERROR

#define BNM_WARNING

#endif

//! Tambahkan enkripsi string milikmu.
//! Add your string encryptor
#define BNM_OBFUSCATE(str) str // const char *
//! Data obfuscated using this macro, can be freed, after BNM loaded. Only for advanced users! If you don't know what and how, just use your basic macro here.
//! Data yang dienkripsi dengan macro ini boleh dibebaskan setelah BNM dimuat. Khusus pengguna tingkat lanjut; jika belum memahami cara kerjanya, gunakan macro enkripsi dasarmu.
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

// Sesuaikan jika perlu menyembunyikan pemanggilan dlfcn atau memakai loader sendiri untuk memuat BNM dari luar game.
// If you need to hide dlfcn calls or use your dl to load BNM in the game from the outside
#define BNM_dlopen dlopen
#define BNM_dlsym dlsym
#define BNM_dlclose dlclose
#define BNM_dladdr dladdr

#include <cstdlib>

// Sesuaikan jika perlu menyembunyikan pemanggilan fungsi pengelolaan memori.
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
