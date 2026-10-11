# TCC installation and updates

`apps/compiled` receives only the TCC build's binaries:
`bin/tcc` and `lib/libmos.a`. Building TCC does not copy headers, examples,
newlib, Pico SDK, C++ files or library source trees into that directory.
The runtime archive and its publication are owned by `libs/runtime`.

`api/install.json` declares SDK files; `apps/tcc/install.json` declares TCC,
its example and the exact source set used for self-compilation.
`install_tcc.py` installs both explicit manifests.
Source paths are relative to the repository root; destinations are relative
to the MOS2 directory. No directory recursion or wildcard is used.
MOS headers are read directly from `api`, not maintained as duplicate copies.
`stddef.h`, `stdarg.h` and `stdbool.h` are the original TinyCC headers from
`tinycc-arm-thumb/include`; GCC's compiler headers cannot replace them.

## Install

Build the default target in `apps/tcc`, then run with Python 3.9 or newer
from the repository root (E: is the example SD-card drive):

```
python tools/install_tcc.py --dest E:/mos2 --dry-run
python tools/install_tcc.py --dest E:/mos2
```

The same command can target a separate release directory. It never stages
headers in `apps/compiled`. Use the default CMake build target, which includes
`tcc_binaries`; building only the `tcc` executable target does not publish it.

## Update after changing MOS2

Update and build MOS2 and TCC from the same repository revision, then repeat
the installation command. It rereads current API files using the manifest.
Changed files are replaced; unchanged files are not recopied. An installed
`.tcc-install.json` records file hashes. When an entry is removed from the
manifest, the next update removes that previously installed file only if it
still matches its recorded hash. Unrelated files are never swept or removed.
Local edits to managed files stop the update before any copies or removals.
Save edited example programs under another filename.

There is no guarantee that headers from one MOS2 revision match an older
running kernel. The installer does not flash the kernel or detect its version.
New API entries must be added to the manifest only after their dependencies
and compatibility with TCC have been checked. The declared profile contains
the C headers and the file/time POSIX headers required by the compiler; it is
not a claim that every MOS libc/POSIX interface is ready for TCC.

The rejected bulk-copy installer has no ownership record. This installer
will not delete its untracked leftovers. In particular, it does not copy any
existing `apps/compiled/include` tree that an earlier local build left behind.
The build removes the obsolete generated `apps/compiled/tcc`; an old
`/mos2/tcc` on the SD card must be removed separately because directory copying
does not propagate deletion. Use the absolute binary path below.

## First compilation

```
/mos2/bin/tcc -nostdlib -c /mos2/src/hello.c -o /mos2/bin/hello
/mos2/bin/hello one two
```

The example uses the existing MOS `<stdio.h>`. The output is ELF ET_REL;
MOS calls `main` directly. The accompanying compiler fix sets the Thumb bit
on generated function symbols. To explicitly link existing runtime helpers,
use `-nostdlib -r ... -lmos` instead of `-c`, placing the library last.
The archive supplies existing `libs/runtime` functions, not a complete libc.

## Runtime setjmp/longjmp

`libs/runtime/setjmp.S` is part of `murmulator_runtime`, so both the
GCC-built TCC and `libmos.a` use the same implementation. The assembly is
assembled on the development PC when the runtime archive is built; linking
that archive on MOS2 does not require an assembler.

The installer publishes `libs/runtime/setjmp.h` as
`/mos2/include/setjmp.h`. It provides `jmp_buf`, `setjmp` and `longjmp`
with standard linker symbols and no name-mapping macros. Its context contains
r4-r11, SP and LR; this is the existing soft-float implementation, without
VFP register preservation. The move does not change its ABI or instructions.

Rebuild the default TCC target and run the installer to update both
`bin/tcc` and `lib/libmos.a`, together with the header. MOS2 itself does not
need rebuilding. Programs using these functions must link the archive after
their objects, using `-nostdlib -r ... -lmos`.

## Shared standard functions

The standard `strto*`, `ldexp`, `time`, `gettimeofday` and `localtime`
implementations now belong to `libs/runtime` and are included in `libmos.a`.
Their existing limitations are documented in `libs/runtime/README.md`.
TCC no longer supplies private copies or defines `HAS_OWN_STRTOL`.
The existing installation manifest already reads the updated `api/libc/stdlib.h`
directly. Rebuild and repeat installation to update the archive and declarations.
## Self-compilation on MOS2

The installed source tree is `/mos2/src/tcc`. Build from that directory so
the object names in the response files have a stable location:

```
cd /mos2/src/tcc
/mos2/bin/tcc @selfhost-compile.rsp
/mos2/bin/tcc @selfhost-link.rsp
/mos2/bin/tcc-self
```

Compilation and linking are intentionally separate. The first invocation uses
`ONE_SOURCE=0` and writes one object at a time, releasing parser state between
translation units. The second invocation combines those objects with
`/mos2/lib/libmos.a`. This lowers the compilation peak compared with feeding
the complete amalgamated `tcc.c` to one compiler instance. The output has a
different name so a failed self-build cannot overwrite the bootstrap compiler.

The code generator is split into `tccgen.c` (core, value stack, casts),
`tcctype.c` (types and declarators), `tccstruct.c` (enum/struct/union
declarations, `struct_decl`), `tccexpr.c` (expressions) and
`tccstmt.c` (statements, initializers, declarations) with the private header
`tccgen.h`, so no single translation unit of the self-build is as large as the
former 8000-line `tccgen.c`. With `ONE_SOURCE=1` (the CMake build) `libtcc.c`
includes them in this order and the result is the same as the single file.

The response files select `-mfloat-abi=softfp`, matching the core-register ABI
of `libmos.a`. The Thumb backend still emits VFP instructions for floating-point
operations; this is not an RP2040 code generator.

The self-build source manifest is explicit. It contains the C implementation,
private headers, response files and TCC license; it does not copy the rest of
the repository or either SDK.

## SDK without rebuilding TCC

See `libs/runtime/README.md` for the standalone `runtime_binaries` build.
Run `python tools/install_sdk.py --dest I:/mos2` to update the library and
headers alone. The legacy `install_tcc.py` command still installs the complete
SDK plus compiler/example. Headers continue to come directly from the declared
repository paths, not from `apps/compiled/include`.
