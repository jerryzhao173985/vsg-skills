---
title: view_model
description: Load a 3D model from the command line and view it, framing the camera from the scene bounds.
---

# Load & view a model — `view_model`

Loads a model named on the command line with `vsg::read_cast<vsg::Node>`, frames the
camera from the scene's bounding box via `vsg::ComputeBounds`, and views it with a
trackball. Native `.vsgt`/`.vsgb` load out of the box; other formats need `vsgXchange`
installed and linked (a separate library, out of scope here). Compile-verified twin:
`examples/view_model.cpp`.

```cpp
#include <vsg/all.h>

int main(int argc, char** argv)
{
    vsg::CommandLine arguments(&argc, argv);
    auto options = vsg::Options::create();

    if (argc < 2) { vsg::warn("usage: view_model <model.vsgt>"); return 1; }

    auto scene = vsg::read_cast<vsg::Node>(argv[1], options);
    if (!scene) { vsg::fatal("Could not load model: ", argv[1]); return 1; }

    // frame the camera from the loaded scene's bounds
    auto computeBounds = vsg::ComputeBounds::create();
    scene->accept(*computeBounds);
    vsg::dbox bounds = computeBounds->bounds;
    vsg::dvec3 centre = (bounds.min + bounds.max) * 0.5;
    double radius = vsg::length(bounds.max - bounds.min) * 0.5;
    if (radius <= 0.0) radius = 1.0;

    auto traits = vsg::WindowTraits::create();
    traits->windowTitle = "view_model";
    auto window = vsg::Window::create(traits);
    if (!window) { vsg::fatal("Could not create window."); return 1; }

    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    double aspect = static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height);
    auto perspective = vsg::Perspective::create(30.0, aspect, radius * 0.001, radius * 10.0);
    auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -radius * 3.5, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
    auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});
    viewer->addEventHandler(vsg::Trackball::create(camera));
    viewer->compile();

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

- **Loading**: `vsg::read_cast<vsg::Node>(path, options)` returns a typed `ref_ptr` and is null on failure — always check it. Pass a `vsg::Options` so search paths / readerwriters are respected.
- **Framing from bounds**: run a `ComputeBounds` visitor over the loaded scene, then derive `centre` and `radius` to place the camera and set sensible near/far planes (near/far scaled by `radius` avoids z-fighting and clipping on models of any size).
- **`CommandLine`**: construct `vsg::CommandLine arguments(&argc, argv)` to consume option flags before reading positional arguments (here the model path is `argv[1]`).

## Key APIs

- `read_cast` / `Options` — see `references/components/io.md`.
- `ComputeBounds`, `CommandLine` — see `references/components/utils.md`.
- Camera, window, viewer, the loop — see `references/components/app.md` and `references/foundations/rendering-model.md`.
- `accept` visitor dispatch (used by `ComputeBounds`) — see `references/foundations/object-model.md`.
