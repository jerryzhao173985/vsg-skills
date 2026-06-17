---
name: vsg
description: Build native C++ applications with VulkanSceneGraph (VSG) — scene-graph nodes, the viewer/render loop, Vulkan state & pipelines, maths, IO, text, lighting, animation and threading. Use when the user writes or edits C++ that includes <vsg/...>, builds a Vulkan scene graph, sets up a vsg::Viewer, or asks for VulkanSceneGraph / VSG / vsg:: code. Scope: the public C++ API and idiomatic wiring. IMPORTANT: this file is an orchestrator. Load the references/ files named in the routing table; SKILL.md alone is insufficient.
---

# VulkanSceneGraph (VSG) — native C++ skill

## Mission

A `vsg` skill is an adapter that teaches an agent how to build high-fidelity **native C++ applications** with VulkanSceneGraph. It is not a copy of the doxygen reference. It tells the agent what to read, which APIs are public, which sources are authoritative, and how to verify that generated C++ actually compiles against the real library. The authoritative source is the VSG headers in this repository; idiomatic wiring is grounded in the `src/` implementations; and every rule in the `references/` files cites a real `file:line`.

## Setup (CMake + minimal app)

VSG is a C++17 library consumed through CMake. A consuming project links the imported target `vsg::vsg`:

```cmake
cmake_minimum_required(VERSION 3.14)
project(my_vsg_app CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(vsg REQUIRED)          # find_dependency(Vulkan) is handled by vsgConfig

add_executable(my_vsg_app main.cpp)
target_link_libraries(my_vsg_app vsg::vsg)
```

The smallest correct application — create a window, wrap the scene in a `View`/`RenderGraph`/`CommandGraph`, **compile once**, then run the frame loop. Full grounded recipe in `references/patterns.md` (Minimal application skeleton) and `references/foundations/rendering-model.md`:

```cpp
#include <vsg/all.h>

int main(int argc, char** argv)
{
    auto scene = vsg::Group::create();                         // build your scene graph here

    auto traits = vsg::WindowTraits::create();
    traits->windowTitle = "vsg app";
    auto window = vsg::Window::create(traits);

    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    // camera framing the scene
    auto perspective = vsg::Perspective::create(60.0, double(window->extent2D().width) / double(window->extent2D().height), 0.1, 100.0);
    auto lookAt = vsg::LookAt::create(vsg::dvec3(0.0, -4.0, 2.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
    auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    viewer->addEventHandler(vsg::Trackball::create(camera));    // mouse/keyboard camera control
    viewer->compile();                                          // allocate GPU resources — once, before the loop

    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }
    return 0;
}
```

## Import rules

- Canonical include is the umbrella header `#include <vsg/all.h>`, or the specific public header `#include <vsg/<module>/<Header>.h>` (e.g. `<vsg/nodes/Group.h>`, `<vsg/app/Viewer.h>`).
- All public symbols live in namespace `vsg::`. There is no other public namespace an app needs.
- Do **not** include or rely on internal/private headers outside `include/vsg/` (e.g. third-party `vk/vulkan.h` shim internals). Companion features (image/model loaders, HTTP) live in **separate** libraries (`vsgXchange`, `vsgImGui`, `vsgQt`, …) with their own `find_package`/headers — out of scope for this skill.

## Source-of-truth rules

Code wins over prose on every conflict. In precedence order:

1. **Authoritative API** — the headers under `include/vsg/<module>/*.h`, pinned to VSG commit **`3b986a00`** (`v1.1.15-10-g3b986a00`). Every `references/` rule cites these by `file:line` **against that commit**; line numbers drift if you sync to a newer `master`, so re-run `scripts/check-skill-docs.sh` after updating VSG. The audit *line-resolves* every citation; the compile-probe is the *semantic* gate (it proves signatures link, not merely that a line exists).
2. **Idiomatic wiring** — the implementations under `src/vsg/<module>/*.cpp`. Cited when a rule depends on runtime behavior (e.g. what `compile()` traverses, the exact frame-loop order).
3. **Concepts & tutorials** — https://vsg-dev.github.io/vsg-dev.io and https://vsg-dev.github.io/vsgTutorial (Foundations / Scene Graph / Application). Cited for framing; never authoritative over the headers on a signature.
4. **Build/link reality** — the locally installed `vsg::vsg` (`1.1.14`, one patch behind the pinned commit; stable core API is identical across that delta) used by the compile-probe in `probe/`. A documented symbol that fails to compile against it is flagged, never shipped silently.

## Out of scope

This skill covers the **core public C++ API of the VulkanSceneGraph library itself**. It deliberately does NOT cover — and an agent should say so and point at the right home rather than improvise an API:

- **Companion libraries** — `vsgXchange` (glTF/OBJ/image loaders, HTTP), `vsgImGui`, `vsgQt`, `vsgPoints`, `osg2vsg`. Non-native model formats (anything but `.vsgt`/`.vsgb`) need `vsgXchange`; `read_cast` returns null without it. Each is a separate `find_package` dependency.
- **Niche VSG modules** — ray tracing (`include/vsg/raytracing/`), mesh shaders (`meshshaders/`), and the low-level Vulkan wrappers in `include/vsg/vk/` (use the higher-level `state`/`app` objects instead).
- **Custom `ReaderWriter` authoring**, GPU compute beyond a passing mention, and platform windowing internals (`platform/`).
- **Shader authoring** (GLSL/SPIR-V *content*). The skill shows how to *wire* a `ShaderSet`/pipeline (see `references/components/state.md` + `utils.md`), not how to write shader code.

## When to Load References

| Trigger | Files to load | Notes |
|---|---|---|
| user constructs/owns VSG objects, uses `ref_ptr`/`observer_ptr`, writes a Visitor, or builds `Data`/`Array`/`Value` | `references/foundations/object-model.md`, `references/components/core.md` | the memory & RTTI contract every other file assumes |
| user sets up a window, viewer, camera, or the render loop | `references/foundations/rendering-model.md`, `references/components/app.md` | compile → record → submit → present pipeline |
| user composes a scene graph, transforms, or binds state to a subtree | `references/foundations/scene-and-state.md`, `references/components/scene-graph.md` | Group/Transform/StateGroup/geometry leaves |
| user configures pipelines, shaders, descriptors, or textures, or builds custom geometry | `references/components/state.md`, `references/examples/custom-geometry.md` | Vulkan state as VSG objects; the hand-built `ShaderSet`/`GraphicsPipelineConfigurator` path |
| user does vector/matrix/quaternion maths or transforms | `references/components/maths.md` | header-only `vec*`/`mat*`/`transform` |
| user loads or saves models/data, or uses `Options`/`Logger` | `references/components/io.md` | `read`/`read_cast`/`write`, native `.vsgt`/`.vsgb` |
| user records draw/dispatch/copy commands | `references/components/commands.md` | `Command`/`Commands`/`Draw`/`Bind*` |
| user needs `Builder` primitives, shader sets, picking, or command-line parsing | `references/components/utils.md` | high-level helpers |
| user renders text | `references/components/text.md` | `Text`/`Font`/`TextLayout` |
| user adds lights or shadows | `references/components/lighting.md` | `Light` subclasses + `ShadowSettings` |
| user animates the scene or camera | `references/components/animation.md` | `Animation`/`AnimationManager`/samplers |
| user enables multi-threaded recording | `references/components/threading.md` | `setupThreading()` + primitives |
| user asks for a runnable recipe or a minimal/whole app | `references/patterns.md` | end-to-end idioms (app skeleton, scene build, events, loop) |
| user wants a complete, compile-verified example program | `references/examples/index.md` | index of runnable examples (built by `examples/build-examples.sh`) |
| hello_vsg: the minimal app | `references/examples/hello-vsg.md` | window + scene + trackball + loop |
| view_model: load & view a model | `references/examples/view-model.md` | `read_cast`, `CommandLine`, framing from `ComputeBounds` |
| builder_primitives: geometry + lights | `references/examples/builder-primitives.md` | `Builder` primitives, `MatrixTransform`, light nodes |
| custom_geometry: own mesh + pipeline (no Builder) | `references/examples/custom-geometry.md` | `ShaderSet` + `GraphicsPipelineConfigurator` + `VertexIndexDraw` + material descriptor |
| (audit/registry) recurring API mistakes | `references/anti-patterns.md` | `Bad \| Good \| Why`; rules also mirrored in Hard rules + component files |
| (maintenance) prove the skill grounds in real source | `probe/`, `scripts/check-skill-docs.sh` | compile-probe + citation resolver |

## Component slate

Each entry resolves to its own `references/components/<name>.md`. **CORE** is load-bearing for essentially every app; **EXTENDED** is reached for on demand. Steer attention to CORE first.

**CORE**
- `core` — object model: `Object`, `Inherit`/`create`, `ref_ptr`/`observer_ptr`, `Data`/`Value`/`Array`, `Visitor`.
- `scene-graph` — nodes: `Group`, `StateGroup`, `MatrixTransform`, `Switch`, `LOD`/`PagedLOD`, `CullGroup`, `Geometry`, `VertexIndexDraw`.
- `app` — `Viewer`, `Window`/`WindowTraits`, `Camera`, `View`, `RenderGraph`/`CommandGraph`, `Trackball`.
- `state` — `GraphicsPipeline`, `ShaderStage`, `DescriptorSet(Layout)`, `PipelineLayout`, `Sampler`, `Image`/`ImageView`, the `*State` structs.
- `utils` — `Builder`, `GraphicsPipelineConfigurator`, `ShaderSet`, intersectors, `CommandLine`.
- `maths` — `vec*`/`mat*`/`quat`, `transform`, `box`/`sphere`.
- `io` — `read`/`read_cast`/`write`, `Options`, `ReaderWriter`, `Logger`.

**EXTENDED** (reach for on demand)
- `commands` — `Command`/`Commands`, `Draw`/`DrawIndexed`, `BindVertexBuffers`/`BindIndexBuffer` (usually carried for you by `Geometry`/`VertexIndexDraw`).
- `text` — `Text`, `TextGroup`, `Font`, `TextLayout`.
- `lighting` — `Light`, `AmbientLight`/`DirectionalLight`/`PointLight`/`SpotLight`, `ShadowSettings`.
- `animation` — `Animation`, `AnimationManager`, `TransformSampler`/`CameraSampler`.
- `threading` — `setupThreading`, `OperationThreads`, `OperationQueue`, `Barrier`/`Latch`.

## Hard rules

- **Construct with `T::create(...)`, never `new`/`delete`.** Every VSG class derives from `vsg::Inherit<Parent,Self>`, whose static `create(...)` returns an owning `vsg::ref_ptr<T>` (`core/Inherit.h:35`). `Object`'s destructor is `protected` (`core/Object.h`), so stack-allocating or `delete`-ing a VSG object is a deliberate compile error.
- **Own objects with `vsg::ref_ptr<T>`; use `vsg::observer_ptr<T>` for non-owning/back-references.** `ref_ptr` is intrusive (half the size of `std::shared_ptr`) (`core/ref_ptr.h:18-19`). Never hold a VSG object by a raw owning pointer or wrap it in `std::shared_ptr`. Break reference cycles with `observer_ptr` (`core/observer_ptr.h`).
- **Call `viewer->compile()` exactly once, after command graphs are assigned and before the frame loop.** It allocates GPU resources by traversing the graph. Editing the scene after compile may require recompiling new subgraphs.
- **The frame loop is `advanceToNextFrame()` → `handleEvents()` → `update()` → `recordAndSubmit()` → `present()`**, in that order, inside `while (viewer->advanceToNextFrame())`. Order is load-bearing (see `references/foundations/rendering-model.md`).
- **GPU data: set the right `DataVariance` and call `dirty()` after editing dynamic data**, or the change is not re-uploaded (`core/Data.h:206`).
- **Include via `<vsg/all.h>` or `<vsg/<module>/<Header>.h>`; everything is in `vsg::`.** Companion features (loaders, ImGui, Qt) are separate libraries, not part of this skill.
- **Anything not grounded in a header gets a literal `[VERIFY]` marker inline.** The authoritative source is `include/vsg/*.h`; report blockers instead of guessing. Do not invent a class, method, field, enum, or free function — if the header does not declare it, it does not exist.

## Final checks

After generating VSG C++, the agent confirms, in a short closing note:

1. Every type/function used is cited to a real `include/vsg/<module>/<Header>.h` (or named `[VERIFY]` if it could not be grounded).
2. No raw `new`/`delete` on VSG objects; ownership is `ref_ptr`; cycles use `observer_ptr`.
3. `viewer->compile()` is called once before the loop, and the loop uses the five calls in order.
4. The code compiles against `vsg::vsg` — run `probe/run-probe.sh` (documented API surface) and `examples/build-examples.sh` (the complete example programs), or build the consuming project.
5. Any `[VERIFY]` markers left in the generated code are listed for the user to resolve.
