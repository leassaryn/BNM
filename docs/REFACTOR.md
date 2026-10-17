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
destructor/copy/move khusus, dan layout proxy coroutine berubah. Jangan mencampur
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

Array mengembalikan null saat ukuran overflow atau alokasi gagal. List tidak
menulis melewati kapasitas jika pertumbuhannya gagal. Dictionary non-DOTNET35
menghitung `count - freeCount` dan melewati entry dengan hash negatif sesuai
layout header yang direpresentasikan. Tidak ada klaim untuk layout .NET lain.

## ClassesManagement

Batas parameter 255 dan batas counter metadata 65535 diperiksa sebelum mutasi.
Field class baru harus muat dalam `_size`; ukuran harus mencakup parent.
Pembuatan class baru dan modifikasi/hook method tetap tersedia.

Menambah field ke **class yang sudah ada** atau mengganti parent-nya ditolak.
Gunakan class baru dengan layout yang ditentukan sebelum instance dibuat.
Mengganti angka `instance_size` tidak memperbesar objek lama atau memperbarui
layout turunan/GC. Tidak ada opsi yang diam-diam mengaktifkan kembali perilaku ini.
Periksa `CustomClass::myClass`/`CustomMethod::myInfo` sesudah pemrosesan.

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

Alokasi metadata kini diperiksa sebelum dereference dan error ditahan di boundary
pemrosesan. Rollback penuh seluruh perubahan metadata/hook pada OOM belum
tersedia; sebagian alokasi bisa bertahan sampai proses selesai. Jangan mengulang
mutasi descriptor setelah OOM. Jalankan uji Android/Unity dengan backend nyata,
full Unity patch version, ABI dan konfigurasi GC yang dipakai aplikasi.
