# VSG compile-probe

This is the **validation gate** for the `vsg` skill — the native-C++ substitute for a
web design-system's typecheck. The meta-skill that produced this skill validates web
DS skills by typechecking TypeScript against npm types; there is no TS or npm here, so
the equivalent grounding is a real C++ build: `probe.cpp` exercises the public API
surface documented in `../references/`, and building it against the installed
`vsg::vsg` proves those types, signatures, and symbols are real — stronger than a name
grep, because the compiler checks signatures and the linker checks symbols.

## Run it

```bash
./run-probe.sh            # configure + build; prints PROBE_RESULT=PASS | FAIL
./run-probe.sh --clean    # wipe build/ first
```

`run-probe.sh` uses `find_package(vsg)` with `CMAKE_PREFIX_PATH=/usr/local` (override
with `VSG_PREFIX=/path`). `vsgConfig.cmake` resolves Vulkan via the `VULKAN_SDK`
environment variable.

## What it is NOT

It is not a runnable demo. Every API call sits behind `if (argc < 0)` (never true), so
no window or Vulkan device is ever created — the binary is built purely to make the
compiler and linker resolve every referenced symbol. To build an actual application,
follow `../references/patterns.md` (Minimal application skeleton).

## Version note

Citations in `../references/` point at the repo headers (master, `v1.1.15`). This probe
links the locally installed `vsg::vsg` (currently `1.1.14`) — one patch behind. The
documented surface is the stable core, identical across that delta. If a future
documented symbol exists only on master and fails to compile here, that is the signal
to mark it `[VERIFY]` rather than ship it unproven.
