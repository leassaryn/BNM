#!/usr/bin/env python3
"""Read-only production audit; generated snapshots/logs live under build/.

The only staged source change selects the existing Dobby configuration.
The supplied Dobby stub ALWAYS FAILS and must never be used in a game.
"""
import argparse
import concurrent.futures
import difflib
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', default='build/audit-phase23')
parser.add_argument('--cmake', default=shutil.which('cmake'))
parser.add_argument('--ninja', default=shutil.which('ninja'))
parser.add_argument('--mode', choices=['prepare', 'build', 'matrix', 'probes', 'integration', 'analyze', 'scanner', 'variants', 'regression'], default='prepare')
args = parser.parse_args()
OUT = (ROOT / args.out).resolve()
if not OUT.is_relative_to((ROOT / 'build').resolve()):
    parser.error('--out must be inside repository build/')
OUT.mkdir(parents=True, exist_ok=True)
NDK = Path(os.environ['ANDROID_NDK_HOME'])
host = 'linux-arm64' if os.uname().machine == 'aarch64' else 'linux-x86_64'
BIN = NDK / 'toolchains/llvm/prebuilt' / host / 'bin'
SNAP = OUT / 'snapshot'
LOG = OUT / 'logs'
LOG.mkdir(exist_ok=True)


def run(name, argv, cwd=ROOT, timeout=300, env=None):
    start = time.monotonic()
    argv = list(map(str, argv))
    try:
        proc = subprocess.run(argv, cwd=cwd, stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, timeout=timeout, env=env)
        code, output = proc.returncode, proc.stdout.decode(errors='replace')
    except (OSError, subprocess.TimeoutExpired) as exc:
        code, output = 125, str(exc)
    record = dict(name=name, command=argv, cwd=str(cwd), exit_code=code,
                  seconds=round(time.monotonic()-start, 2),
                  warnings=output.count('warning:'), errors=output.count('error:'))
    (LOG / (name+'.log')).write_text(output)
    (LOG / (name+'.json')).write_text(json.dumps(record, indent=2)+'\n')
    print(json.dumps(record), flush=True)
    return code


def includes(tree):
    return ['-I'+str(tree / p) for p in
            ['include', 'external/include', 'external', 'external/utf8', 'src/private']] + ['-I'+str(OUT/'stub')]


if args.mode == 'prepare':
    if SNAP.exists():
        parser.error('snapshot already exists; use another --out to prepare a fresh baseline')
    run('compiler-version', [BIN/'aarch64-linux-android21-clang++', '--version'])
    run('original-syntax', [BIN/'aarch64-linux-android21-clang++', '-std=c++20', '-fsyntax-only',
                            *includes(ROOT), ROOT/'src/Class.cpp'])
    SNAP.mkdir()
    for name in ['src', 'include', 'external']:
        shutil.copytree(ROOT/name, SNAP/name)
    for name in ['CMakeLists.txt', 'Android.mk']:
        shutil.copy2(ROOT/name, SNAP/name)
    settings = SNAP/'include/BNM/UserSettings/GlobalSettings.hpp'
    before = settings.read_text()
    start = before.index('// Dobby\n/*\n')
    end = before.index('*/', start)
    dummy = before.index('#include <dlfcn.h>', end)
    after = before[:start] + '// Dobby (audit snapshot only)\n' + before[start+len('// Dobby\n/*\n'):end] + '\n' + before[dummy:]
    settings.write_text(after)
    (OUT/'settings.patch').write_text(''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True), fromfile='original/GlobalSettings.hpp', tofile='snapshot/GlobalSettings.hpp')))
    (OUT/'stub').mkdir()
    (OUT/'stub/dobby.h').write_text('#pragma once\n// AUDIT ONLY: deliberately unsuccessful, no runtime hooks.\ninline int DobbyHook(void*, void*, void**) { return -1; }\ninline int DobbyDestroy(void*) { return -1; }\n')
    manifest = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                for name in ['src', 'include', 'external'] for p in (ROOT/name).rglob('*') if p.is_file()}
    (OUT/'source-sha256.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('Prepared '+str(SNAP))
elif args.mode == 'build':
    if not args.cmake or not args.ninja:
        parser.error('--cmake and --ninja required when not on PATH')
    def build(pair):
        abi, config = pair
        name = abi+'-'+config.lower()
        dest = OUT/name
        cmd = [args.cmake, '-S', SNAP, '-B', dest, '-G', 'Ninja',
               '-DCMAKE_MAKE_PROGRAM='+args.ninja,
               '-DCMAKE_TOOLCHAIN_FILE='+str(NDK/'build/cmake/android.toolchain.cmake'),
               '-DANDROID_ABI='+abi, '-DANDROID_PLATFORM=android-21',
               '-DCMAKE_BUILD_TYPE='+config, '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
               '-DCMAKE_CXX_FLAGS=-I'+str(OUT/'stub')]
        if run(name+'-configure', cmd) == 0:
            run(name+'-build', [args.cmake, '--build', dest, '--parallel', '2'])
    pairs = [(abi, cfg) for abi in ['arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64'] for cfg in ['Debug', 'Release']]
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(build, pairs))
elif args.mode == 'matrix':
    # Public API compile at every selector branch. Full source checks at six
    # representative old/current/new layouts; this is not a full cross-product.
    versions = [(v, 0) for v in [56,171,172,181,182,183,191,193,194,201,202,203,212,213]]
    versions += [(211,0),(211,24),(221,0),(221,8),(222,0),(222,9),
                 (223,0),(223,8),(223,33),(231,0),(231,10),(232,0),
                 (6000,0),(6000,10),(6000,44),(6000,57),(6001,0),
                 (6002,0),(6002,2),(6003,0),(6003,3),(6004,0),(6005,0),(6005,1)]
    base = (SNAP/'include/BNM/UserSettings/GlobalSettings.hpp').read_text()
    def check(pair):
        version, patch = pair
        name = f'unity-{version}-{patch}'
        config = OUT/'matrix'/name
        config.mkdir(parents=True, exist_ok=True)
        # Separate header tree ensures relative includes use variant settings.
        tree = config/'include'
        if not tree.exists():
            shutil.copytree(SNAP/'include', tree, ignore=shutil.ignore_patterns('Il2CppHeaders'))
            (tree/'BNM/Il2CppHeaders').symlink_to(SNAP/'include/BNM/Il2CppHeaders', target_is_directory=True)
        settings = tree/'BNM/UserSettings/GlobalSettings.hpp'
        changed = base.replace('#define UNITY_VER 222 //', f'#define UNITY_VER {version} //', 1)
        changed = changed.replace('#define UNITY_PATCH_VER 32 ', f'#define UNITY_PATCH_VER {patch} ', 1)
        settings.write_text(changed)
        sources = sorted((SNAP/'src').glob('*.cpp')) if pair in [(56,0),(172,0),(211,24),(222,9),(6003,3),(6005,1)] else []
        run(name, [BIN/'aarch64-linux-android21-clang++', '-std=c++20', '-fsyntax-only',
                   '-Wall', '-Wextra', '-Wpedantic', '-I'+str(tree), *includes(SNAP),
                   *sources, ROOT/'tools/audit/public_api.cpp'], timeout=300)
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(check, versions))
elif args.mode in ['probes', 'regression']:
    if not args.cmake or not args.ninja:
        parser.error('--cmake and --ninja required')
    dest = OUT/'sanitized'
    if run('sanitized-configure', [args.cmake, '-S', SNAP, '-B', dest, '-G', 'Ninja',
           '-DCMAKE_MAKE_PROGRAM='+args.ninja,
           '-DCMAKE_TOOLCHAIN_FILE='+str(NDK/'build/cmake/android.toolchain.cmake'),
           '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-21',
           '-DCMAKE_BUILD_TYPE=Debug', '-DBNM_SANITIZERS=ON',
           '-DCMAKE_CXX_FLAGS=-I'+str(OUT/'stub')]) == 0:
        if run('sanitized-build', [args.cmake, '--build', dest, '--parallel', '2']) == 0:
            compiler = BIN/'aarch64-linux-android21-clang++'
            flags = ['-std=c++20', '-O1', '-g', '-fsanitize=address,undefined',
                     '-fno-sanitize-recover=all', '-fno-omit-frame-pointer',
                     '-static-libstdc++', *includes(SNAP)]
            exe = OUT/'probes'
            if args.mode == 'regression':
                flags += ['-Wl,--wrap=malloc']
            if run('probes-compile', [compiler, *flags, ROOT/('tests/core_regressions.cpp' if args.mode == 'regression' else 'tools/audit/probes.cpp'),
                                     dest/'libBNM.a', '-llog', '-ldl', '-o', exe]) == 0:
                runtime = subprocess.check_output([str(compiler), '-print-file-name=libclang_rt.asan-aarch64-android.so'], text=True).strip()
                env = os.environ.copy()
                env['LD_LIBRARY_PATH'] = str(Path(runtime).parent)+':'+env.get('LD_LIBRARY_PATH','')
                env['ASAN_OPTIONS'] = 'detect_leaks=0:detect_stack_use_after_return=1'
                env['UBSAN_OPTIONS'] = 'print_stacktrace=1'
                env['ASAN_SYMBOLIZER_PATH'] = str(BIN/'llvm-symbolizer')
                cases = ['positive', 'copy-lifetime', 'const-generic', 'class-publication',
                             'dictionary-holes', 'array-overflow', 'coroutine-yield',
                             'coroutine-temporary', 'missing-symbol', 'hook-failure',
                             'custom-method-limit', 'mapped-not-readable']
                if args.mode == 'regression':
                    cases += ['reflection', 'copy-move', 'list-boundaries', 'layout-rejection', 'jni-errors', 'invoke-hook', 'custom-class', 'metadata-oom', 'virtual-dispatch', 'thread-scope', 'write-barriers', 'metadata-rollback-create', 'metadata-rollback-modify', 'metadata-hook-rollback', 'list-metadata-rollback']
                failed = [case for case in cases if run(('regression-' if args.mode == 'regression' else 'probe-')+case, [exe, case], timeout=20, env=env)]
                if args.mode == 'regression' and failed:
                    raise SystemExit(1)
            elif args.mode == 'regression':
                raise SystemExit(1)
        elif args.mode == 'regression':
            raise SystemExit(1)
    elif args.mode == 'regression':
        raise SystemExit(1)
elif args.mode == 'integration':
    # Build a real consumer shared library; stub only satisfies hook symbols.
    project = OUT/'consumer'
    project.mkdir(exist_ok=True)
    (project/'consumer.cpp').write_text('#include <BNM/Loading.hpp>\nextern "C" bool audit_load(void* handle) { return BNM::Loading::TryLoadByDlfcnHandle(handle); }\n')
    (project/'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.14)\nproject(AuditConsumer LANGUAGES CXX)\nadd_subdirectory("'+str(SNAP)+'" bnm)\nadd_library(audit_consumer SHARED consumer.cpp)\ntarget_link_libraries(audit_consumer PRIVATE BNM log dl)\n')
    dest = OUT/'consumer-build'
    if run('consumer-configure', [args.cmake, '-S', project, '-B', dest, '-G', 'Ninja',
           '-DCMAKE_MAKE_PROGRAM='+args.ninja,
           '-DCMAKE_TOOLCHAIN_FILE='+str(NDK/'build/cmake/android.toolchain.cmake'),
           '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-21', '-DCMAKE_BUILD_TYPE=Debug',
           '-DCMAKE_CXX_FLAGS=-I'+str(OUT/'stub')]) == 0:
        run('consumer-build', [args.cmake, '--build', dest, '--parallel', '2'])
    ndk_project = OUT/'ndk-project'
    (ndk_project/'jni').mkdir(parents=True, exist_ok=True)
    (ndk_project/'jni/Android.mk').write_text('include '+str(SNAP/'Android.mk')+'\n')
    (ndk_project/'jni/Application.mk').write_text('APP_ABI := arm64-v8a\nAPP_PLATFORM := android-21\nAPP_STL := c++_static\nAPP_CPPFLAGS += -fexceptions -frtti -I'+str(OUT/'stub')+'\n')
    for config in ['debug', 'release']:
        run('ndk-'+config, [NDK/'ndk-build', 'NDK_PROJECT_PATH='+str(ndk_project),
                           'NDK_OUT='+str(OUT/('ndk-'+config)), 'APP_OPTIM='+config, '-j2'])
elif args.mode == 'analyze':
    def analyze(source):
        run('analyze-'+source.stem, [BIN/'clang-tidy', source, '-p', OUT/'arm64-v8a-debug',
            '--checks=-*,clang-analyzer-*', '--header-filter=.*', '--quiet'], timeout=120)
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        list(pool.map(analyze, sorted((SNAP/'src').glob('*.cpp'))))
elif args.mode == 'scanner':
    # Extract the actual scanner; rename architecture condition symbols only so
    # x86 logic can be exercised on this ARM64 host. No algorithm changes.
    loading = (SNAP/'src/Loading.cpp').read_text()
    scanner = loading[loading.index('namespace AssemblerUtils {'):loading.index('void Internal::LateInit')]
    for old, new in [('__ARM_ARCH_7A__','AUDIT_ARMV7'),('__aarch64__','AUDIT_ARM64'),
                     ('__i386__','AUDIT_X86'),('__x86_64__','AUDIT_X64')]:
        scanner = scanner.replace('defined('+old+')', 'defined('+new+')')
    source = OUT/'scanner.cpp'
    source.write_text('#include <cstdint>\n#include <cstring>\n#include <string>\n#include <cstdlib>\n#include <cstdio>\n#include <sys/mman.h>\n#include <unistd.h>\nnamespace BNM { size_t ReadableBytes(const void*, size_t); }\nusing BNM_PTR = uintptr_t;\n#define BNM_OBFUSCATE_TMP(x) x\n#define AUDIT_X64\n'+scanner+'''
int main(int argc, char** argv) {
    if (argc != 2) return 64;
    if (std::string(argv[1]) == "rel32") {
        BNM_PTR target = 0;
        AssemblerUtils::DecodeBranchOrCall("E8FBFFFFFF", 0x1000, target);
        std::printf("expected=0x1000 actual=0x%zx\\n", static_cast<size_t>(target));
        return target == 0x1000 ? 0 : 1;
    }
    if (std::string(argv[1]) == "bounds") {
        const auto page = static_cast<size_t>(getpagesize());
        auto *memory = static_cast<unsigned char *>(mmap(nullptr, page * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (memory == MAP_FAILED) return 77;
        if (mprotect(memory + page, page, PROT_NONE)) return 77;
        memory[page - 1] = 0xE8;
        const auto tail = reinterpret_cast<BNM_PTR>(memory + page - 4);
        const bool safe = AssemblerUtils::FindNextJump(0, 1) == 0 && AssemblerUtils::FindNextJump(tail, 1) == 0;
        munmap(memory, page * 2);
        return safe ? 0 : 1;
    }
    const unsigned char instruction[] = {0xE8};
    auto hex = AssemblerUtils::ReadMemory<1>(reinterpret_cast<BNM_PTR>(instruction));
    std::printf("expected=E8 actual=%s\\n", hex.c_str());
    return hex == "E8" ? 0 : 1;
}
''')
    compiler = BIN/'aarch64-linux-android21-clang++'
    exe = OUT/'scanner-probes'
    if run('scanner-compile', [compiler, '-std=c++20', '-O1', '-g', '-fsigned-char',
           '-static-libstdc++', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', source, OUT/'sanitized/libBNM.a', '-llog', '-ldl', '-o', exe]) == 0:
        runtime = subprocess.check_output([str(compiler), '-print-file-name=libclang_rt.asan-aarch64-android.so'], text=True).strip()
        env = os.environ.copy()
        env['LD_LIBRARY_PATH'] = str(Path(runtime).parent)+':'+env.get('LD_LIBRARY_PATH','')
        env['ASAN_OPTIONS'] = 'detect_leaks=0'
        env['ASAN_SYMBOLIZER_PATH'] = str(BIN/'llvm-symbolizer')
        for case in ['rel32', 'signed-char', 'bounds']:
            run('scanner-'+case, [exe, case], env=env)
elif args.mode == 'variants':
    base = (SNAP/'include/BNM/UserSettings/GlobalSettings.hpp').read_text()
    for name in ['sync', 'dotnet35', 'minimal', 'unsupported-6006']:
        tree = OUT/'variants'/name/'include'
        if not tree.exists():
            shutil.copytree(SNAP/'include', tree, ignore=shutil.ignore_patterns('Il2CppHeaders'))
            (tree/'BNM/Il2CppHeaders').symlink_to(SNAP/'include/BNM/Il2CppHeaders', target_is_directory=True)
        config = base
        if name == 'sync':
            config = config.replace('// #define BNM_ALLOW_MULTI_THREADING_SYNC', '#define BNM_ALLOW_MULTI_THREADING_SYNC', 1)
        elif name == 'dotnet35':
            config = config.replace('// #define BNM_DOTNET35', '#define BNM_DOTNET35', 1)
        elif name == 'minimal':
            config = config.replace('#define BNM_CLASSES_MANAGEMENT\n', '// audit: classes disabled\n', 1)
            config = config.replace('#define BNM_COROUTINE\n', '// audit: coroutine disabled\n', 1)
        else:
            config = config.replace('#define UNITY_VER 222 //', '#define UNITY_VER 6006 //', 1)
        (tree/'BNM/UserSettings/GlobalSettings.hpp').write_text(config)
        sources = sorted((SNAP/'src').glob('*.cpp')) if name in ['sync', 'minimal'] else []
        run('variant-'+name, [BIN/'aarch64-linux-android21-clang++', '-std=c++20', '-fsyntax-only',
            '-Wall', '-Wextra', '-Wpedantic', '-Wno-dollar-in-identifier-extension', '-Wno-zero-length-array',
            '-I'+str(tree), *includes(SNAP), *sources, ROOT/'tools/audit/public_api.cpp', ROOT/'tools/audit/extended_api.cpp'])
