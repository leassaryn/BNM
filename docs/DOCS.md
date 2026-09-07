# ByNameModding (BNM) — Dokumentasi Lengkap

> BNM adalah library C++20 statik untuk modding game Unity (il2cpp) di Android.
> Versi library saat ini: **2.5.3** (`BNM_VER` di `include/BNM/UserSettings/GlobalSettings.hpp`)

---

## Daftar Isi

1. [Prasyarat](#1-prasyarat)
2. [Setup Proyek](#2-setup-proyek)
3. [Pemilihan Versi Unity](#3-pemilihan-versi-unity)
4. [Konfigurasi Hook Backend](#4-konfigurasi-hook-backend)
5. [Cara Memuat (Loading) BNM](#5-cara-muat-loading-bnm)
6. [API Inti: BNM::Class](#6-api-inti-bnmclass)
7. [Field — Membaca/Menulis Field](#7-field--membacamenulis-field)
8. [Method — Memanggil Method](#8-method--memanggil-method)
9. [Property — Getter/Setter](#9-property--gettersetter)
10. [Event — Menambah/Menghapus/Menaikkan Event](#10-event--menambahmenghapuspendongkalevent)
11. [Image — Bekerja dengan Assembly](#11-image--bekerja-dengan-assembly)
12. [Mono Structures — String, Array, List](#12-mono-structures--string-array-list)
13. [Unity Structures — Vector, Color, dll.](#13-unity-structures--vector-color-dll)
14. [Defaults — Mapping Tipe C++ ke C#](#14-defaults--mapping-tipe-c-c-ke-c)
15. [CompileTimeClass — Referensi Tipe di Waktu Kompilasi](#15-compiletimeclass--referensi-tipe-di-waktu-kompilasi)
16. [ClassesManagement — Membuat/Memodifikasi Kelas di Runtime](#16-classesmanagement--membuatmemodifikasi-kelas-di-runtime)
17. [Coroutine — Koroutine C++20](#17-coroutine--koroutine-c20)
18. [Delegates — Action dan MulticastDelegate](#18-delegates--action-dan-multicastdelegate)
19. [Hooking — Cara Hook Method](#19-hooking--cara-hook-method)
20. [Utilitas Tambahan](#20-utilitas-tambahan)
21. [Trouble Shooting](#21-trouble-shooting)

---

## 1. Prasyarat

| Komponen | Keterangan |
|---|---|
| **Android NDK** | Dibutuhkan untuk mengkompilasi. Gunakan `aarch64-linux-android-clang++` (NDK r25+ direkomendasikan). |
| **C++20** | BNM menggunakan fitur C++20 (`std::string_view`, `constexpr`, `if constexpr`, dll). |
| **Hook Backend** | Salah satu: [Dobby](https://github.com/jmpews/Dobby), [ShadowHook](https://github.com/bytedance/android-inline-hook), atau Substrate. |
| **Target** | Game Unity yang menggunakan **il2cpp** (bukan Mono). |

---

## 2. Setup Proyek

### 2.1 Struktur Direktori

```
BNM-Android/
├── include/
│   ├── BNM/
│   │   ├── UserSettings/
│   │   │   ├── GlobalSettings.hpp        ← KONFIGURASI UTAMA
│   │   │   └── Il2CppMethodNames.hpp     ← Nama-nama API il2cpp
│   │   ├── Il2CppHeaders.hpp             ← Pemilih header versi il2cpp
│   │   ├── Il2CppHeaders/                ← Header per versi Unity
│   │   ├── Il2CppApi/                    ← Daftar API per versi (referensi saja)
│   │   ├── UnityStructures/              ← Vector, Color, Quaternion, dll.
│   │   ├── Class.hpp
│   │   ├── Field.hpp / FieldBase.hpp
│   │   ├── Method.hpp / MethodBase.hpp
│   │   ├── Property.hpp / PropertyBase.hpp
│   │   ├── Event.hpp / EventBase.hpp
│   │   ├── Image.hpp
│   │   ├── Loading.hpp
│   │   ├── Defaults.hpp
│   │   ├── Delegates.hpp
│   │   ├── Coroutine.hpp
│   │   ├── ClassesManagement.hpp
│   │   ├── BasicMonoStructures.hpp       ← String, Array, List
│   │   ├── Utils.hpp
│   │   └── UnityStructures.hpp
│   └── external/
│       ├── include/utf8.h
│       └── utf8/                         ← UTF8-CPP library
├── src/                                  ← Implementasi (.cpp)
│   └── private/Internals.hpp             ← Internal, jangan diinclude langsung
├── Android.mk                            ← Build ndk-build
└── CMakeLists.txt                        ← Build CMake
```

### 2.2 Menambahkan BNM ke Proyek Anda

**Menggunakan ndk-build (Android.mk):**

Tambahkan di `Android.mk` proyek Anda:

```makefile
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# ... konfigurasi modul Anda ...

# Arah ke BNM (adjust sesuai lokasi)
BNM_PATH := $(LOCAL_PATH)/path/to/BNM-Android

LOCAL_CPPFLAGS += -std=c++20

LOCAL_STATIC_LIBRARIES += BNM

include $(BUILD_SHARED_LIBRARY)  # atau BUILD_EXECUTABLE

$(call import-module, path/to/BNM-Android)
```

**Menggunakan CMake:**

```cmake
add_subdirectory(path/to/BNM-Android)

target_link_libraries(your_target PRIVATE BNM)
target_compile_features(your_target PRIVATE cxx_std_20)
```

### 2.3 Include Header

Include header utama BNM di file C++ Anda:

```cpp
#include <BNM/UserSettings/GlobalSettings.hpp>  // Selalu include ini
#include <BNM/Class.hpp>     // Untuk mencari dan bekerja dengan kelas
#include <BNM/Field.hpp>     // Untuk field
#include <BNM/Method.hpp>    // Untuk method
#include <BNM/Loading.hpp>   // Untuk memuat BNM
#include <BNM/Defaults.hpp>  // Untuk mapping tipe
```

---

## 3. Pemilihan Versi Unity

**Ini adalah langkah paling penting.** BNM harus dikonfigurasi sesuai versi Unity target.

Buka `include/BNM/UserSettings/GlobalSettings.hpp` dan:

1. **Uncomment** tepat satu `UNITY_VER` yang sesuai.
2. Untuk Unity **2022.x atau lebih baru** (termasuk Unity 6 / 6000.x), juga set `UNITY_PATCH_VER` ke nomor patch game.

### Tabel Pemilihan Versi

| UNITY_VER | Rentang yang Didukung | Catatan Patch |
|---|---|---|
| `56` | 5.6.x (5.6.0f1 - 5.6.4f1) | Tidak perlu patch |
| `171` | 2017.1.x | Tidak perlu patch |
| `172` | 2017.2.x - 2017.4.x | Tidak perlu patch |
| `181` | 2018.1.x | Tidak perlu patch |
| `182` | 2018.2.x | Tidak perlu patch |
| `183` | 2018.3.x - 2018.4.x | Tidak perlu patch |
| `191` | 2019.1.x - 2019.2.x | Tidak perlu patch |
| `193` | 2019.3.x | Tidak perlu patch |
| `194` | 2019.4.x | Tidak perlu patch |
| `201` | 2020.1.x | Tidak perlu patch |
| `202` | 2020.2.x - 2020.3.19f1 | Tidak perlu patch |
| `203` | 2020.3.20f1 - 2020.3.x | Tidak perlu patch |
| `211` | 2021.1.x | Patch ≥24: set `UNITY_PATCH_VER 24` |
| `212` | 2021.2.x | Tidak perlu patch |
| `213` | 2021.3.x | Tidak perlu patch |
| `221` | 2022.1.x | Patch ≥8: set `UNITY_PATCH_VER 8` |
| `222` | 2022.2.x | Patch ≥9: set `UNITY_PATCH_VER 9` |
| `223` | 2022.3.x | Patch <8, 8-32, ≥33 → set `UNITY_PATCH_VER` sesuai |
| `231` | 2023.1.x | Patch ≥10: set `UNITY_PATCH_VER 10` |
| `232` | 2023.2.x | Set `UNITY_PATCH_VER 22` |
| `6000` | 6000.0.x | Patch ≥10/44/57: set `UNITY_PATCH_VER` sesuai |
| `6001` | 6000.1.x | Set `UNITY_PATCH_VER 17` |
| `6002` | 6000.2.x | Patch ≥2: set `UNITY_PATCH_VER 2` |
| `6003` | 6000.3.x | Patch ≥3: set `UNITY_PATCH_VER 3` |
| `6004` | 6000.4.x | Set `UNITY_PATCH_VER 12` |
| `6005` | 6000.5.x | Patch ≥1: set `UNITY_PATCH_VER 1` |

**Contoh:**
```cpp
// Game Unity 2022.3.20f1 → UNITY_VER=223, UNITY_PATCH_VER=20
#define UNITY_VER 223
#define UNITY_PATCH_VER 20
```

```cpp
// Game Unity 6000.0.50f1 → UNITY_VER=6000, UNITY_PATCH_VER=50
// 50 masuk range 44-56, jadi pakai header 6000.0.44f1.h
#define UNITY_VER 6000
#define UNITY_PATCH_VER 50
```

> **Penting:** Build akan gagal dengan `static_assert(false, "Setup UNITY_VER in GlobalSettings.hpp")` jika versi tidak dikenali.

---

## 4. Konfigurasi Hook Backend

BNM memerlukan software hooking. Di `GlobalSettings.hpp`, bagian akhir file, ada tiga blok yang dikomentari. **Uncomment salah satu**:

### Dobby (Direkomendasikan)

```cpp
// Dobby
#include <dobby.h>

template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T ptr, NEW_T newMethod, T_OLD &oldBytes) {
    if ((void *) ptr != nullptr) DobbyHook((void *)ptr, (void *) newMethod, (void **) &oldBytes);
    return (void *) ptr;
}

template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T ptr, NEW_T newMethod, T_OLD &&oldBytes) {
    if ((void *) ptr != nullptr) DobbyHook((void *)ptr, (void *) newMethod, (void **) &oldBytes);
    return (void *) ptr;
}

template<typename PTR_T>
inline void Unhook(PTR_T ptr) {
    if ((void *) ptr != nullptr) DobbyDestroy((void *)ptr);
}
```

### ShadowHook

```cpp
// Shadowhook
template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T ptr, NEW_T newMethod, T_OLD &oldBytes) {
    if ((void *) ptr != nullptr) return shadowhook_hook_func_addr((void *)ptr, (void *) newMethod, (void **) &oldBytes);
    return nullptr;
}

template<typename PTR_T, typename NEW_T, typename T_OLD>
inline void *BasicHook(PTR_T ptr, NEW_T newMethod, T_OLD &&oldBytes) {
    if ((void *) ptr != nullptr) return shadowhook_hook_func_addr((void *)ptr, (void *) newMethod, (void **) &oldBytes);
    return nullptr;
}

template<typename PTR_T>
inline void Unhook(PTR_T ptr) {
    if ((void *) ptr != nullptr) shadowhook_unhook((void *)ptr);
}
```

> **Peringatan:** Bagian "Dummy" (dengan `static_assert(false, "No hooking software!")`) harus dikomentari atau dihapus. Tanpa hook backend yang valid, BNM tidak akan dikompilasi.

### Flag Tambahan di GlobalSettings.hpp

| Flag | Default | Keterangan |
|---|---|---|
| `BNM_CLASSES_MANAGEMENT` | ✅ Aktif | Mengaktifkan pembuatan/modifikasi kelas di runtime |
| `BNM_COROUTINE` | ✅ Aktif | Mengaktifkan koroutine C++20. **Memerlukan** `BNM_CLASSES_MANAGEMENT` |
| `BNM_USE_IL2CPP_ALLOCATOR` | ✅ Aktif | Gunakan alokator il2cpp untuk array Mono |
| `BNM_DEPRECATED` | ❌ Nonaktif | Mengaktifkan method usang (jika ada) |
| `BNM_DOTNET35` | ❌ Nonaktif | Untuk game yang pakai .NET 3.5 (Dictionary) |
| `BNM_ALLOW_MULTI_THREADING_SYNC` | ❌ Nonaktif | Synchronisasi thread (nonaktifkan untuk penggunaan internal) |

Debug flags (`BNM_DEBUG`, `BNM_INFO`, `BNM_ERROR`, `BNM_WARNING`, `BNM_ALLOW_SELF_CHECKS`, `BNM_CHECK_INSTANCE_TYPE`, `BNM_ALLOW_STR_METHODS`, `BNM_ALLOW_SAFE_IS_ALLOCATED`) aktif otomatis dalam **debug build** (`#ifndef NDEBUG`).

---

## 5. Cara Memuat (Loading) BNM

BNM harus dimuat sebelum Anda bisa menggunakannya. Ada beberapa metode:

### 5.1 Menggunakan JNI (Paling Umum)

```cpp
#include <jni.h>
#include <BNM/Loading.hpp>

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);

    // Memuat BNM menggunakan JNI — akan mencari libil2cpp.so secara otomatis
    BNM::Loading::TryLoadByJNI(env);

    return JNI_VERSION_1_6;
}
```

`TryLoadByJNI` akan:
- Mendapatkan path ke `libil2cpp.so` dari Context aplikasi
- Membuka library tersebut dengan `dlopen`
- Hook `il2cpp_init` untuk memuat BNM dari thread il2cpp

### 5.2 Menggunakan dlfcn Handle

Jika Anda sudah memiliki handle ke `libil2cpp.so`:

```cpp
#include <BNM/Loading.hpp>

void MyLoadFunction() {
    auto handle = BNM_dlopen("/path/to/libil2cpp.so", RTLD_LAZY);
    BNM::Loading::TryLoadByDlfcnHandle(handle);
}
```

### 5.3 Menggunakan Custom Method Finder

Jika dlfcn atau JNI tidak berfungsi di konteks Anda:

```cpp
#include <BNM/Loading.hpp>

void *MyMethodFinder(const char *name, void *userData) {
    // Implementasi pencarian method Anda sendiri
    return nullptr;
}

void MyLoadFunction() {
    BNM::Loading::SetMethodFinder(MyMethodFinder, myUserData);
    BNM::Loading::TryLoadByUsersFinder();
}
```

### 5.4 Setup Langsung (Tanpa Hook il2cpp_init)

**Peringatan:** Il2cpp harus sudah dimuat sepenuhnya sebelum memanggil ini. Bisa crash jika dipanggil terlalu awal.

```cpp
void MyLoadFunction() {
    BNM::Loading::SetMethodFinder(MyMethodFinder, myUserData);
    BNM::Loading::TrySetupByUsersFinder();  // Langsung muat dari thread ini
}
```

### 5.5 Late Init Hook

Untuk game yang memulai il2cpp sangat lambat:

```cpp
// Panggil SEBELUM TryLoad*
BNM::Loading::AllowLateInitHook();

// Lalu muat seperti biasa
BNM::Loading::TryLoadByJNI(env);
```

### 5.6 Event Setelah Loaded

Daftarkan callback yang dipanggil setelah BNM dan il2cpp selesai dimuat:

```cpp
void OnBNMLoaded() {
    // BNM sudah siap digunakan di sini
    auto cls = BNM::Class("UnityEngine", "GameObject");
    // ...
}

BNM::Loading::AddOnLoadedEvent(OnBNMLoaded);
```

---

## 6. API Inti: BNM::Class

`BNM::Class` adalah kelas pusat untuk berinteraksi dengan il2cpp classes.

### 6.1 Membuat Class

```cpp
#include <BNM/Class.hpp>

// Dari namespace + nama (cari di semua assembly)
BNM::Class cls("UnityEngine", "GameObject");

// Dari namespace + nama + image tertentu (lebih cepat)
BNM::Image img("Assembly-CSharp.dll");
BNM::Class cls2("MyNamespace", "MyClass", img);

// Dari Il2CppClass*
BNM::Class cls3(someIl2CppClass);

// Dari objek il2cpp
BNM::Class cls4(someIl2CppObject);

// Dari CompileTimeClass
BNM::Class cls5 = myCompileTimeClass;
```

### 6.2 Validasi

```cpp
if (cls.IsValid()) {   // atau cls.Alive() atau if(cls)
    // Class ditemukan dan valid
}
```

### 6.3 Mendapatkan Anggota

```cpp
// Semua fields (termasuk parent jika includeParent=true)
std::vector<BNM::FieldBase> fields = cls.GetFields(true);

// Semua methods
std::vector<BNM::MethodBase> methods = cls.GetMethods(true);

// Semua properties
std::vector<BNM::PropertyBase> props = cls.GetProperties(true);

// Semua events
std::vector<BNM::EventBase> events = cls.GetEvents(true);

// Semua inner classes
std::vector<BNM::Class> innerClasses = cls.GetInnerClasses(true);
```

### 6.4 Mencari Anggota Tertentu

```cpp
// Mencari method berdasarkan nama dan jumlah parameter
BNM::MethodBase method = cls.GetMethod("TakeDamage", 2);

// Mencari method berdasarkan nama dan nama parameter
BNM::MethodBase method2 = cls.GetMethod("TakeDamage", {"damage", "attacker"});

// Mencari method berdasarkan nama dan tipe parameter
BNM::MethodBase method3 = cls.GetMethod("TakeDamage", {
    BNM::Defaults::Get<float>(),
    BNM::Defaults::Get<BNM::UnityEngine::Object *>()
});

// Mencari field
BNM::FieldBase field = cls.GetField("health");

// Mencari property
BNM::PropertyBase prop = cls.GetProperty("IsAlive");

// Mencari event
BNM::EventBase evt = cls.GetEvent("OnDeath");

// Mencari inner class
BNM::Class inner = cls.GetInnerClass("NestedClass");
```

### 6.5 Hierarki Kelas

```cpp
BNM::Class parent = cls.GetParent();     // Parent class
BNM::Class arr = cls.GetArray();          // Tipe array (cls[])
BNM::Class ptr = cls.GetPointer();        // Tipe pointer (cls*)
BNM::Class ref = cls.GetReference();      // Tipe reference (cls&)
BNM::Class generic = cls.GetGeneric({    // Tipe generic
    BNM::Defaults::Get<int>(),
    BNM::Defaults::Get<float>()
});
```

### 6.6 Informasi Tipe

```cpp
BNM::IL2CPP::Il2CppType *il2cppType = cls.GetIl2CppType();
BNM::MonoType *monoType = cls.GetMonoType();
BNM::IL2CPP::Il2CppClass *il2CppClass = cls.GetClass();
BNM::CompileTimeClass ctc = cls.GetCompileTimeClass();
BNM::Image image = cls.GetImage();
```

### 6.7 Membuat Instance

```cpp
// Membuat instance tanpa constructor (new object() tanpa .ctor)
BNM::IL2CPP::Il2CppObject *obj = cls.CreateNewInstance();

// Membuat instance dengan constructor berdasarkan jumlah parameter
auto obj2 = cls.CreateNewObjectParameters(10.0f, "hello");

// Membuat instance dengan constructor berdasarkan nama parameter
auto obj3 = cls.CreateNewObjectTypes({"value", "name"}, 10.0f, "hello");
```

### 6.8 Membuat Array dan List

```cpp
// Membuat array
auto *arr = cls.NewArray<BNM::IL2CPP::Il2CppObject *>(10);

// Membuat List<T>
auto *list = cls.NewList<int>();

// Membuat List<T> menggunakan BNM (untuk tipe yang tidak ada di game)
auto *list2 = cls.NewListBNM<MyCustomStruct>();
```

### 6.9 Boxing

```cpp
// Box nilai primitif ke Il2CppObject
BNM::IL2CPP::Il2CppObject *boxed = cls.BoxObject(42);
```

### 6.10 Instance Checking

```cpp
// Cek apakah objek adalah instance dari kelas tertentu
bool isPlayer = BNM::IsA(playerObj, playerClass);
bool isPlayer2 = BNM::IsA(playerObj, someIl2CppClass);
bool isSame = BNM::IsA(obj1, obj2);
```

---

## 7. Field — Membaca/Menulis Field

### 7.1 Mendapatkan Field dari Class

```cpp
BNM::Class cls("UnityEngine", "Transform");
BNM::FieldBase fieldBase = cls.GetField("m_Position");
```

### 7.2 Menggunakan FieldBase (UnTyped)

```cpp
// Mendapatkan tipe field
BNM::Class fieldType = fieldBase.GetType();
BNM::Class parentClass = fieldBase.GetParentClass();

// Mendapatkan offset field
BNM_PTR offset = fieldBase.GetOffset();

// Mendapatkan pointer field
void *ptr = fieldBase.GetFieldPointer();

// Cek validitas
if (fieldBase.IsValid()) { /* ... */ }

// Cast ke typed field
BNM::Field<BNM::Structures::Unity::Vector3> posField = fieldBase.cast<BNM::Structures::Unity::Vector3>();
```

### 7.3 Menggunakan Field<T> (Typed)

```cpp
BNM::Class cls("UnityEngine", "Transform");
auto field = cls.GetField("m_LocalPosition").cast<BNM::Structures::Unity::Vector3>();

// Set instance (wajib untuk field non-static)
auto obj = cls.CreateNewObjectParameters();
field.SetInstance((BNM::IL2CPP::Il2CppObject *) obj);

// atau gunakan operator[]
field[obj];

// Baca nilai field
BNM::Structures::Unity::Vector3 value = field.Get();

// Atau gunakan operator
BNM::Structures::Unity::Vector3 value2 = field;         // operator T()
BNM::Structures::Unity::Vector3 value3 = field();        // operator()()

// Tulis nilai field
field.Set({1.0f, 2.0f, 3.0f});

// Atau gunakan operator=
field = {1.0f, 2.0f, 3.0f};
```

### 7.4 Field Static

Field static tidak memerlukan instance:

```cpp
auto staticField = cls.GetField("someStaticField").cast<int>();
int val = staticField.Get();     // Baca field static
staticField.Set(42);             // Tulis field static
```

### 7.5 Field Thread-Static

Field `[ThreadStatic]` dibaca/ditulis menggunakan cara yang sama dengan static, tetapi nilainya per-thread:

```cpp
auto tsField = cls.GetField("threadLocalValue").cast<float>();
float val = tsField.Get();       // Baca dari thread saat ini
tsField.Set(3.14f);              // Tulis ke thread saat ini
```

---

## 8. Method — Memanggil Method

### 8.1 Mendapatkan Method dari Class

```cpp
BNM::Class cls("UnityEngine", "GameObject");

// Berdasarkan nama + jumlah parameter
BNM::MethodBase mb = cls.GetMethod("SetActive", 1);

// Berdasarkan nama + nama parameter
BNM::MethodBase mb2 = cls.GetMethod("SetActive", {"value"});

// Berdasarkan nama + tipe parameter
BNM::MethodBase mb3 = cls.GetMethod("SetActive", {
    BNM::Defaults::Get<bool>()
});
```

### 8.2 Memanggil Method (Non-Static)

```cpp
// Method harus di-cast ke tipe return yang benar
auto method = cls.GetMethod("SetActive", 1).cast<void>();

// Set instance
method.SetInstance((BNM::IL2CPP::Il2CppObject *)gameObject);

// atau gunakan operator[]
method[gameObject];

// Panggil method
method(true);

// atau gunakan operator()
method(true);
```

### 8.3 Memanggil Method (Static)

```cpp
auto cls = BNM::Class("System", "Math");
auto method = cls.GetMethod("Abs", 1).cast<int>();

// Panggil tanpa instance (method static)
int result = method(-42);
```

### 8.4 Menggunakan MethodBase (Tanpa Tipe Return)

```cpp
BNM::MethodBase mb = cls.GetMethod("SomeMethod", 0);
if (mb.IsValid()) {
    mb.SetInstance(instance);

    // Cast ke tipe yang diinginkan sebelum memanggil
    auto &typed = mb.cast<void>();
    typed();
}
```

### 8.5 Mendapatkan Info Method

```cpp
BNM::MethodBase mb = cls.GetMethod("DoSomething", 2);

// Info
BNM::IL2CPP::MethodInfo *info = mb.GetInfo();
BNM_PTR offset = mb.GetOffset();
BNM::Class returnType = mb.GetReturnType();
BNM::Class parentClass = mb.GetParentClass();
bool isStatic = mb._isStatic;
bool isVirtual = mb._isVirtual;

// Cek validitas
if (mb.IsValid()) { /* ... */ }

// Debug: nama lengkap method
#ifdef BNM_ALLOW_STR_METHODS
std::string name = mb.str();
#endif
```

### 8.6 Generic Method

```cpp
auto genericMethod = method.GetGeneric({
    BNM::Defaults::Get<int>()
});
```

### 8.7 Virtual Method Override

```cpp
auto overrideMethod = method.GetOverride();
```

---

## 9. Property — Getter/Setter

### 9.1 Mendapatkan Property

```cpp
BNM::Class cls("UnityEngine", "Transform");

// Mencari property
BNM::PropertyBase propBase = cls.GetProperty("position");

// Mencari property dengan tipe tertentu
BNM::PropertyBase propBase2 = cls.GetProperty("position",
    BNM::Defaults::Get<BNM::Structures::Unity::Vector3>());
```

### 9.2 Menggunakan Property<T> (Typed)

```cpp
auto prop = cls.GetProperty("position").cast<BNM::Structures::Unity::Vector3>();

// Set instance (wajib untuk property non-static)
prop[gameObject];

// Baca nilai (getter)
BNM::Structures::Unity::Vector3 pos = prop.Get();
BNM::Structures::Unity::Vector3 pos2 = prop;       // operator T()
BNM::Structures::Unity::Vector3 pos3 = prop();      // operator()()

// Tulis nilai (setter)
prop.Set({10.0f, 20.0f, 30.0f});
prop = {10.0f, 20.0f, 30.0f};                       // operator=
```

### 9.3 Property Static

```cpp
auto prop = cls.GetProperty("someStaticProp").cast<int>();
int val = prop.Get();         // Baca property static
prop.Set(42);                 // Tulis property static
```

---

## 10. Event — Menambah/Menghapus/Menaikkan Event

### 10.1 Mendapatkan Event

```cpp
BNM::Class cls("UnityEngine", "GameObject");
BNM::EventBase eventBase = cls.GetEvent("onCollisionEnter");
```

### 10.2 Menggunakan Event<Ret, Params...>

```cpp
// Event dengan parameter
auto event = eventBase.cast<void, BNM::IL2CPP::Il2CppObject *>();

// Set instance
event[gameObject];

// Menambah delegate
event.Add(someDelegate);

// Menghapus delegate
event.Remove(someDelegate);

// Menaikkan (call) event
event.Raise(someCollision);
// atau
event(someCollision);
```

### 10.3 Operator += dan -=

```cpp
event += someDelegate;   // Add
event -= someDelegate;   // Remove
```

---

## 11. Image — Bekerja dengan Assembly

### 11.1 Membuat Image

```cpp
// Dari nama
BNM::Image img("Assembly-CSharp.dll");

// Dari Il2CppImage*
BNM::Image img2(someIl2CppImage);

// Dari assembly
BNM::Image img3(someIl2CppAssembly);
```

### 11.2 Mendapatkan Semua Image

```cpp
std::vector<BNM::Image> allImages = BNM::Image::GetImages();
```

### 11.3 Mendapatkan Semua Kelas dari Image

```cpp
std::vector<BNM::Class> classes = img.GetClasses(true);  // true = sertakan inner class
uint32_t count = img.GetClassesCount();
```

---

## 12. Mono Structures — String, Array, List

### 12.1 String (Mono::String)

```cpp
#include <BNM/BasicMonoStructures.hpp>

// Membuat Mono string dari std::string
BNM::Structures::Mono::String *monoStr = BNM::CreateMonoString("Hello");

// Mengkonversi Mono string ke std::string
std::string cppStr = monoStr->str();

// Cek kosong
bool empty = monoStr->IsNullOrEmpty();

// String kosong
BNM::Structures::Mono::String *emptyStr = BNM::Structures::Mono::String::Empty();
```

### 12.2 Array (Mono::Array<T>)

```cpp
// Membuat array kosong
auto *arr = BNM::Structures::Mono::Array<int>::Create(10);

// Membuat array dari vector
std::vector<int> vec = {1, 2, 3};
auto *arr2 = BNM::Structures::Mono::Array<int>::Create(vec);

// Membuat array dari pointer
int data[] = {1, 2, 3};
auto *arr3 = BNM::Structures::Mono::Array<int>::Create(data, 3);

// Operasi array
IL2CPP::il2cpp_array_size_t size = arr->GetCapacity();
int *dataPtr = arr->GetData();
std::vector<int> vecResult = arr->ToVector();

// Akses element
auto element = arr->At(0);      // DataIterator
arr->At(0) = 42;                 // Set element

// Copy data
arr->CopyFrom(vec);
arr->CopyTo(destPtr);

// Cek kosong
bool empty = arr->Empty();

// Hapus (hanya untuk array yang dibuat BNM)
arr->Destroy();
```

### 12.3 List (Mono::List<T>)

```cpp
// Membuat list baru (menggunakan BNM::Class)
auto *list = BNM::Class("System.Collections.Generic", "List`1")
    .GetGeneric({BNM::Defaults::Get<int>()})
    .CreateNewObjectParameters();

// Atau menggunakan helper
auto *list2 = BNM::Class().NewList<int>();

// Operasi list
list->Add(10);
list->Add(20);
list->Add(30);

int size = list->GetSize();
int cap = list->GetCapacity();

// Akses element
int item = list->get_Item(0);
list->set_Item(0, 42);

// Pencarian
int idx = list->IndexOf(20);
bool found = list->Contains(20);

// Hapus
list->Remove(20);
list->RemoveAt(0);
list->Clear();

// Insert
list->Insert(1, 15);

// Resize
list->Resize(100);

// Copy
list->CopyFrom(vec);
list->CopyTo(destArr, 0);

// Konversi ke vector
std::vector<int> vecResult = list->ToVector();

// Iterasi
for (int i = 0; i < list->GetSize(); i++) {
    int val = list->get_Item(i);
}

// Enumerator (foreach-style)
auto enumerator = list->GetEnumerator();
for (auto &item : enumerator) {
    // proses item
}
```

---

## 13. Unity Structures — Vector, Color, dll.

```cpp
#include <BNM/UnityStructures.hpp>

// Tipe yang tersedia:
BNM::Structures::Unity::Vector2 v2{1.0f, 2.0f};
BNM::Structures::Unity::Vector3 v3{1.0f, 2.0f, 3.0f};
BNM::Structures::Unity::Vector4 v4{1.0f, 2.0f, 3.0f, 4.0f};
BNM::Structures::Unity::Color c{1.0f, 0.0f, 0.0f, 1.0f};      // RGBA
BNM::Structures::Unity::Color32 c32{255, 0, 0, 255};           // RGBA byte
BNM::Structures::Unity::Quaternion q{0, 0, 0, 1};               // xyzw
BNM::Structures::Unity::Matrix4x4 m;                             // Matriks 4x4
BNM::Structures::Unity::Ray ray;                                 // Ray
BNM::Structures::Unity::RaycastHit hit;                          // RaycastHit
BNM::Structures::Unity::RaycastHit2D hit2D;                      // RaycastHit2D
```

---

## 14. Defaults — Mapping Tipe C++ ke C#

`BNM::Defaults::Get<T>()` mengembalikan referensi ke `Il2CppClass` untuk tipe C++ tertentu.

### Tipe yang Didukung

| C++ | C# | .NET |
|---|---|---|
| `void` | `void` | `System.Void` |
| `bool` | `bool` | `System.Boolean` |
| `BNM::Types::byte` (uint8_t) | `byte` | `System.Byte` |
| `BNM::Types::sbyte` (int8_t) | `sbyte` | `System.SByte` |
| `short` (int16_t) | `short` | `System.Int16` |
| `BNM::Types::ushort` (uint16_t) | `ushort` | `System.UInt16` |
| `int` (int32_t) | `int` | `System.Int32` |
| `BNM::Types::uint` (unsigned int) | `uint` | `System.UInt32` |
| `BNM::Types::nint` (intptr_t) | `nint` | `System.IntPtr` |
| `BNM::Types::nuint` (uintptr_t) | `nuint` | `System.UIntPtr` |
| `long` (int64_t) | `long` | `System.Int64` |
| `BNM::Types::ulong` (unsigned long) | `ulong` | `System.UInt64` |
| `float` | `float` | `System.Single` |
| `double` | `double` | `System.Double` |
| `BNM::Structures::Mono::String *` | `string` | `System.String` |
| `BNM::Structures::Unity::Vector2` | `Vector2` | `UnityEngine.Vector2` |
| `BNM::Structures::Unity::Vector3` | `Vector3` | `UnityEngine.Vector3` |
| `BNM::Structures::Unity::Vector4` | `Vector4` | `UnityEngine.Vector4` |
| `BNM::Structures::Unity::Color` | `Color` | `UnityEngine.Color` |
| `BNM::Structures::Unity::Quaternion` | `Quaternion` | `UnityEngine.Quaternion` |
| `BNM::UnityEngine::Object *` | `Object` | `UnityEngine.Object` |
| Pointer lainnya | `object` | `System.Object` |

### Boxing

```cpp
// Box nilai primitif ke Il2CppObject
BNM::IL2CPP::Il2CppObject *boxed = BNM::Defaults::Box(42);
BNM::IL2CPP::Il2CppObject *boxedFloat = BNM::Defaults::Box(3.14f);

// Unbox
int val = BNM::UnboxObject(boxed);
```

---

## 15. CompileTimeClass — Referensi Tipe di Waktu Kompilasi

`CompileTimeClass` memungkinkan Anda mereferensikan tipe il2cpp di waktu kompilasi, yang akan di-resolve saat runtime.

### 15.1 Membuat CompileTimeClass

```cpp
// Menggunakan CompileTimeClassBuilder (direkomendasikan)
auto ctc = BNM::CompileTimeClassBuilder(
    "UnityEngine",              // namespace
    "GameObject",               // nama kelas
    "UnityEngine.CoreModule.dll" // nama image (opsional)
).Build();
```

### 15.2 Menambahkan Inner Class, Modifier, Generic

```cpp
auto ctc = BNM::CompileTimeClassBuilder("MyNamespace", "MyClass")
    .Class("NestedClass")                                    // Inner class
    .Modifier(BNM::CompileTimeClass::ModifierType::Array)    // Array
    .Generic({                                               // Generic
        BNM::Defaults::Get<int>(),
        BNM::Defaults::Get<float>()
    })
    .Build();
```

### 15.3 Menggunakan CompileTimeClass

```cpp
// Konversi ke Class
BNM::Class cls = ctc.ToClass();

// Konversi ke Il2CppType
BNM::IL2CPP::Il2CppType *type = ctc.ToIl2CppType();

// Konversi ke Il2CppClass
BNM::IL2CPP::Il2CppClass *klass = ctc.ToIl2CppClass();
```

---

## 16. ClassesManagement — Membuat/Memodifikasi Kelas di Runtime

> **Memerlukan** `#define BNM_CLASSES_MANAGEMENT` di `GlobalSettings.hpp` (aktif secara default).

### 16.1 Membuat Kelas Baru

```cpp
#include <BNM/ClassesManagement.hpp>

// Definisikan kelas C++ yang akan ditambahkan ke il2cpp
struct MyCustomComponent : BNM::IL2CPP::Il2CppObject {
    float health;
    int damage;
    BNM::Structures::Mono::String *name;
};

// Daftarkan ke il2cpp
BNM_CustomClass(
    MyCustomComponent,                              // Kelas C++
    BNM::CompileTimeClassBuilder("Game", "CustomComponent").Build(), // Tipe di il2cpp
    BNM::Defaults::Get<BNM::UnityEngine::MonoBehaviour *>(),        // Base class
    BNM::Defaults::Get<void>()                     // Owner (null = tidak nested)
);

// Daftarkan field
BNM_CustomField(MyCustomComponent::health,  BNM::Defaults::Get<float>(), "health");
BNM_CustomField(MyCustomComponent::damage,  BNM::Defaults::Get<int>(), "damage");
BNM_CustomField(MyCustomComponent::name,    BNM::Defaults::Get<BNM::Structures::Mono::String *>(), "name");

// Daftarkan method
BNM_CustomMethod(MyCustomComponent::TakeDamage, false,  // false = non-static
    BNM::Defaults::Get<void>(), "TakeDamage",
    BNM::Defaults::Get<float>()
);
```

### 16.2 Menggunakan Kelas yang Dibuat

```cpp
// Membuat instance
auto *comp = MyCustomComponent::BNMCustomClass.CreateNewInstance();

// Mengakses field
comp->health = 100.0f;
comp->damage = 10;

// Atau melalui BNM
auto field = BNM::Class(MyCustomComponent::BNMCustomClass.type)
    .GetField("health").cast<float>();
field[comp];
float hp = field.Get();
```

### 16.3 Memodifikasi Kelas yang Ada

```cpp
// Override method yang sudah ada
struct MyOverride : BNM::IL2CPP::Il2CppObject {
    void Start() {
        // Kode Anda
    }
};

// Mendaftarkan override method (mengganti method yang ada)
// ... (lihat BNM_CustomMethod dengan macro pendukung)
```

### 16.4 Macro Pendukung

| Macro | Keterangan |
|---|---|
| `BNM_CustomMethodMarkAsInvokeHook(method)` | Gunakan InvokeHook untuk method ini |
| `BNM_CustomMethodMarkAsBasicHook(method)` | Gunakan BasicHook untuk method ini |
| `BNM_CustomMethodSkipTypeMatch(method)` | Lewati pencocokan tipe parameter |
| `BNM_CustomMethodCopyAttributes(method, "targetMethod")` | Salin atribut dari method lain |
| `BNM_CallCustomMethodOrigin(method, ...)` | Panggil method asli (override/hook origin) |

### 16.5 Runtime Class Processing

```cpp
// Memproses kelas custom di runtime (harus di thread il2cpp)
BNM::ClassesManagement::ProcessClassRuntime(myCustomClass);
```

---

## 17. Coroutine — Koroutine C++20

> **Memerlukan** `BNM_CLASSES_MANAGEMENT` dan `BNM_COROUTINE` (keduanya aktif secara default).

### 17.1 Struktur yang Tersedia

```cpp
#include <BNM/Coroutine.hpp>

// Yield instructions:
BNM::Coroutine::WaitForSeconds(1.0f);           // Tunggu 1 detik
BNM::Coroutine::WaitForSecondsRealtime(1.0f);   // Tunggu 1 detik (realtime)
BNM::Coroutine::WaitForEndOfFrame();             // Tunggu akhir frame
BNM::Coroutine::WaitForFixedUpdate();            // Tunggu FixedUpdate
BNM::Coroutine::WaitUntil([]() { return ready; });    // Tunggu sampai true
BNM::Coroutine::WaitWhile([]() { return loading; });  // Tunggu sampai false
```

### 17.2 Membuat Koroutine

```cpp
// Koroutine dikembalikan sebagai IEnumerator yang bisa diberikan ke Unity
BNM::Coroutine::IEnumerator MyCoroutine() {
    co_yield BNM::Coroutine::WaitForSeconds(1.0f);
    // Lakukan sesuatu setelah 1 detik

    co_yield BNM::Coroutine::WaitForEndOfFrame();
    // Lakukan sesuatu di akhir frame

    co_return;  // Selesai
}

// Menggunakan koroutine
auto *enumerator = MyCoroutine().Get();
// Berikan ke Unity: StartCoroutine(enumerator)
```

---

## 18. Delegates — Action dan MulticastDelegate

### 18.1 Delegate (Single)

```cpp
#include <BNM/Delegates.hpp>

// Membuat delegate
BNM::MethodBase method = someClass.GetMethod("OnSomething", 0);
BNM::Delegate<void> *delegate = (BNM::Delegate<void> *)BNM::Allocate(sizeof(BNM::Delegate<void>));
delegate->Create(method);

// Invoke delegate
delegate->Invoke();
// atau
(*delegate)();
```

### 18.2 MulticastDelegate

```cpp
BNM::MulticastDelegate<void> *multi = (BNM::MulticastDelegate<void> *)BNM::Allocate(sizeof(BNM::MulticastDelegate<void>));
multi->Add(method1);
multi->Add(method2);

// Invoke semua delegate
multi->Invoke();

// Tambah/hapus
*multi += delegate;
*multi -= delegate;
```

### 18.3 Action

```cpp
// Action<Params...> adalah MulticastDelegate<void> yang sudah ditypedef
BNM::Structures::Mono::Action<int, float> *action = ...;
action->Invoke(42, 3.14f);
```

---

## 19. Hooking — Cara Hook Method

### 19.1 BasicHook (Inline Hook)

Hook method pada tingkat method pointer:

```cpp
// Simpan method asli
BNM::Method<void> originalMethod;

// Hook
auto target = cls.GetMethod("Update", 0);
BNM::BasicHook(target, [](auto *instance) {
    // Kode hook Anda
    // ...

    // Panggil method asli
    originalMethod.SetInstance(instance);
    originalMethod();
}, originalMethod);
```

### 19.2 InvokeHook (MethodInfo Hook)

Hook dengan mengganti MethodInfo — hanya bekerja jika method dipanggil melalui il2cpp API (umum untuk Unity messages seperti `Start`, `Update`, dll):

```cpp
auto target = cls.GetMethod("Start", 0);
BNM::InvokeHook(target, [](auto *instance) {
    // Kode hook
}, originalMethod);
```

### 19.3 VirtualHook

Hook method virtual dengan mengubah vtable:

```cpp
auto target = cls.GetMethod("OnDestroy", 0);
BNM::VirtualHook(cls, target, [](auto *instance) {
    // Kode hook
}, originalMethod);
```

### 19.4 Perbedaan Metode Hooking

| Metode | Kelebihan | Kekurangan |
|---|---|---|
| **BasicHook** | Bekerja untuk semua method | Overhead kecil (memodifikasi kode mesin) |
| **InvokeHook** | Zero overhead untuk Unity messages | Hanya untuk method yang dipanggil via il2cpp API |
| **VirtualHook** | Tidak memodifikasi kode mesin | Tidak dipicu oleh method yang sudah override |

---

## 20. Utilitas Tambahan

### 20.1 Cek Validitas Pointer

```cpp
// Cek apakah pointer valid (tidak null)
bool valid = BNM::CheckForNull(ptr);

// Cek apakah alamat valid (dengan signal handler)
bool valid = BNM::IsAllocated(ptr);  // Memerlukan BNM_ALLOW_SAFE_IS_ALLOCATED
```

### 20.2 Membuat Mono String

```cpp
BNM::Structures::Mono::String *str = BNM::CreateMonoString("Hello World");
```

### 20.3 Mendapatkan Extern Method (icall)

```cpp
void *addr = BNM::GetExternMethod("UnityEngine.Resources::Load");
```

### 20.4 Cek Status BNM

```cpp
bool loaded = BNM::IsLoaded();
void *handle = BNM::GetIl2CppLibraryHandle();
```

### 20.5 Thread Management

```cpp
// Attach thread ke il2cpp VM
bool attached = BNM::AttachIl2Cpp();

// Dapatkan thread saat ini
auto *thread = BNM::CurrentIl2CppThread();

// Detach thread
BNM::DetachIl2Cpp();
```

### 20.6 Memory Management

```cpp
// Alokasi memory yang terdaftar di GC il2cpp
void *mem = BNM::Allocate(1024);

// Bebaskan memory
BNM::Free(mem);
```

### 20.7 Unmarshal Unity Object (Unity ≥ 2023.2)

```cpp
BNM_INT_PTR unmarshaled = BNM::UnmarshalUnityObject(gcHandlePtr);
```

### 20.8 BNM_OBFUSCATE

Makro untuk enkripsi string (ganti dengan string encryptor Anda):

```cpp
#define BNM_OBFUSCATE(str) str
#define BNM_OBFUSCATE_TMP(str) str
```

---

## 21. Trouble Shooting

### Build Gagal: `static_assert(false, "No hooking software!")`
→ Anda belum mengkonfigurasi hook backend. Lihat [Bagian 4](#4-konfigurasi-hook-backend).

### Build Gagal: `static_assert(false, "Setup UNITY_VER in GlobalSettings.hpp")`
→ `UNITY_VER` tidak dikenali. Periksa versi Unity target Anda dan uncomment yang sesuai.

### Class tidak ditemukan / IsValid() false
- Periksa apakah `UNITY_VER` sudah benar
- Periksa ejaan namespace dan nama kelas (case-sensitive)
- Pastikan BNM sudah dimuat (`BNM::IsLoaded()`)
- Jika menggunakan `Image`, pastikan nama image benar (termasuk `.dll`)

### Method/Field tidak ditemukan
- Periksa ejaan nama (case-sensitive)
- Untuk method overloads, gunakan versi `GetMethod` dengan parameter types
- Gunakan `GetMethods()` untuk melihat semua method yang tersedia

### Crash saat memanggil method
- Pastikan instance valid dan masih hidup
- Untuk Unity objects, selalu gunakan `IsValid()` (bukan null check biasa)
- Periksa jumlah dan tipe parameter sesuai signature method

### Crash saat Loading
- Pastikan il2cpp sudah benar-benar dimuat sebelum `TrySetupByUsersFinder`
- Untuk `TryLoadByJNI`, pastikan `JNIEnv` valid
- Coba gunakan `AllowLateInitHook()` untuk game yang memulai il2cpp lambat

### Multi-threading Issues
- Aktifkan `BNM_ALLOW_MULTI_THREADING_SYNC` jika menggunakan BNM dari banyak thread
- Method `AttachIl2Cpp()` / `DetachIl2Cpp()` diperlukan untuk thread selain thread utama il2cpp
