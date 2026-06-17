# VSG examples

Complete, runnable VulkanSceneGraph programs that demonstrate the skill end to end.
Unlike the snippets in `../references/`, these are whole `main()` programs — and unlike
the compile-probe, they are real apps you can run (they create a window and render).

They are the **compile/link gate** for `../references/examples/*.md`: building them proves
the documented examples ground in the real API.

| Source | What it does |
|---|---|
| `hello_vsg.cpp` | minimal app: one box, trackball, render loop |
| `view_model.cpp` | load a model from argv, frame from bounds, view it |
| `builder_primitives.cpp` | Builder box/sphere/capsule placed with transforms, lit |

## Build

```bash
./build-examples.sh           # configure + build all; prints EXAMPLES_RESULT=PASS|FAIL
./build-examples.sh --clean   # wipe build/ first
```

Uses `find_package(vsg)` with `CMAKE_PREFIX_PATH=/usr/local` (override with `VSG_PREFIX`).
Running an example needs a display + Vulkan driver; building it does not.
