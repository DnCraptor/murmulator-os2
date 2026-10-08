"""Build MOS runtime with an existing application toolchain, then install SDK."""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def read_cache(path):
    values = {}
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        if not line or line.startswith(('#', '//')) or '=' not in line:
            continue
        key, value = line.split('=', 1)
        values[key.split(':', 1)[0]] = value
    return values


def usable(cache):
    compiler = cache.get('CMAKE_C_COMPILER', '')
    sdk = cache.get('PICO_SDK_PATH', '')
    return (compiler and Path(compiler).is_file()
            and 'arm-none-eabi' in Path(compiler).name.lower()
            and sdk and (Path(sdk) / 'pico_sdk_init.cmake').is_file()
            and cache.get('CMAKE_GENERATOR'))


def select_cache(explicit):
    if explicit:
        path = explicit / 'CMakeCache.txt' if explicit.is_dir() else explicit
        cache = read_cache(path)
        if not usable(cache):
            raise ValueError('Cache must reference an existing ARM compiler and Pico SDK: ' + str(path))
        return path, cache
    candidates = []
    for path in ROOT.rglob('CMakeCache.txt'):
        cache = read_cache(path)
        source = Path(cache.get('CMAKE_HOME_DIRECTORY', '')).resolve()
        # Do not reuse the failed standalone runtime configuration.
        if source.parent != (ROOT / 'apps').resolve() or not usable(cache):
            continue
        candidates.append((source.name == 'tcc', path.stat().st_mtime, path, cache))
    if not candidates:
        raise ValueError('No application CMake cache with an existing ARM toolchain found. '
                         'Pass --from-build PATH to the TCC build directory.')
    _, _, path, cache = max(candidates, key=lambda item: item[:2])
    return path, cache


def pico_toolchain_path(cache):
    """Return the toolchain root expected by Pico SDK find_compiler.cmake."""
    configured = cache.get('PICO_TOOLCHAIN_PATH')
    if configured:
        return configured
    compiler = Path(cache['CMAKE_C_COMPILER'])
    # CMAKE_C_COMPILER points to <toolchain>/bin/arm-none-eabi-gcc[.exe].
    root = compiler.parent.parent
    if compiler.parent.name.lower() != 'bin' or not compiler.is_file():
        raise ValueError('Cannot derive PICO_TOOLCHAIN_PATH from compiler: '
                         + str(compiler))
    return str(root)


def run(command):
    print(subprocess.list2cmdline([str(x) for x in command]), flush=True)
    subprocess.run(command, cwd=ROOT, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dest', required=True, type=Path)
    parser.add_argument('--from-build', type=Path,
                        help='existing application build directory or CMakeCache.txt')
    args = parser.parse_args()
    try:
        path, cache = select_cache(args.from_build)
        print('Toolchain configuration: ' + str(path), flush=True)
        cmake = cache.get('CMAKE_COMMAND') or 'cmake'
        # Separate directory: never overwrite the user's failed build/runtime cache.
        build = ROOT / 'build' / 'runtime-sdk'
        keys = ('PICO_SDK_PATH', 'PICO_TOOLCHAIN_PATH', 'PICO_PLATFORM',
                'PICO_BOARD', 'PICO_COMPILER', 'PICO_TOOLCHAIN_TRIPLE',
                'CMAKE_TOOLCHAIN_FILE', 'CMAKE_C_COMPILER', 'CMAKE_CXX_COMPILER',
                'CMAKE_ASM_COMPILER', 'CMAKE_MAKE_PROGRAM')
        settings = {key: cache[key] for key in keys if cache.get(key)}
        # Pico SDK 1.x resolves the compiler inside its toolchain file before
        # CMAKE_C_COMPILER takes effect. It requires this root explicitly.
        settings['PICO_TOOLCHAIN_PATH'] = pico_toolchain_path(cache)
        previous = build / 'CMakeCache.txt'
        if previous.exists():
            prior = read_cache(previous)
            mismatch = [key for key, value in settings.items()
                        if prior.get(key) and not prior[key].endswith('-NOTFOUND')
                        and prior[key] != value]
            if prior.get('CMAKE_GENERATOR') != cache['CMAKE_GENERATOR'] or mismatch:
                raise ValueError('Toolchain changed; move build/runtime-sdk aside before rebuilding. '
                                 'No installation performed.')
        command = [cmake, '-S', str(ROOT / 'libs' / 'runtime'), '-B', str(build),
                   '-G', cache['CMAKE_GENERATOR'], '-DCMAKE_BUILD_TYPE=MinSizeRel']
        for flag, key in (('-A', 'CMAKE_GENERATOR_PLATFORM'), ('-T', 'CMAKE_GENERATOR_TOOLSET')):
            if cache.get(key):
                command.extend([flag, cache[key]])
        command.extend('-D' + key + '=' + value for key, value in settings.items())
        run(command)
        run([cmake, '--build', str(build), '--config', 'MinSizeRel',
             '--target', 'runtime_binaries'])
        run([sys.executable, str(ROOT / 'tools' / 'install_sdk.py'),
             '--dest', str(args.dest.resolve())])
    except subprocess.CalledProcessError as error:
        print('Stopped after command failure (exit %d).' % error.returncode, file=sys.stderr)
        return error.returncode or 1
    except (OSError, ValueError) as error:
        print(str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
