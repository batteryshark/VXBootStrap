# VXBootStrap

A small Windows program that starts a process suspended and loads a list of DLLs into it
before the program's entry point runs.

It's the "vxbootstrap" piece from
[part five of the Masterpiece write-up](https://batteryshark.github.io/2021/10/25/masterpiece-creating-an-app-controller-part-five.html),
the part where the VX app controller gets put together.

## Why this exists

VX runs packaged Windows apps inside a compatibility layer called
[pdx](https://github.com/batteryshark/pdx): a set of DLLs that hook system calls so the app
sees the filesystem and environment we want it to see. Hooks like that only help if they're
in place before the app opens its first file, so something has to get those DLLs into the
process right at the start. That's this program.
[vxlauncher](https://github.com/batteryshark/vxlauncher) works out which libraries an app
needs and hands them to the bootstrap along with the executable.

To be plain about what it is: a DLL loader for my own compatibility layer. It only works on
a process you already have full access to, and it makes no attempt to hide what it's doing.

## How it works

`vxcmd=start` creates the target with `CREATE_SUSPENDED`. At that point the only DLL
mapped is ntdll (the 64-bit one, plus the 32-bit one for a WoW64 app), so there's no
kernel32 yet and no `LoadLibrary` to call. Instead the bootstrap writes three things into
the target: each library path as a `UNICODE_STRING`, a struct holding those strings and the
address of ntdll's `LdrLoadDll`, and a loader stub of about a hundred bytes that walks the
struct and calls `LdrLoadDll` once per library. Then it queues the stub as a user APC on the
main thread and resumes it, so the libraries load while the thread is still initialising and
the stub never depends on anything but ntdll.

The stub is precompiled. `src/vxbootstrap.cpp` carries it as two byte arrays, one x86 and one
x64, with the original C function kept in a comment above them. The `LdrLoadDll` address
comes from the bootstrap's own copy of ntdll, which is why there are two builds: the
bootstrap has to be the same bitness as the target.

`vxcmd=inject` does the same thing to a process that already exists, given a pid and a
thread id. That one is for child processes: pdx's `pdxproc` catches process creation, holds
the child suspended and calls the bootstrap on it. With `vxls=0` the stub is queued as an APC
and the thread is resumed, same as `start`. With `vxls=1` the caller wants the process left
suspended, so the stub runs on a new thread from `CreateRemoteThread` and the original
thread is not touched.

A library name ending in `.dlldynamic` becomes `32.dll` or `64.dll` depending on the target,
so one list (`pdxfs.dlldynamic`) covers both architectures.

## Build

You need MinGW-w64 with both compilers on your `PATH` (`x86_64-w64-mingw32-gcc` and
`i686-w64-mingw32-gcc`). Cross-compiling from macOS works; Linux with the same toolchain
should too.

```sh
make          # bin/VXBootstrap64.exe and bin/VXBootstrap32.exe
make clean
```

libstdc++ and libgcc are linked in statically. With a UCRT-based MinGW-w64 (Homebrew's is),
the only runtime dependency is the Universal CRT, which ships with Windows 10 and later, so
there's nothing to ship next to them there; Windows 7 and 8.1 need the UCRT update. An
msvcrt-based toolchain links `msvcrt.dll` instead. Keep the output names as they are:
vxlauncher and pdx look for exactly `VXBootstrap32.exe` and `VXBootstrap64.exe`.

## Usage

```text
VXBootstrap64.exe vxcmd=start  vxexe=PATH_TO_EXE vxlibs=LIB1;LIB2 [vxwd=PATH_TO_WD] [ARGS...]
VXBootstrap64.exe vxcmd=inject vxlibs=LIB1;LIB2 vxpid=PID vxtid=TID vxls=0|1
```

| Argument | Meaning |
| --- | --- |
| `vxcmd=start` | Start a new process. This is the default if `vxcmd` is left out. |
| `vxcmd=inject` | Load into an existing process, by pid and thread id. |
| `vxexe` | Path to the target executable. |
| `vxlibs` | Semicolon-separated list of libraries to load, in order. |
| `vxwd` | Working directory for the target. Defaults to the folder the executable is in; if `vxexe` has no backslash in it, the target inherits the bootstrap's working directory. |
| `vxpid`, `vxtid` | Target process id and thread id, for `inject`. |
| `vxls` | `1` leaves the target suspended after loading, `0` resumes it. |
| anything else | Passed through to the target as its arguments. |

For example:

```text
VXBootstrap32.exe vxcmd=start vxexe="C:\Games\Example\game.exe" vxlibs=C:\vx\pdxproc.dlldynamic;C:\vx\pdxfs.dlldynamic -windowed
```

The bootstrap doesn't wait for the target. It exits with 0 once the load has been queued,
or -1 on any failure, bad arguments included. It's built as a GUI-subsystem program so it
doesn't flash a console window on every launch; set the `PDXDBG` environment variable and
it allocates one.

## Windows versions and architectures

x86 and x64 targets on 64-bit Windows. Use `VXBootstrap32.exe` for a 32-bit target and
`VXBootstrap64.exe` for a 64-bit one; nothing checks this for you. No ARM64.

The `.dlldynamic` rule assumes a 64-bit OS. On 32-bit Windows nothing is a WoW64 process, so
it would pick `64.dll` for everything.

The original binaries were released in September 2021 and aren't included here. It hasn't been re-tested on a current Windows.

## Status and limits

It's complete for what VX needed, and it's small: one source file and a string helper. The
source here is the 2021 code after a cleanup pass. It closes leaked handles, adds the
library-count check, fixes the size passed to the path conversion and checks its result,
stops freeing the caller's working-directory string, points stdout and stderr at the
`PDXDBG` console so the messages actually show up there, and swaps the borrowed string
helpers for its own. Both builds compile with MinGW-w64 GCC 16.2. The cleaned-up version
hasn't been run on Windows yet.

Two places where this program does less than the write-up suggests:

- **No watchdog callout.** The write-up mentions an optional named-pipe message that
  registers the new process id with the watchdog. There's no pipe code in this program;
  that lives in the .NET bootstrap in [VXTools.NET](https://github.com/batteryshark/VXTools.NET).
- **Libraries only come from `vxlibs=`.** `pdxproc`'s child hand-off doesn't pass one, so
  with this bootstrap a child process is resumed with nothing loaded. The .NET bootstrap
  falls back to the `PDXPL` environment variable, and that's the one that carries the
  library set into child processes.

And the ordinary limits:

- The APC fires at the end of loader initialisation, so the exe's TLS callbacks and the
  `DllMain` of any DLL it imports statically run before the hooks are in place.
- At most 99 libraries (`MAX_NUM_LIBS`). More than that is treated like any other failure:
  the bootstrap terminates the target and exits -1.
- Paths go through the ANSI code page and are capped at 2047 characters. Paths that don't
  fit in your code page won't work.
- Extra arguments are joined with spaces and not re-quoted, so an argument that contains a
  space reaches the target split in two.
- The stub ignores what `LdrLoadDll` returns. A library that fails to load fails silently
  and the bootstrap still reports success.
- If writing into the target fails partway, the bootstrap terminates the target. That
  includes `inject` mode.
- With `vxls=1`, a failed `CreateRemoteThread` is not reported; the bootstrap still exits 0.
- The memory it writes into the target is allocated read-write-execute and never freed.

## Related

- [vxlauncher](https://github.com/batteryshark/vxlauncher) starts this for the app's main
  process.
- [pdx](https://github.com/batteryshark/pdx) calls it from `pdxproc` for child processes.
- [VXTools.NET](https://github.com/batteryshark/VXTools.NET) has the .NET port, with the
  watchdog callout and the `PDXPL` fallback.

## Credits

Queued user APC injection is an old and well-documented technique; nothing here is new.

## Licence

MIT, see [LICENSE](LICENSE).
