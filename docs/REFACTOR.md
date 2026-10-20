# Refactor fitur inti BNM

Dokumen ini menjelaskan kontrak baru pada fork eksperimental. Lihat [REVIEW.md](../REVIEW.md)
untuk bukti audit dan hasil verifikasi, [panduan API](DOCS.md) untuk integrasi,
serta [tests/README.md](../tests/README.md) untuk tes regresi.

## Integrasi dan build

Standar C++20, nama library BNM, minimum API konsumen, daftar source CMake/ndk-build,
dan pemetaan header Unity tidak berubah. Pilih backend nyata di
`include/BNM/UserSettings/GlobalSettings.hpp` dan sediakan header/library-nya.
Backend dummy tetap menolak kompilasi; stub pada pengujian **bukan backend game**.
`BNM_SANITIZERS=ON` berlaku pada konfigurasi build yang dipilih, termasuk Release.
`Android.mk` menyediakan include internal, tetapi konsumen ndk-build tetap harus
menambahkan include publik karena `LOCAL_EXPORT_C_INCLUDES` belum disediakan.

Bangun ulang BNM dan seluruh konsumennya bersama. `CompileTimeClass` kini memiliki
destructor/copy/move khusus, dan layout proxy coroutine serta `DataIterator` berubah. Jangan mencampur
objek/library hasil kompilasi header lama dan baru.

## Loading dan threading

- Konfigurasikan finder dan opsi late-init **sebelum** memasang hook atau setup.
- `TryLoadBy*` berarti hook awal berhasil dipasang; kesiapan runtime diperiksa
  dengan `BNM::IsLoaded()` atau callback `AddOnLoadedEvent`.
- Simbol wajib yang tidak ditemukan, scanner yang keluar dari mapping readable,
  dan kegagalan pemasangan hook menggagalkan setup. Hook internal yang sudah
  dipasang dalam percobaan setup dibatalkan apabila hook berikutnya gagal.
- Resolusi simbol yang gagal boleh dicoba lagi sebelum mutasi metadata dimulai.
  Kegagalan sesudah mutasi dimulai memerlukan restart proses; jangan replay
  descriptor yang mungkin telah mengubah method.
- Callback dipanggil tanpa mutex daftar callback. Registrasi/penghapusan di dalam
  callback memengaruhi daftar untuk pemanggilan berikutnya, bukan snapshot aktif.
- Cache/list internal disinkronkan; ini tidak menjadikan semua operasi managed
  thread-safe. Mutasi metadata dan hook tetap memerlukan safe point aplikasi
  tanpa pembaca metadata/eksekusi method target yang bersamaan.
- JNI memakai local frame dan melepaskan UTF chars. Exception Java dipertahankan;
  caller bertanggung jawab menanganinya. Utamakan Context eksplisit. Pencarian
  split APK memakai `ApplicationInfo.splitSourceDirs`.

`ScopedIl2CppThread` melakukan attach hanya jika thread belum terpasang. Guard
bersarang meminjam attachment; hanya guard pemilik melakukan detach saat scope
berakhir, termasuk saat exception C++ keluar. Guard tidak dapat disalin atau
dipindahkan dan harus dihancurkan di thread pembuatnya sebelum runtime berhenti.
Jangan melakukan detach manual selama guard aktif. Periksa `operator bool()`;
API/domain tidak tersedia atau attach gagal menghasilkan guard kosong.

```cpp
BNM::ScopedIl2CppThread scope;
if (!scope) return;
// Panggil API managed di thread ini.
```

## Class, method, field, dan koleksi

`CompileTimeClass` memiliki descriptor sendiri pada setiap copy; `Free()` hanya
membebaskan milik objek itu. Destructor membersihkan descriptor yang tersisa.
`autoFree=false` mempertahankan descriptor sampai `Free()` atau destructor.
Nama namespace/class/image masih string pinjaman: caller harus menjaga lifetime.
Resolusi const/generic tidak menghapus argumen pada descriptor sumber.

`MethodBase`, `FieldBase`, `PropertyBase`, dan `EventBase::cast<T>()` mengembalikan
wrapper **by value**. Ganti `auto &typed = base.cast<T>()` menjadi
`auto typed = base.cast<T>()`. `typed.SetInstance(...)` mengubah wrapper itu;
wrapper `base` tidak ikut berubah. Pemanggilan berantai tetap didukung.
`Method::Call` menolak jumlah argumen yang salah dan methodPointer null.
Template tipe parameter/return tetap harus cocok dengan signature IL2CPP.

`GetOverride()` dan `VirtualHook()` memakai slot metadata, bukan pencocokan nama
atau parameter. Method class harus berasal dari class target atau parent-nya;
method interface memakai pasangan interface/offset yang persis tercatat pada
class target. Ini membedakan method `new virtual` dengan signature sama dan
mendukung nama implementasi interface eksplisit yang berbeda. Resolusi memanggil
inisialisasi class runtime sebelum membaca vtable. Interface variance dan
inflasi method generic tidak ditambahkan oleh resolver ini; jangan menganggapnya
pengganti seluruh mekanisme dispatch runtime IL2CPP.

`VirtualHook()` menolak replacement null, slot tidak valid, method nonvirtual,
class tidak terkait, dan entry tanpa method/pointer. Kegagalan tidak mengubah
`oldMet`. `GetOverride()` tetap mengembalikan metadata method dengan instance
terikat; hook vtable hanya mengubah `methodPtr` pada vtable, bukan `methodPointer`
pada metadata. Pemanggilan langsung wrapper tidak otomatis melewati hook vtable.
Pada mode pemeriksaan tipe, binding method virtual menerima interface yang
tercatat; binding null atau ditolak menghapus instance lama.

Array mengembalikan null saat ukuran overflow atau alokasi gagal. List tidak
menulis melewati kapasitas jika pertumbuhannya gagal. Dictionary non-DOTNET35
menghitung `count - freeCount` dan melewati entry dengan hash negatif sesuai
layout header yang direpresentasikan. Tidak ada klaim untuk layout .NET lain.

## Write barrier GC

Loader kini mewajibkan simbol `il2cpp_gc_wbarrier_set_field`; finder kustom harus
menyediakannya. Store referensi tanpa API barrier gagal tanpa menulis slot.
`StoreManagedReference(owner, slot, value)` mengembalikan status untuk caller
level rendah. `StoreManagedValue` dan `CopyManagedRange` mendukung data trivially
copyable; reference-free bulk copy tetap memakai `memmove`.

`Field<T>::Set` melewatkan field static/thread-static ke setter IL2CPP. Instance
reference memakai barrier, sedangkan value type dengan metadata ditelusuri untuk
reference field di dalamnya. Array `Set`, `CopyFrom`, `CopyRange`, assignment
`array[i] = value`, serta mutasi List memakai jalur yang sama, termasuk pergeseran,
pembersihan slot, dan penggantian backing array. Pertumbuhan list managed memakai
allocator array runtime; tidak lagi menempelkan `klass` pada buffer native.
Delegate dan field current proxy coroutine juga memakai barrier.

Reference type dibedakan dari native value melalui tipe wrapper/metadata.
Struct native tanpa metadata IL2CPP tidak dapat dipindai untuk referensi tersembunyi;
berikan class value type yang sesuai ke `StoreManagedValue`/`CopyManagedValue`.
`GetData()`, `GetPointer()`, `m_Items`, dereference iterator, dan konversi ke `T&`
tetap merupakan akses langsung yang melewati barrier; gunakan setter untuk
menulis referensi. Barrier tidak membuat pointer native menjadi GC root dan tidak
menjadikan koleksi thread-safe. Dictionary memakai mutator managed yang sudah ada.

[Dokumentasi Unity](https://docs.unity.cn/Manual/performance-incremental-garbage-collection.html)
menjelaskan kebutuhan barrier ketika referensi berubah selama incremental GC.

## ClassesManagement

Batas parameter 255 dan batas counter metadata 65535 diperiksa sebelum mutasi.
Field class baru harus muat dalam `_size`; ukuran harus mencakup parent.
Pembuatan class baru dan modifikasi/hook method tetap tersedia.

Menambah field ke **class yang sudah ada** atau mengganti parent-nya ditolak.
Gunakan class baru dengan layout yang ditentukan sebelum instance dibuat.
Mengganti angka `instance_size` tidak memperbesar objek lama atau memperbarui
layout turunan/GC. Tidak ada opsi yang diam-diam mengaktifkan kembali perilaku ini.
Periksa `CustomClass::myClass`/`CustomMethod::myInfo` sesudah pemrosesan.

Pemrosesan **satu descriptor** sekarang memakai transaksi metadata native:
alokasi baru dicatat, metadata/owner/method/vtable disalin sebelum mutasi, dan
buffer lama baru dibebaskan setelah commit. Cache class List juga memakai RAII:
class/method sementara dibebaskan jika pembuatan gagal, dan cache baru diisi
sesudah konstruksi selesai. Kegagalan alokasi atau hook membatalkan
perubahan, menarik assembly baru, membersihkan reservasi map kosong, serta
memulihkan `myInfo`, `_origin`, dan `_originalAddress`. Parameter/target descriptor
tetap tersedia saat gagal; descriptor yang sudah commit tidak diproses ulang.
Hook native yang terpasang dibatalkan memakai handle yang dikembalikan backend.

Transaksi bukan rollback seluruh batch loading: descriptor sebelumnya yang sudah
commit tetap berlaku. Cache/alokasi internal IL2CPP dan kegagalan unhook backend
bukan resource yang dapat dipulihkan oleh jurnal native. Retry descriptor setelah
OOM native didukung; jika ada hook mesin, pastikan backend benar-benar berhasil
melepas hook sebelum mencoba lagi. Tetap gunakan safe point tanpa pembaca metadata
bersamaan. Semua alokasi native untuk class baru diselesaikan sebelum inisialisasi
class oleh runtime.

## Coroutine

```cpp
auto *enumerator = MyCoroutine().Get();
// Serahkan enumerator ke Unity dan jaga referensi managed selama digunakan.
```

`IEnumerator` native bersifat move-only. `.Get()` mentransfer frame ke proxy
managed dan tidak menjalankan body. Unity `MoveNext()` mengamati setiap yield,
termasuk `nullptr` untuk menunggu satu frame. Penyelesaian mengembalikan false;
Reset/finalizer menghancurkan frame satu kali. Nilai native yang tidak pernah
`Get()` membebaskan frame melalui destructor. Exception body/predicate dicatat
dan menghentikan enumerasi; exception C++ tidak dilempar ke caller Unity.

Proxy mendaftarkan field current agar terlihat pada descriptor GC. Referensi
managed lain yang ditangkap coroutine/lambda tetap memerlukan rooting oleh
konsumen. Finalizer, write barrier collector incremental, dan scheduler Unity
harus diuji di proyek target sebelum rilis; fixture native tidak meniru GC.

## Batas verifikasi

`IsAllocated` pada mode pemeriksaan hanya menguji permission mapping saat itu;
bukan bukti lifetime objek, sinkronisasi, atau keselamatan dereference kemudian.
Pemeriksaan mapping memakai `/proc/self/maps`, tidak cocok untuk signal handler.
Mode Release tanpa flag pemeriksaan tetap hanya memeriksa null.

Uji kegagalan alokasi native mencakup pembuatan class/image dan modifikasi class,
termasuk owner serta invoke hook sebelum alokasi berikutnya gagal. Uji ini tidak
mensimulasikan OOM internal IL2CPP, collector, atau kegagalan backend saat unhook.
Jalankan uji Android/Unity dengan backend nyata, full Unity patch version, ABI,
dan konfigurasi GC yang dipakai aplikasi.

## Perbaikan review lanjutan (lihat TODO.md)

- `NewListBNM<T>()` sekarang alias `NewList<T>()`: class dan interface List berasal
  dari generic instantiation IL2CPP yang sebenarnya. Metadata/vtable buatan dari
  open generic class sudah dihapus. Tipe yang tidak tersedia bagi runtime tidak
  didukung dan kegagalan yang dilaporkan resolver/alokasi sebagai null menghasilkan
  `nullptr`. Managed exception dari generic instantiation/constructor mengikuti
  kontrak `NewList` dan dapat ditangani dengan mekanisme exception BNM; factory ini
  bukan API no-throw. `T` tetap harus cocok dengan class elemen yang digunakan. Untuk koleksi C++ murni,
  gunakan container native; jangan menyerahkannya sebagai managed `List<T>`.
- `Types::ulong` kini selalu 64 bit. `Defaults::Get<long>()` mengikuti ukuran native
  (`Int32` pada ABI 32-bit, `Int64` pada ABI 64-bit); gunakan `int64_t`/`uint64_t`
  untuk C# `long`/`ulong`, dan `Types::nint/nuint` untuk integer seukuran pointer.
- `MulticastDelegateBase::TryAdd` melaporkan kegagalan tanpa mengganti invocation
  list. `Add`/`Remove` tetap memutasi objek yang sama, sehingga alias melihat perubahan;
  ini berbeda dari semantik immutable `System.Delegate.Combine/Remove`.
- Scanner x86/x64 mengikuti batas instruksi. Decoder sengaja konservatif: instruksi
  tidak dikenal, branch bersyarat, return, atau control transfer tidak langsung
  menghentikan pencarian. Setup dapat gagal pada stub yang belum didukung, sehingga
  tidak akan menebak alamat dari byte operand. Perluasan decoder harus disertai tes.
- `Matrix4x4::rotation()` mendukung basis TRS tanpa shear; reflection ditempatkan pada
  skala X seperti `GetLossyScale()`. Basis singular, non-finite, atau shear menghasilkan
  quaternion identity. `NormalizeAngle` memakai modulo; NaN/Inf menghasilkan NaN.
- UnityEvent dirty menjalankan persistent calls langsung dan runtime listeners sekali,
  tanpa memakai cache persistent yang usang. Flag dirty tidak dibersihkan karena BNM
  tidak membangun ulang cache internal Unity.

Perubahan ini memiliki regresi native sintetis. Dukungan GC, hook backend dan perilaku
managed pada game sesungguhnya tetap memerlukan pengujian runtime per konfigurasi.
