# Application / Viewer

The `vsg/app` module is the entry point for building a running VSG application: it owns the window(s), the Vulkan device setup, the per-frame event/update/record/submit/present loop, and the camera that controls what is rendered. An app author reaches for this module to create a `Window` from `WindowTraits`, build a `Camera` (pairing a `ProjectionMatrix` with a `ViewMatrix`), wire the scene into a `CommandGraph`/`RenderGraph`, drive everything from a `Viewer`, and attach interaction such as a `Trackball`. `CompileManager` handles compiling subgraphs (uploading GPU resources) before they are rendered.

## Public includes

```cpp
#include <vsg/all.h>            // umbrella header: pulls in everything below
// or include only what you need:
#include <vsg/app/Viewer.h>
#include <vsg/app/Window.h>
#include <vsg/app/WindowTraits.h>
#include <vsg/app/Camera.h>
#include <vsg/app/ProjectionMatrix.h>   // Perspective, Orthographic, EllipsoidPerspective
#include <vsg/app/ViewMatrix.h>         // LookAt, LookDirection
#include <vsg/app/View.h>
#include <vsg/app/RenderGraph.h>        // createRenderGraphForView
#include <vsg/app/CommandGraph.h>       // createCommandGraphForView
#include <vsg/app/Trackball.h>
#include <vsg/app/CompileManager.h>
#include <vsg/app/CloseHandler.h>
```

## Key classes

### vsg::Viewer
High level object that manages windows, handles events, and records/submits command graphs for compute and rendering (app/Viewer.h:30-32). Constructed with `Viewer::create()`.
- `addWindow(ref_ptr<Window>)` attaches a window to the viewer (app/Viewer.h:41).
- `addEventHandler(ref_ptr<Visitor>)` registers an event handler such as a `Trackball` or `CloseHandler` (app/Viewer.h:71).
- `assignRecordAndSubmitTaskAndPresentation(CommandGraphs)` wires the supplied command graphs into the record/submit/present pipeline, replacing any prior setup (app/Viewer.h:122-124).
- `compile(ref_ptr<ResourceHints> = {})` compiles the assigned scene/command graphs (returns a `CompileResult`); call once after the graphs are assigned and before the frame loop (app/Viewer.h:107).
- `advanceToNextFrame(double = UseTimeSinceStartPoint)` returns false once the viewer is closed; otherwise polls events and advances the `FrameStamp` (app/Viewer.h:99-102).
- `handleEvents()` passes polled events to the registered event handlers (app/Viewer.h:104-105).
- `update()` runs queued update operations (app/Viewer.h:135).
- `recordAndSubmit()` records and submits the command graphs for the frame (app/Viewer.h:137).
- `present()` presents the rendered results to the swapchains (app/Viewer.h:139).
- `active()` returns true while the viewer is running; `close()` schedules shutdown (app/Viewer.h:56-59).
- `addUpdateOperation(ref_ptr<Operation>, UpdateOperations::RunBehavior = ONE_TIME)` queues a thread-safe update operation (app/Viewer.h:85-88).
- `compileManager` is the `ref_ptr<CompileManager>` for compiling subgraphs at runtime (app/Viewer.h:94).
- `setupThreading()` / `stopThreading()` enable multi-threaded recording (app/Viewer.h:132-133).

### vsg::Window
Cross-platform window base class; the platform-specific subclass is chosen automatically (app/Window.h:27-30).
- `Window::create(ref_ptr<WindowTraits>)` is the factory that returns a platform-specific window from traits (app/Window.h:36-37).
- `traits()` returns the `WindowTraits` the window was created from (app/Window.h:61).
- `extent2D()` returns the current `VkExtent2D` of the window (app/Window.h:64).
- `clearColor()` accesses the window's clear color (app/Window.h:66).
- `getDevice()` / `getOrCreateDevice()` access the Vulkan `Device` (app/Window.h:88-89).
- `Windows` is `std::vector<ref_ptr<Window>>` (app/Window.h:186).

### vsg::WindowTraits
Specifies the settings used when creating windows, the Vulkan instance, and the device (app/WindowTraits.h:26-27).
- Create via `WindowTraits::create()`; convenience constructors take a title, or width/height/title, or x/y/width/height/title (app/WindowTraits.h:33-35).
- `windowTitle` / `width` / `height` / `x` / `y` configure window geometry and title (app/WindowTraits.h:47-58).
- `fullscreen` toggles fullscreen mode (app/WindowTraits.h:52).
- `debugLayer` enables Vulkan validation; `synchronizationLayer`, `apiDumpLayer`, `debugUtils` enable further diagnostics (app/WindowTraits.h:77-80).
- `samples` requests MSAA sample count (app/WindowTraits.h:96).
- `swapchainPreferences`, `depthFormat`, `vulkanVersion` and `deviceFeatures` tune the Vulkan setup (app/WindowTraits.h:66-90).
- `WindowTraits(CommandLine&)` builds traits from command-line arguments (app/WindowTraits.h:31).

### vsg::Camera
Holds the projection matrix, view matrix, and viewport that control what a `View` looks at (app/Camera.h:23-26). It is a `Node`, so it can also be placed in the scene graph.
- `Camera::create(projectionMatrix, viewMatrix, viewportState = {})` is the typical factory (app/Camera.h:31).
- `projectionMatrix` is a `ref_ptr<ProjectionMatrix>` (app/Camera.h:34).
- `viewMatrix` is a `ref_ptr<ViewMatrix>` (app/Camera.h:35).
- `viewportState` is a `ref_ptr<ViewportState>` (app/Camera.h:36).
- `getViewport()` / `getRenderArea()` read the viewport and scissor from the viewport state (app/Camera.h:38-39).

### vsg::ProjectionMatrix
Base class for a camera projection matrix and its inverse; subclasses implement `transform()` (app/ProjectionMatrix.h:22-35).
- `vsg::Perspective` implements a gluPerspective-style projection; `Perspective::create(fov, aspectRatio, nearDistance, farDistance)` (app/ProjectionMatrix.h:48-75). Fields: `fieldOfViewY`, `aspectRatio`, `nearDistance`, `farDistance` (app/ProjectionMatrix.h:89-92).
- `vsg::Orthographic` implements a glOrtho-style projection; `Orthographic::create(left, right, bottom, top, near, far)` (app/ProjectionMatrix.h:100-133).
- `vsg::EllipsoidPerspective` is a perspective projection that auto-clamps near/far to an ellipsoid model, for whole-earth rendering (app/ProjectionMatrix.h:187-200).
- `changeExtent(prevExtent, newExtent)` updates aspect ratio on window resize (app/ProjectionMatrix.h:42-43).

### vsg::ViewMatrix
Base class for a camera view matrix and its inverse; subclasses implement `transform(offset)` (app/ViewMatrix.h:21-40).
- `vsg::LookAt` implements gluLookAt; `LookAt::create(eye, center, up)` with `dvec3` arguments (app/ViewMatrix.h:52-79). Fields `eye`, `center`, `up` (app/ViewMatrix.h:100-102).
- `LookDirection` uses a `position` and a `dquat rotation` (app/ViewMatrix.h:106-126).
- `RelativeViewMatrix` and `TrackingViewMatrix` decorate/track another matrix or a scene path (app/ViewMatrix.h:134-172).

### vsg::View
A `Group` that pairs a `Camera` with the scene subgraph being rendered (app/View.h:34-35).
- `View::create(camera, scenegraph = {}, features = RECORD_ALL)` is the common factory (app/View.h:43).
- `camera` is the `ref_ptr<Camera>` (app/View.h:67).
- `features` (a `ViewFeatures` mask such as `RECORD_LIGHTS`, `RECORD_SHADOW_MAPS`, `RECORD_ALL`) controls view-dependent state (app/View.h:26-31, app/View.h:73).
- `mask` controls traversal of the view's subgraph (app/View.h:78).

### vsg::RenderGraph
Encapsulates the `vkCmdBeginRenderPass`/`vkCmdEndRenderPass` pair; its children are recorded within that pair (app/RenderGraph.h:25-28).
- `RenderGraph::create(window, view = {})` sets up clear values from the window's attachments and color (app/RenderGraph.h:33-34).
- `window` / `framebuffer`: assign one as the render target (framebuffer takes precedence) (app/RenderGraph.h:41-43).
- `clearValues` plus `setClearValues(clearColor, clearDepthStencil)` configure attachment clears (app/RenderGraph.h:60-66).
- `windowResizeHandler` auto-updates viewports/scissors/renderArea on resize (default on) (app/RenderGraph.h:73-75).
- `createRenderGraphForView(window, camera, scenegraph, contents, assignHeadlight)` is a convenience factory that builds the RenderGraph and its `View` (app/RenderGraph.h:86-88).

### vsg::CommandGraph
A group node at the top of the scene graph that manages recording its subgraph into Vulkan command buffers (app/CommandGraph.h:26-27).
- `CommandGraph::create(window, child = {})` constructs a command graph bound to a window (app/CommandGraph.h:32).
- `window` / `framebuffer` / `device` configure the recording target at construction time (app/CommandGraph.h:35-38).
- `createCommandGraphForView(window, camera, scenegraph, contents, assignHeadlight)` is the one-call convenience factory that builds a CommandGraph containing a RenderGraph+View for the scene (app/CommandGraph.h:65-66).
- `CommandGraphs` is `std::vector<ref_ptr<CommandGraph>>` (app/CommandGraph.h:63).

### vsg::Trackball
An event-handler `Visitor` providing mouse/touch 3D trackball camera manipulation (app/Trackball.h:26-27).
- `Trackball::create(camera, ellipsoidModel = {})` constructs it bound to a camera (app/Trackball.h:30).
- `addWindow(window, offset = {})` registers a window whose events it should respond to (app/Trackball.h:65).
- `setViewpoint(lookAt, duration = 1.0)` animates the camera to a new viewpoint (app/Trackball.h:75).
- `addKeyViewpoint(...)` binds a key to a viewpoint (by `LookAt` or lat/long/altitude) (app/Trackball.h:68-71).
- Button-mask fields `rotateButtonMask`, `panButtonMask`, `zoomButtonMask` map mouse buttons to actions (app/Trackball.h:123-129).

### vsg::CompileManager
Helper that compiles subgraphs (uploads GPU resources) for the windows/framebuffers it knows about (app/CompileManager.h:55-56). Usually accessed via `Viewer::compileManager`.
- `compile(ref_ptr<Object>, ContextSelectionFunction = {})` compiles an object and returns a `CompileResult` (app/CompileManager.h:82).
- `compileTask(ref_ptr<RecordAndSubmitTask>, ...)` compiles all command graphs in a task (app/CompileManager.h:85).
- `CompileResult` is convertible to bool (true on `VK_SUCCESS`) and reports whether the viewer needs updating via `requiresViewerUpdate()` (app/CompileManager.h:36-40).
- `updateViewer(Viewer&, const CompileResult&)` updates viewer data structures to match newly compiled subgraphs (app/Viewer.h:172).

## Idioms

Minimal full viewer setup and frame loop:

```cpp
auto traits = vsg::WindowTraits::create("my vsg app");
auto window = vsg::Window::create(traits);

auto viewer = vsg::Viewer::create();
viewer->addWindow(window);

// camera = projection + view + viewport
auto perspective = vsg::Perspective::create(
    60.0, static_cast<double>(window->extent2D().width) / window->extent2D().height,
    0.1, 1000.0);
auto lookAt = vsg::LookAt::create(vsg::dvec3(0.0, -5.0, 2.0),  // eye
                                  vsg::dvec3(0.0, 0.0, 0.0),    // center
                                  vsg::dvec3(0.0, 0.0, 1.0));   // up
auto viewportState = vsg::ViewportState::create(window->extent2D());
auto camera = vsg::Camera::create(perspective, lookAt, viewportState);

// wire scene -> CommandGraph(RenderGraph(View))
auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

viewer->compile();

while (viewer->advanceToNextFrame())
{
    viewer->handleEvents();
    viewer->update();
    viewer->recordAndSubmit();
    viewer->present();
}
```

Adding interaction handlers (close on Escape, trackball manipulation):

```cpp
viewer->addEventHandler(vsg::CloseHandler::create(viewer));
viewer->addEventHandler(vsg::Trackball::create(camera));
```

Compiling a subgraph added after the loop has started:

```cpp
auto result = viewer->compileManager->compile(newSubgraph);
if (result) vsg::updateViewer(*viewer, result);
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `new vsg::Viewer()` / `new vsg::Window(traits)` | `vsg::Viewer::create()` / `vsg::Window::create(traits)` | All VSG types are reference-counted and constructed via the static `create()` factory returning a `ref_ptr` (app/Window.h:36-37). |
| Calling `recordAndSubmit()` without ever calling `viewer->compile()` | Call `viewer->compile()` once after `assignRecordAndSubmitTaskAndPresentation(...)` and before the loop | GPU resources must be compiled/uploaded before recording (app/Viewer.h:107). |
| `while (true)` { ... } as the frame loop | `while (viewer->advanceToNextFrame())` | `advanceToNextFrame()` returns false when the viewer is closed and advances the `FrameStamp` (app/Viewer.h:99-102). |
| Forgetting to assign command graphs to the viewer | `viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph})` | The record/submit/present pipeline is empty until command graphs are wired in (app/Viewer.h:122-124). |
| Compiling a runtime subgraph but not refreshing the viewer | `if (result) vsg::updateViewer(*viewer, result);` | The viewer's data structures must be updated to match newly compiled subgraphs (app/Viewer.h:172). |

## Things to never invent

- `Viewer::run()` does not exist as a one-call loop; drive the loop yourself with `advanceToNextFrame()`/`handleEvents()`/`update()`/`recordAndSubmit()`/`present()` (app/Viewer.h:99-139).
- `WindowTraits` has no `setTitle(...)` method; set the `windowTitle` field directly (app/WindowTraits.h:58).
- `Camera` does not expose a `setProjectionMatrix(...)`/`setViewMatrix(...)` setter pair; assign the public `projectionMatrix`/`viewMatrix` members (app/Camera.h:34-35).
- `Perspective` takes `fieldOfViewY, aspectRatio, nearDistance, farDistance` (in that order) — not separate near/far setter calls (app/ProjectionMatrix.h:69).
- `LookAt` constructor order is `eye, center, up`; there is no `setEye()`-style API, the fields are public (app/ViewMatrix.h:71, app/ViewMatrix.h:100-102).

## Source references

- include/vsg/app/Viewer.h
- include/vsg/app/Window.h
- include/vsg/app/WindowTraits.h
- include/vsg/app/Camera.h
- include/vsg/app/ProjectionMatrix.h
- include/vsg/app/ViewMatrix.h
- include/vsg/app/View.h
- include/vsg/app/RenderGraph.h
- include/vsg/app/CommandGraph.h
- include/vsg/app/Trackball.h
- include/vsg/app/CompileManager.h
- include/vsg/app/CloseHandler.h
