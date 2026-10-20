# TODO — perbaikan hasil review BNM

Baseline: `81c8c0f`. Lokasi/baris pada deskripsi masalah merujuk baseline sebelum perbaikan. Checkbox selesai berarti implementasi dan regresi lokal lulus; bukan sertifikasi runtime semua versi Unity.

## Checklist bug

- [x] R01 — P1 — Lebar tipe C# tidak konsisten pada ABI 32-bit
- [x] R02 — P1 — Vtable custom List menyatukan method dengan signature berbeda
- [x] R03 — P1 — NewListBNM mengembalikan managed object tanpa klass saat OOM
- [x] R04 — P1 — Resolver collider yang gagal tetap dipanggil
- [x] R05 — P1 — Scanner x86 menganggap byte operand sebagai CALL
- [x] R06 — P2 — UnityEvent dirty melewatkan runtime listeners
- [x] R07 — P2 — Assignment proxy ke proxy tidak menulis elemen
- [x] R08 — P2 — Field mempertahankan instance lama setelah bind ditolak
- [x] R09 — P2 — Penambahan pada delegate tunggal menjadi no-op
- [x] R10 — P2 — Matrix3x3 perkalian diri sendiri salah
- [x] R11 — P2 — Matrix4x4::rotation salah pada nonuniform scale
- [x] R12 — P2 — NormalizeAngle dapat loop tanpa akhir
- [x] R13 — P2 — Audit matrix diam-diam tetap memakai patch 32
- [x] R14 — P2 — Logging string_view memakai %s tanpa panjang
- [x] R15 — P3 — Header publik bergantung pada urutan include
- [x] R16 — P2 — Overflow pertumbuhan List pada header array signed

## R01 — P1 — Lebar tipe C# tidak konsisten pada ABI 32-bit

include/BNM/Defaults.hpp:49,157; include/BNM/Class.hpp:460
Get<long>() memilih System.Int64, tetapi sizeof(long)==4 di ARMv7. Types::ulong juga
menggunakan unsigned long. Get<long long>() justru kosong. Saat boxing, alamat objek
C++ 4-byte diteruskan ke runtime untuk tipe 8-byte; operasi method/field terkait juga
berisiko salah ABI. Cross-compile width_check.cpp mengonfirmasi mismatch tipe.
Saran: tipe managed integer fixed-width dan pemetaan berdasarkan ukuran/signedness;
pertahankan nint/nuint sebagai tipe pointer-sized terpisah. Tambahkan compile assertion
untuk seluruh ABI dan test boxing/argument/return 64-bit di target 32-bit.

**Perbaikan:** Pemetaan long/unsigned long mengikuti lebar native; long long dan fixed-width 64-bit didukung; Types::ulong memakai uint64_t.

**Regresi:** tests/abi_types.cpp pada empat ABI.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R02 — P1 — Vtable custom List menyatukan method dengan signature berbeda

src/MonoStructures.cpp:76-93; include/BNM/BasicMonoStructures.hpp:689-711
Nama interface dipotong sampai komponen setelah titik, kemudian metadata dikunci
hanya dengan hash nama. Add dan System.Collections.IList.Add menjadi satu MethodInfo,
meskipun return type berbeda. Probe list-signatures membuktikan dua slot berbeda
mendapat pointer MethodInfo yang sama. GetEnumerator juga diarahkan ke fungsi native
yang mengembalikan Enumerator struct; interface enumerator memerlukan adapter yang
sesuai. Pemanggilan managed dapat memakai ABI/boxing yang salah.
Saran: cocokkan declaring interface dan signature lengkap, gunakan thunk/adaptor
per signature, dan jangan memublikasikan interface yang belum benar. Utamakan NewList
runtime untuk tipe yang tersedia. Warning 'expert API' tidak menyelesaikan mismatch.

**Perbaikan:** Jalur class/vtable List sintetis dihapus. NewListBNM<T>() menjadi alias NewList<T>() dengan metadata generic runtime asli. Tipe native-only tanpa instantiation runtime tidak didukung; lihat docs/REFACTOR.md.

**Regresi:** runtime-list: metadata runtime tetap utuh dan constructor dijalankan.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R03 — P1 — NewListBNM mengembalikan managed object tanpa klass saat OOM

include/BNM/Class.hpp:433-446; include/BNM/BasicMonoStructures.hpp:711
TryGetMonoListClass sudah mengembalikan nullptr dengan aman saat alokasi gagal, tetapi
InitMonoListVTable menuliskannya ke list->klass dan public factory tetap return list.
Probe list-oom-public: non-null list with null klass after OOM=1.
Saran: bangun/validasi metadata lebih dahulu; hanya pasang klass yang valid. Factory
harus return nullptr pada kegagalan dan menjaga objek yang telah dialokasikan tetap
memiliki header valid bagi GC. Tambahkan tes factory, bukan hanya helper cache.

**Perbaikan:** Factory memakai jalur runtime, tanpa mengganti header klass setelah alokasi. Kegagalan resolver/alokasi yang dilaporkan null menghasilkan nullptr; managed exception mengikuti kontrak NewList (bukan API no-throw).

**Regresi:** runtime-list: alokasi objek gagal, alokasi argument generic gagal, resolver tidak tersedia.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R04 — P1 — Resolver collider yang gagal tetap dipanggil

src/UnityStructures.cpp:9-44
GetExternMethod dapat mengembalikan nullptr, tetapi RaycastHit/RaycastHit2D langsung
memanggil FromId tanpa cek. Probe raycast-null dengan ID positif dan resolver null
menghasilkan crash ASan di baris 23. Jalur Unity baru juga perlu menjaga hasil handle
null sebelum UnmarshalUnityObject.
Saran: return nullptr jika resolver/hasil resolve tidak tersedia; tes kedua tipe dan
kedua kelompok versi. Signature icall Unity terbaru tetap perlu runtime validation.

**Perbaikan:** RaycastHit dan RaycastHit2D memeriksa resolver null; UnmarshalUnityObject memeriksa alamat handle setelah tag dihapus.

**Regresi:** raycast-resolver: missing icall dan hasil resolver null; kompilasi jalur Unity baru melalui matrix.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R05 — P1 — Scanner x86 menganggap byte operand sebagai CALL

src/Loading.cpp:301-317
Scanner maju satu byte dan menerima E8 di mana pun, tanpa memeriksa batas instruksi.
Fixture berisi MOV eax,0xe8; NOP; RET (tanpa CALL) menghasilkan target non-null.
Alamat tersebut dapat diteruskan ke tahap resolve/hook, jadi cek readable saja tidak
cukup untuk menjamin target adalah fungsi yang dimaksud.
Saran: instruction decoding/boundary-aware traversal, validasi executable target,
stop pada terminator, dan gagalkan setup bila pola tidak dapat dibuktikan. Pertahankan
custom resolver sebagai jalur eksplisit. Fixture scanner-review.cpp/log tersedia.

**Perbaikan:** Scanner x86/x64 memakai decoder batas instruksi konservatif. Opcode tidak dikenali, branch/return dan indirect transfer menghentikan pencarian. Tidak lagi melanjutkan satu byte setelah kegagalan decode.

**Regresi:** x86-decoder dan scanner-operand: immediate/displacement E8, CALL sungguhan, truncation, RET, opcode unsupported.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R06 — P2 — UnityEvent dirty melewatkan runtime listeners

include/BNM/UnityStructures.hpp:215-229,300
Invoke memilih InvokePersistent saat m_CallsDirty, lalu tidak mengeksekusi runtime
listeners. Persistent group kosong langsung return. Probe event-dirty: listener yang
sama dipanggil 0 kali saat dirty, 1 kali setelah dirty dimatikan.
Saran: rebuild persistent calls bila dirty, kemudian jalankan gabungan persistent dan
runtime calls sekali. Test dirty/no persistent, dirty/mixed, clean, listener removal.

**Perbaikan:** Saat dirty, persistent calls dijalankan langsung lalu runtime listeners sekali; cache persistent lama diabaikan. Flag dirty dipertahankan sampai Unity membangun cache sendiri.

**Regresi:** event-dirty: runtime-only, cache persistent usang, clean combined calls, removal; runtime managed asli tetap manual.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R07 — P2 — Assignment proxy ke proxy tidak menulis elemen

include/BNM/BasicMonoStructures.hpp:24-65
DataIterator memiliki operator=(T), tetapi implicit copy assignment tetap tersedia.
(*array)[0] = (*array)[1] menyalin pointer proxy sementara, bukan nilai elemen.
Probe proxy-copy: expected 2, actual 1. List operator[] memakai proxy yang sama.
Saran: implementasikan copy assignment proxy yang meneruskan nilai ke StoreManagedValue,
misalnya operator=(const DataIterator& other) { return *this = T(*other); } dengan
penanganan proxy invalid yang konsisten. Tes value/reference dan write barrier.

**Perbaikan:** Copy assignment DataIterator menulis nilai melalui StoreManagedValue; copy constructor tetap menyalin view. Proxy sumber invalid tidak mengubah destination.

**Regresi:** proxy-assignment: Array, List, invalid proxy dan barrier reference store.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R08 — P2 — Field mempertahankan instance lama setelah bind ditolak

src/FieldBase.cpp:33-48
Pada BNM_CHECK_INSTANCE_TYPE (default debug), cabang gagal tidak mereset _instance.
Field yang awalnya valid, dibind nullptr, lalu Set(99) tetap mengubah objek lama.
Probe field-clear: nilai semula 42 menjadi 99. MethodBase sudah membersihkan instance.
Saran: reset _instance sebelum validasi dan samakan kontrak semua wrapper. Test null,
unrelated class, valid rebind, serta Debug/Release.

**Perbaikan:** Checked FieldBase::SetInstance mereset instance sebelum menerima binding baru; nullptr dan tipe tidak cocok tidak mempertahankan objek lama.

**Regresi:** field-rebind: null, unrelated class, valid rebind; build Debug/Release.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R09 — P2 — Penambahan pada delegate tunggal menjadi no-op

src/Delegates.cpp:34-43
GetMethods/Invoke mendukung bentuk single delegate tanpa delegates array, tetapi Add
langsung return untuk bentuk tersebut. Probe delegate-single: expected count 2, actual 1.
Add(MethodBase) bahkan dapat mengembalikan delegate baru yang belum terpasang.
Saran: tangani transisi single -> multicast sambil mempertahankan listener pertama;
tinjau Remove serta kontrak delegate immutable/runtime Combine/Remove.

**Perbaikan:** TryAdd menyediakan hasil sukses/gagal; transisi single ke multicast mengklon delegate awal, menyalin reference dengan barrier, lalu memublikasikan array baru. Remove menangani single dan occurrence terakhir.

**Regresi:** delegate-mutation: alokasi gagal, single -> multi -> single -> empty, clone tidak menunjuk ke objek mutable asal.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R10 — P2 — Matrix3x3 perkalian diri sendiri salah

include/BNM/UnityStructures/Matrix3x3.hpp:65-74
m *= m membaca rhs yang sedang ditimpa. Probe matriks 1..9 menemukan 6 elemen berbeda
dari m*m. Saran: hitung ke temporary sebelum assignment; test alias lhs/rhs.

**Perbaikan:** Matrix3x3::operator*= menghitung hasil ke temporary sebelum assignment.

**Regresi:** math-review: m*=m dibandingkan m*m untuk seluruh elemen.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R11 — P2 — Matrix4x4::rotation salah pada nonuniform scale

include/BNM/UnityStructures/Matrix4x4.hpp:98-113,582
SetTRS memberi skala pada kolom, tetapi rotation menormalkan baris. Round-trip rotasi
z 0.8 rad dengan skala (2,3,4) menghasilkan (z,w)=(0.382884,0.917599), seharusnya
(0.389418,0.921061). Saran: ekstrak basis kolom dan tentukan kontrak untuk reflection,
zero scale, dan shear. Tes round-trip dengan toleransi, bukan equality float mentah.

**Perbaikan:** Rotasi diekstrak dari kolom TRS; reflection ditempatkan pada skala X. Basis singular/non-finite/shear mengembalikan identity sesuai kontrak yang didokumentasikan.

**Regresi:** math-review: skala uniform/nonuniform, reflection dan singular.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R12 — P2 — NormalizeAngle dapat loop tanpa akhir

include/BNM/UnityStructures/Vector3.hpp:185-190
Pengurangan 360 berulang tidak mengubah FLT_MAX karena presisi float. Probe angle-large
berakhir timeout 5 detik. Infinity juga tidak akan konvergen.
Saran: reduksi modulo waktu konstan, cek isfinite, dan dokumentasikan hasil NaN/Inf.

**Perbaikan:** NormalizeAngle memakai fmod dengan finite check; NaN/Inf menghasilkan NaN. Endpoint lama -180, 180, dan 360 tetap dipertahankan.

**Regresi:** math-review: FLT_MAX, infinity, negatif, kelipatan 360 dan endpoint.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R13 — P2 — Audit matrix diam-diam tetap memakai patch 32

tools/audit/baseline.py:123-124
replace mencari '#define UNITY_PATCH_VER 32 ' dengan spasi akhir. GlobalSettings kini
berakhir newline setelah 32, sehingga replacement tidak terjadi. Contoh label 223/50,
6000/57, 6003/3 seluruhnya efektif patch32. Ini regresi tooling akibat perubahan format
dokumentasi pada commit yang baru, bukan bug selector header.
Saran: gunakan parser/regex anchored dengan tepat satu match, assert effective macros,
rekam header yang dipilih, dan gate hasil berdasarkan konfigurasi aktual. Hasil compile
731 sebelumnya memakai script regex independen, sehingga tidak terkena masalah ini.
Bukti: matrix-config.json. Pola replacement lain juga perlu assertion agar gagal jelas.

**Perbaikan:** Penggantian macro wajib tepat satu definisi aktif. Matrix membaca macro efektif dan header terpilih dari preprocessor, menyimpan hash, dan gate memvalidasinya. Input test/tool juga memiliki freshness manifest.

**Regresi:** test_audit_configuration.py, setiap log unity-*-configuration.json, tests/check_results.py.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R14 — P2 — Logging string_view memakai %s tanpa panjang

src/Class.cpp:48,53,60,160,180,202,219,239,257,276,296;
include/BNM/DebugMessages.hpp
String_view tidak mensyaratkan null terminator. Lookup yang gagal memakai name.data()
ke format %s, sehingga substring dapat menampilkan karakter berlebih atau membaca di
luar batas buffer. Ini temuan inspeksi kode; belum direproduksi terhadap liblog.
Saran: logging yang sadar panjang, atau materialisasi std::string untuk error path.
Pisahkan dari string::c_str()/data() yang memang terminasi-null.

**Perbaikan:** Lookup diagnostic mematerialisasi string_view menjadi string berterminator-null pada error path.

**Regresi:** string-view-logging: buffer satu karakter tepat sebelum guard page.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R15 — P3 — Header publik bergantung pada urutan include

36 header .hpp dikompilasi masing-masing sebagai satu-satunya include: 25 lulus,
11 gagal. Contoh Vector2.hpp tidak menyertakan <string>/settings untuk BNM_OBFUSCATE;
Il2CppHeaders.hpp memerlukan definisi uintptr_t dari header lain. Ini tidak menggagalkan
build umbrella saat ini, tetapi membuat integrasi direct include rapuh.
Saran: setiap header menyertakan prasyaratnya sendiri atau dokumentasikan header mana
yang hanya internal. Detail setiap diagnostik disimpan di header-isolation.json.

**Perbaikan:** Header menambahkan include dan forward declaration prasyarat secara langsung. Android.mk juga mengekspor include publik.

**Regresi:** mode headers: 36 header sebagai satu-satunya include; integrasi CMake/ndk-build.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## R16 — P2 — Overflow pertumbuhan List pada header array signed

`include/BNM/BasicMonoStructures.hpp`: header Unity lama memakai `il2cpp_array_size_t`
bertanda. Ekspresi `capacity + capacity / 2` dapat overflow sebelum clamp INT32_MAX,
sementara perbandingan signed/unsigned menutupi kapasitas negatif.

**Perbaikan:** gunakan `std::cmp_*` untuk batas; tolak kapasitas negatif dan lakukan
aritmetika pertumbuhan dalam size_t setelah membuktikan kapasitas lebih kecil dari
ukuran yang diminta (maksimum INT32_MAX).

**Regresi:** `list-growth-limit` memastikan pertumbuhan mendekati INT32_MAX meminta
kapasitas yang sudah di-clamp, gagal alokasi tanpa perubahan list, dan menolak ukuran
melebihi batas. Matrix header lama memeriksa instantiation dengan ukuran signed.

**Status:** selesai; implementasi dan regresi lokal lulus gate snapshot akhir.

## Validasi runtime manual

- [ ] Unity 6000.0.51f1, 6000.0.58f2, 6000.3.19f1: loading, hooks, GC, collections, delegates/events, coroutine.
- [ ] Versi lain per batas perubahan layout dan ABI yang didistribusikan.

## Saran lanjutan (di luar 16 bug)

- [ ] Diagnostic report versi/header/ABI/backend dan tahap gagal.
- [ ] API status kegagalan dan RAII managed GC handles.
- [x] Runtime smoke sample dan otomatisasi runner Android.

## Hasil verifikasi

Snapshot akhir: `build/review-fixes-complete/`. Log compiler, sanitizer, matrix dan gate disimpan di folder tersebut.

Gate dijalankan ulang pada 2026-10-20 dengan
`python3 tests/check_results.py build/review-fixes-complete`: **PASS**, seluruh
404 input produksi cocok dengan manifest snapshot dan input verifikasi masih sesuai.
Hasil tersimpan yang diperiksa gate mencakup:

- Build Debug/Release pada empat ABI Android.
- 36 kasus regresi native dengan ASan/UBSan dan empat kasus scanner.
- 38 konfigurasi versi/patch Unity beserta identitas header terpilih.
- 36 header publik secara terpisah, pemetaan tipe pada empat ABI, dan tes konfigurasi audit.
- Varian fitur, kontrol negatif selector unsupported/backend belum dipilih,
  integrasi consumer CMake serta ndk-build Debug/Release.
- Analyzer pada 17 translation unit, semuanya exit 0. Satu warning unik berulang
  di seluruh unit: padding `Il2CppInternalThread` pada header `2022.2.9f1.h`.
  Warning ditinjau dan layout dipertahankan karena struktur mengikuti ABI IL2CPP;
  mengurutkan ulang field demi padding akan mengubah offset runtime.

Empat pemeriksaan Release tambahan (`field-rebind`, `proxy-assignment`,
`math-review`, `list-growth-limit`) juga memiliki exit 0 pada
`release-review-results.json`. Pemeriksaan `git diff --check` lulus.

Checklist runtime manual tetap terbuka: hasil lokal tidak memvalidasi GC, hook
backend nyata, atau perilaku managed di game Unity.
