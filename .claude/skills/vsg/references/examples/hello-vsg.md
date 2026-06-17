---
title: hello_vsg
description: The smallest complete VulkanSceneGraph application — window, one-box scene, trackball, render loop.
---

# Minimal application — `hello_vsg`

The smallest complete VSG program. It exercises the three core idioms end to end:
`create()`/`ref_ptr` ownership, `Group`/`addChild` composition, and the compile-then-loop
rendering model. Compile-verified twin: `examples/hello_vsg.cpp`.

```cpp
#include <vsg/all.h>

int main(int argc, char** argv)
{
    // 1. scene graph: a single box created by the geometry Builder
    auto scene = vsg::Group::create();

    auto builder = vsg::Builder::create();
    vsg::GeometryInfo geomInfo;
    vsg::StateInfo stateInfo;
    scene->addChild(builder->createBox(geomInfo, stateInfo));

    // 2. window + viewer
    auto traits = vsg::WindowTraits::create();
    traits->windowTitle = "hello_vsg";
    auto window = vsg::Window::create(traits);
    if (!window) { vsg::fatal("Could not create window."); return 1; }

    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    // 3. camera framing the scene
    double aspect = static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height);
    auto perspective = vsg::Perspective::create(60.0, aspect, 0.1, 100.0);
    auto lookAt = vsg::LookAt::create(vsg::dvec3(0.0, -4.0, 2.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
    auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    // 4. wire the scene into a command graph, assign to the viewer
    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    // 5. interaction + compile (allocate GPU resources) before the loop
    viewer->addEventHandler(vsg::Trackball::create(camera));
    viewer->compile();

    // 6. the canonical render loop
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

## What to copy

- The **six-step skeleton** — scene → window/viewer → camera → command graph → compile → loop — is the backbone of every VSG app. Keep the order: `compile()` comes after `assignRecordAndSubmitTaskAndPresentation` and before the loop.
- The **camera triple**: a `ProjectionMatrix` (`Perspective`), a `ViewMatrix` (`LookAt`), and a `ViewportState` built from `window->extent2D()` (nodes/Group.h:39, app/Window.h:64).
- `vsg::createCommandGraphForView(window, camera, scene)` builds the `CommandGraph → RenderGraph → View` nesting for you (app/CommandGraph.h:65-66).
- Always guard `Window::create` returning null and bail with `vsg::fatal`.

## Key APIs

- Construction + composition — see `references/components/core.md`, `references/components/scene-graph.md` (`Group::addChild`, nodes/Group.h:39).
- Viewer + window + camera — see `references/components/app.md`; `viewer->compile()` once before the loop (app/Viewer.h:107).
- `Builder::createBox` — see `references/components/utils.md` (utils/Builder.h:123-130).
- The render loop and its ordering — see `references/foundations/rendering-model.md`.
