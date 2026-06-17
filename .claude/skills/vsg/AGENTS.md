# AGENTS.md — VSG quick reference

Agent-facing cheat sheet for writing VulkanSceneGraph (VSG) C++. Load the routed `references/` files in `SKILL.md` for depth; this page is the spine you keep in working memory.

## The three idioms that govern everything

1. **Create + own.** `auto x = vsg::Thing::create(args...);` returns a `vsg::ref_ptr<vsg::Thing>`. Never `new`, never `delete`, never `std::shared_ptr`. Non-owning reference → `vsg::observer_ptr<vsg::Thing>`. (`core/Inherit.h:35`, `core/ref_ptr.h:18`)
2. **Compose + traverse.** A scene is a graph of `vsg::Node`s. `vsg::Group` holds `children`; add with `group->addChild(child)`. Processing is done by **Visitors** (`accept` → `apply` → `traverse`). Transforms (`MatrixTransform`) and state (`StateGroup`) decorate subtrees. (`nodes/Group.h:39`, `core/Inherit.h:70`)
3. **Compile then loop.** Wrap the scene in `View → RenderGraph → CommandGraph`, assign it to the `Viewer`, call `viewer->compile()` once, then run the frame loop. (`app/Viewer.h`)

## Canonical render loop

```cpp
viewer->compile();                       // once, after command graphs are assigned
while (viewer->advanceToNextFrame())     // returns false when the viewer closes
{
    viewer->handleEvents();              // dispatch events to handlers (e.g. Trackball)
    viewer->update();                    // run update operations + animations
    viewer->recordAndSubmit();           // record command buffers and submit to the GPU
    viewer->present();                   // present the swapchain image
}
```

Order is load-bearing. See `references/foundations/rendering-model.md`.

## Where things live

| You want to… | Module / file |
|---|---|
| objects, smart pointers, data arrays, visitors | `core` · foundations/object-model |
| nodes, transforms, geometry, binding state to a subtree | `scene-graph` · foundations/scene-and-state |
| window, viewer, camera, render loop | `app` · foundations/rendering-model |
| pipelines, shaders, descriptors, textures | `state` |
| vectors, matrices, quaternions, transforms | `maths` |
| load/save models, options, logging | `io` |
| draw/dispatch/copy commands | `commands` |
| primitive builder, shader sets, picking, CLI | `utils` |
| text, lights/shadows, animation, threading | `text` · `lighting` · `animation` · `threading` |

## Hard rules (do not violate)

- `T::create(...)` only — no `new`/`delete` on VSG objects (the destructor is `protected`).
- Own with `ref_ptr`; non-owning/back-ref with `observer_ptr`; never raw owning pointers.
- `viewer->compile()` once, before the loop; loop calls in the exact 5-step order above.
- Maths types and double precision: positions accumulate through `MatrixTransform` (a `dmat4`); use `dvec3`/`dmat4` for large/whole-earth coordinates.
- Set `DataVariance` correctly and call `dirty()` after editing dynamic GPU data.
- Anything you cannot ground in `include/vsg/*.h` → mark `[VERIFY]`; never invent a symbol.

## Verifying generated code

The documented API surface is checked by a real compiler in `probe/` (`run-probe.sh` builds `probe.cpp` against the installed `vsg::vsg`). Every rule in `references/` cites `include/vsg/<module>/<Header>.h:line` (authoritative API) or `src/vsg/<module>/<File>.cpp:line` (idiomatic wiring). `scripts/check-skill-docs.sh` re-resolves every citation.
