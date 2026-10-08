# TCC installation and updates

`apps/compiled` receives only the TCC build's binaries:
`bin/tcc` and `lib/libmos.a`. Building TCC does not copy headers, examples,
newlib, Pico SDK, C++ files or library source trees into that directory.
The runtime archive is built from the existing `libs/runtime` target.

`apps/tcc/install.json` is the explicit source-to-destination declaration.
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
and compatibility with TCC have been checked. This initial profile contains
stdio, stdlib, ctype, libgen, errno and three compiler headers; it is not a
claim that all MOS libc/POSIX interfaces are ready for TCC.

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
