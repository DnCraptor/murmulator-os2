# MOS application runtime

The `murmulator_runtime` CMake target supplies application-side implementations.
The TCC build also packages them as `apps/compiled/lib/libmos.a` for installation
at `/mos2/lib/libmos.a`. No new kernel exports are needed.

Standard functions include `setjmp`, `longjmp`, the integer and floating-point
`strto*` conversions, `ldexp`, `time`, `gettimeofday`, `localtime`, and `ftime`.
Use the standard API headers; no `HAS_OWN_STRTOL` definition is required.
For MOS TCC relocatable links use `-nostdlib -r` and place `-lmos` after inputs.

This refactoring preserves the existing implementations and their limitations:

- `setjmp`/`longjmp` preserve core registers only (the existing soft-float ABI).
- `strtod` uses MOS `sscanf`; incomplete exponents and range/errno behavior are
  not yet fully conforming. `strtof` converts through double and may double-round.
- `strtold` requires binary64 `long double`.
- `gettimeofday` provides whole seconds only and rejects a non-null timezone.
- `localtime` converts to UTC, without timezone/DST configuration, and accepts
  calendar years 1 through 9999.
- `time` forwards the existing wall-clock API. It does not initialize the clock.

Compiler-specific SOURCE_DATE_EPOCH handling remains in `apps/tcc/mos_time.c`.

## Independent build and installation

From the repository root, with the same Pico SDK/toolchain environment used
for applications:

```
cmake -S libs/runtime -B build/runtime -DCMAKE_BUILD_TYPE=MinSizeRel
cmake --build build/runtime --target runtime_binaries
python tools/install_sdk.py --dest I:/mos2
```

Use the same board/platform/toolchain options as the application build.
`runtime_binaries` owns publication of `apps/compiled/lib/libmos.a`.
Application builds including this directory also get that default target.
The interface target remains available for existing application consumers.

`api/install.json` declares SDK headers and the archive. `apps/tcc/install.json`
contains only the compiler and example. `install_tcc.py` installs both manifests;
`install_sdk.py` updates only SDK-owned files and retains compiler/example entries
in the shared installation ledger. Neither uses recursive header copying.

The installable headers include strings, math, directory operations, getopt,
fixed-width integers, floating-point limits, alignment and noreturn macros.
The compiler headers are for the MOS ARM ILP32 profile. Additional POSIX headers
that depend on missing newlib types are not yet part of this profile.
