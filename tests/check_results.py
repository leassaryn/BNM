#!/usr/bin/env python3
"""Require complete, fresh verification; expected negative controls are explicit."""
import hashlib
import json
from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]
out = (root / (sys.argv[1] if len(sys.argv) > 1 else 'build/refactor-verified')).resolve()
records = {p.stem: json.loads(p.read_text()) for p in (out / 'logs').glob('*.json')}
failures = []


def require(name, expected=0):
    item = records.get(name)
    if item is None or item['exit_code'] != expected:
        failures.append(f'{name}: expected exit {expected}, got {None if item is None else item["exit_code"]}')


for abi in ['arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64']:
    for config in ['debug', 'release']:
        for stage in ['configure', 'build']:
            require(f'{abi}-{config}-{stage}')
for name in ['sanitized-configure', 'sanitized-build', 'probes-compile', 'scanner-compile',
             'consumer-configure', 'consumer-build', 'ndk-debug', 'ndk-release']:
    require(name)
for case in ['positive', 'copy-lifetime', 'const-generic', 'class-publication',
             'dictionary-holes', 'array-overflow', 'coroutine-yield', 'coroutine-temporary',
             'missing-symbol', 'hook-failure', 'custom-method-limit', 'mapped-not-readable',
             'reflection', 'copy-move', 'list-boundaries', 'layout-rejection', 'jni-errors',
             'invoke-hook', 'custom-class', 'metadata-oom', 'virtual-dispatch', 'thread-scope', 'gc-handle', 'write-barriers', 'metadata-rollback-create', 'metadata-rollback-modify', 'metadata-hook-rollback', 'runtime-list', 'proxy-assignment', 'field-rebind', 'math-review', 'raycast-resolver', 'event-dirty', 'delegate-mutation', 'string-view-logging', 'x86-decoder', 'list-growth-limit']:
    require('regression-' + case)
for case in ['rel32', 'signed-char', 'bounds', 'operand']:
    require('scanner-' + case)
for header in sorted((root / 'include/BNM').rglob('*.hpp')):
    require('header-' + header.relative_to(root / 'include').as_posix().replace('/', '-'))
for triple in ['aarch64-linux-android21', 'armv7a-linux-androideabi21', 'i686-linux-android21', 'x86_64-linux-android21']:
    require('abi-types-' + triple)
require('audit-configuration')
for case in ['sync' , 'dotnet35', 'minimal']:
    require('variant-' + case)
require('variant-unsupported-6006', 1)
require('original-syntax', 1)
versions = [(v, 0) for v in [56,171,172,181,182,183,191,193,194,201,202,203,212,213]]
versions += [(211,0),(211,24),(221,0),(221,8),(222,0),(222,9),(223,0),(223,8),
             (223,33),(231,0),(231,10),(232,0),(6000,0),(6000,10),(6000,44),
             (6000,57),(6001,0),(6002,0),(6002,2),(6003,0),(6003,3),(6004,0),(6005,0),(6005,1)]
for version, patch in versions:
    require(f'unity-{version}-{patch}')
    configuration = records.get(f'unity-{version}-{patch}-configuration', {})
    if configuration.get('requested') != [version, patch] or configuration.get('effective') != [version, patch]:
        failures.append(f'unity-{version}-{patch}: effective configuration missing or incorrect')
    header = configuration.get('header', '')
    path = root / 'include/BNM/Il2CppHeaders' / header
    if not header or not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != configuration.get('sha256'):
        failures.append(f'unity-{version}-{patch}: selected header missing or stale')
for source in (root / 'src').glob('*.cpp'):
    require('analyze-' + source.stem)
if (out / 'review-history').exists():
    # An incrementally refreshed snapshot must not reuse pre-refresh build results.
    refreshed = (out / 'source-sha256.json').stat().st_mtime_ns
    refreshed_names = [name for name in records if name.endswith('-build') or
                       name.startswith(('regression-', 'review-'))]
    refreshed_names += ['probes-compile', 'ndk-debug', 'ndk-release', 'analyze-Coroutine']
    for name in refreshed_names:
        result = out / 'logs' / (name + '.json')
        if result.exists() and result.stat().st_mtime_ns < refreshed:
            failures.append('result predates reviewed source refresh: ' + name)
    for name in ['unity-56-0', 'unity-172-0', 'unity-211-24', 'unity-222-9',
                 'unity-6003-3', 'unity-6005-1', 'variant-sync', 'variant-minimal']:
        require('review-' + name)
verification_path = out / 'verification-sha256.json'
if not verification_path.exists():
    failures.append('missing verification input manifest')
else:
    for name, digest in json.loads(verification_path.read_text()).items():
        if not (root / name).is_file() or hashlib.sha256((root / name).read_bytes()).hexdigest() != digest:
            failures.append('stale verification input: ' + name)
manifest = json.loads((out / 'source-sha256.json').read_text())
for name, digest in manifest.items():
    if hashlib.sha256((root / name).read_bytes()).hexdigest() != digest:
        failures.append('stale production input: ' + name)
    if name != 'include/BNM/UserSettings/GlobalSettings.hpp':
        if hashlib.sha256((out / 'snapshot' / name).read_bytes()).hexdigest() != digest:
            failures.append('modified snapshot input: ' + name)
for name in ['CMakeLists.txt', 'Android.mk']:
    if (root / name).read_bytes() != (out / 'snapshot' / name).read_bytes():
        failures.append('stale build input: ' + name)
if failures:
    print('\n'.join(failures))
    sys.exit(1)
print(f'PASS: required results observed; {len(manifest)} production inputs match the snapshot manifest.')
print('Analyzer warnings and real Unity/GC/backend behavior still require review; this gate does not certify them.')
