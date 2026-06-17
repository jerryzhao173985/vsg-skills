# Rendering model: compile, record, submit, present

A VSG application does not draw to the screen by calling Vulkan directly. Instead it builds a tree of nodes that *describe* the work, compiles that description into GPU objects once, and then drives a fixed per-frame loop that records command buffers and submits them to queues. To compose correct VSG rendering code you must understand the layered ownership — a `CommandGraph` wraps a `RenderGraph` wraps a `View` (a `Camera` plus a scene subgraph) — and the strict order of the four phases each frame: compile (once, up front), record, submit, present. Getting this contract wrong (e.g. compiling after the loop starts, or adding scene nodes without re-compiling) is the most common source of blank windows and validation errors.

## The model

A built scene graph is inert until it is wrapped in the recording hierarchy and compiled. The hierarchy has three layers, each a `Group` subclass:

- A `View` pairs a `Camera` with the scene subgraph it renders; it is "a Group class that pairs a Camera that defines the view with a subgraph that defines the scene that is being viewed/rendered" (app/View.h:34-43). The `Camera` supplies the projection matrix, view matrix and viewport (app/Camera.h:23-36).
- A `RenderGraph` wraps a `View` and "encapsulates the vkCmdBeginRenderPass/vkCmdEndRenderPass functionality" — during record traversal its children are visited inside that begin/end pair (app/RenderGraph.h:25-39). Its member variables map onto `VkRenderPassBeginInfo`: `renderArea`, `clearValues`, `contents`, plus the `framebuffer`/`window` it renders into (app/RenderGraph.h:42-69).
- A `CommandGraph` "sits at the top of the scene graph and manages the recording of its subgraph to Vulkan command buffers" (app/CommandGraph.h:26-27). It is bound to a `Window` (and thus a `Device` and queue family) and owns the `RecordTraversal` that walks the subgraph (app/CommandGraph.h:32-51).

The convenience builders assemble these layers for you: `createCommandGraphForView(window, camera, scenegraph, ...)` creates a `CommandGraph` for the window and adds a `RenderGraph` built by `createRenderGraphForView(...)` as its child (app/CommandGraph.h:65-66, src/vsg/app/CommandGraph.cpp:141-148; app/RenderGraph.h:86-88).

Once the `CommandGraph`(s) exist, they are wired into the `Viewer` via `assignRecordAndSubmitTaskAndPresentation(commandGraphs)`, which creates the `RecordAndSubmitTask` and `Presentation` objects that "manage specified commandGraphs" and replaces any prior setup (app/Viewer.h:122-124). A `RecordAndSubmitTask` "manages the recording of its list of CommandGraph to CommandBuffer which are then submitted to the associated vulkan Queue" (app/RecordAndSubmitTask.h:26-30).

**Compile is a separate, explicit phase.** Before the loop runs, `Viewer::compile()` traverses the command graphs and allocates all GPU resources. `CompileTraversal` "traverses a scene graph and invokes all the StateCommand/Command::compile(..) methods to create all Vulkan objects, allocate GPU memory and transfer data to GPU" (app/CompileTraversal.h:32). `Viewer::compile()` collects resource requirements from each task's command graphs, allocates descriptor pools, assigns viewIDs/bins, creates the `CompileManager` if absent, then calls `compileManager->compileTask(task, ...)` per task (src/vsg/app/Viewer.cpp:299-403). If no tasks are assigned it returns an empty result immediately (src/vsg/app/Viewer.cpp:282-285), so assignment must precede compile.

**The per-frame loop** (read directly from `Viewer::advanceToNextFrame`) is: `advanceToNextFrame()` -> `handleEvents()` -> `update()` -> `recordAndSubmit()` -> `present()`. Inside `advanceToNextFrame()` the viewer checks `active()`, calls `pollEvents(true)`, then `acquireNextFrame()` (which calls `Window::acquireNextImage()` per visible window), then builds a fresh `FrameStamp`, advances every task, and queues a `FrameEvent` (src/vsg/app/Viewer.cpp:155-206). `handleEvents()` dispatches buffered events to the registered handlers (src/vsg/app/Viewer.cpp:265-276). `update()` merges DatabasePager results, runs `updateViewer` when a compile result requires it, then runs update operations and animations (src/vsg/app/Viewer.cpp:793-818). `recordAndSubmit()` resets each command graph then submits each task (src/vsg/app/Viewer.cpp:820-852). `present()` calls `present()` on each `Presentation` (src/vsg/app/Viewer.cpp:854-862).

**Synchronization is fence- and semaphore-based.** Each task holds a ring of fences; `RecordAndSubmitTask::start()` waits on the current frame's fence if it has dependencies before recording the new frame, providing CPU/GPU throttling (src/vsg/app/RecordAndSubmitTask.cpp:116-141). `submit()` runs the pipeline start -> transfer -> record -> finish, where `record()` calls `commandGraph->record(...)` for each command graph (src/vsg/app/RecordAndSubmitTask.cpp:85-153). Window frames carry `imageAvailableSemaphore`/`renderFinishedSemaphore` (app/Window.h:120-126), and task `signalSemaphores` "connect to Presentation.waitSemaphores" (app/RecordAndSubmitTask.h:42) — this is what `assignRecordAndSubmitTaskAndPresentation` sets up automatically.

## Key types & calls

- **`Viewer`** — high level object "for managing windows, handling events and recording and submitting command graphs" (app/Viewer.h:30-32). Key calls: `addWindow(window)` (app/Viewer.h:41); `assignRecordAndSubmitTaskAndPresentation(commandGraphs)` to wire tasks/presentation (app/Viewer.h:124); `compile(hints={})` to allocate GPU resources (app/Viewer.h:107); `advanceToNextFrame()`, `handleEvents()`, `update()`, `recordAndSubmit()`, `present()` for the loop (app/Viewer.h:102-139); `active()` to test the loop condition (app/Viewer.h:56); `addEventHandler(handler)` (app/Viewer.h:71); `addUpdateOperation(op, runBehavior)` (app/Viewer.h:85).
- **`CommandGraph`** — top-of-graph node recording its subgraph to command buffers (app/CommandGraph.h:26-27). Construct with `CommandGraph::create(window, child={})` (app/CommandGraph.h:32); fields `window`, `framebuffer`, `device`, `queueFamily`, `presentFamily`, `submitOrder` (app/CommandGraph.h:36-42); `record(...)` is invoked by the task each frame (app/CommandGraph.h:51).
- **`RenderGraph`** — begins/ends the render pass around its children (app/RenderGraph.h:25-39). Construct with `RenderGraph::create(window, view={})` (app/RenderGraph.h:34); fields `clearValues`, `renderArea`, `contents`, `window`/`framebuffer` (app/RenderGraph.h:42-69); `setClearValues(...)` after a window/framebuffer is assigned (app/RenderGraph.h:64-66); `windowResizeHandler` does resize handling by default (app/RenderGraph.h:73-75).
- **`View`** — pairs `Camera` + scene subgraph (app/View.h:34-43). Construct with `View::create(camera, scenegraph={}, features=RECORD_ALL)` (app/View.h:43); fields `camera`, `mask`, `viewID` (auto-assigned), `features` (app/View.h:67-78).
- **`Camera`** — supplies `projectionMatrix`, `viewMatrix`, `viewportState` (app/Camera.h:23-36); construct with `Camera::create(projection, view, viewport={})` (app/Camera.h:31).
- **`Window`** — cross-platform window created by `Window::create(traits)` (app/Window.h:36-37); `extent2D()` (app/Window.h:64), `acquireNextImage(timeout)` (app/Window.h:112-113), per-frame `Frame` carrying framebuffer and semaphores (app/Window.h:120-126).
- **`RecordAndSubmitTask`** — records command graphs and submits to a queue (app/RecordAndSubmitTask.h:26-30); fields `commandGraphs`, `waitSemaphores`, `signalSemaphores`, `queue`, `transferTask` (app/RecordAndSubmitTask.h:38-44); `submit(frameStamp)` (app/RecordAndSubmitTask.h:32); `fence(relativeFrameIndex)` for sync (app/RecordAndSubmitTask.h:59).
- **`CompileManager`** — "compiles subgraphs for the windows/framebuffers associated with" it (app/CompileManager.h:55-56); `compile(object, ...)` and `compileTask(task, ...)` return a `CompileResult` (app/CompileManager.h:82-85); used at runtime to compile subgraphs added after initial `Viewer::compile()` (app/Viewer.h:94).
- **`CompileResult`** — result of compile traversal; `operator bool()` is true on `VK_SUCCESS`, and `requiresViewerUpdate(viewer)` guides whether `updateViewer` must run (app/CompileManager.h:27-41).
- **`updateViewer(viewer, compileResult)`** — "update Viewer data structures to match the needs of newly compiled subgraphs" (app/Viewer.h:172; src/vsg/app/Viewer.cpp:887-892).
- **`createCommandGraphForView(window, camera, scenegraph, ...)`** — convenience builder that returns a `CommandGraph` containing a `RenderGraph`+`View` for the scene (app/CommandGraph.h:65-66).

## Idiomatic wiring

The standard setup builds the camera, wraps it with the scene into a command graph, assigns it to the viewer, compiles once, then runs the loop. Wiring of tasks and presentation is done for you by `assignRecordAndSubmitTaskAndPresentation`: it groups command graphs by device+queue-family, and for any group with a `presentFamily >= 0` it creates a `RecordAndSubmitTask` (assigning `commandGraphs`, `windows`, and `queue`) plus a `Presentation` whose `queue` is the present queue (src/vsg/app/Viewer.cpp:483-556). `Viewer::compile()` then allocates resources for every assigned task (src/vsg/app/Viewer.cpp:356-403). In the loop, `recordAndSubmit()` resets each command graph and calls `task->submit(frameStamp)` (src/vsg/app/Viewer.cpp:820-852), `submit()` runs start/transfer/record/finish with `record()` delegating to `commandGraph->record(...)` (src/vsg/app/RecordAndSubmitTask.cpp:85-153), and `present()` drives each `Presentation` (src/vsg/app/Viewer.cpp:854-862).

```cpp
auto windowTraits = vsg::WindowTraits::create();
auto window = vsg::Window::create(windowTraits);

auto viewer = vsg::Viewer::create();
viewer->addWindow(window);                              // app/Viewer.h:41

// Camera: projection + view + viewport sized to the window.
auto perspective = vsg::Perspective::create();
auto lookAt = vsg::LookAt::create();
auto viewportState = vsg::ViewportState::create(window->extent2D()); // app/Window.h:64
auto camera = vsg::Camera::create(perspective, lookAt, viewportState); // app/Camera.h:31

// CommandGraph -> RenderGraph -> View(camera, scene) in one call.
auto commandGraph = vsg::createCommandGraphForView(window, camera, scene); // src/vsg/app/CommandGraph.cpp:141

// Wire RecordAndSubmitTask + Presentation, then compile GPU resources ONCE.
viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph}); // app/Viewer.h:124
viewer->compile();                                                // app/Viewer.h:107

while (viewer->advanceToNextFrame())   // src/vsg/app/Viewer.cpp:155
{
    viewer->handleEvents();            // src/vsg/app/Viewer.cpp:265
    viewer->update();                  // src/vsg/app/Viewer.cpp:793
    viewer->recordAndSubmit();         // src/vsg/app/Viewer.cpp:820
    viewer->present();                 // src/vsg/app/Viewer.cpp:854
}
```

## Rules

- Call `assignRecordAndSubmitTaskAndPresentation(commandGraphs)` before `compile()`, since `compile()` returns empty when `recordAndSubmitTasks` is empty (src/vsg/app/Viewer.cpp:282-285).
- Call `Viewer::compile()` exactly once after assignment and before the frame loop to allocate all GPU objects (app/CompileTraversal.h:32; src/vsg/app/Viewer.cpp:356-403).
- Drive the loop with the four calls in order — `handleEvents`, `update`, `recordAndSubmit`, `present` — after `advanceToNextFrame()` returns true (src/vsg/app/Viewer.cpp:155-206).
- Use `while (viewer->advanceToNextFrame())` as the loop condition; it returns false once the viewer is no longer `active()` (app/Viewer.h:99-102; src/vsg/app/Viewer.cpp:162-165).
- When you add subgraphs after the initial compile, compile them through `viewer->compileManager` and apply the result via `updateViewer(viewer, compileResult)` so task data structures stay consistent (app/Viewer.h:94,172; src/vsg/app/Viewer.cpp:887-892).
- Construct every VSG object with `ClassName::create(...)` returning a `ref_ptr`; never use raw `new` (app/Window.h:36-37, app/CommandGraph.h:32).
- Build the `Camera` viewport from the window extent so the render area matches the swapchain: `vsg::ViewportState::create(window->extent2D())` (compile-probe confirmed; app/Camera.h:36-39, app/Window.h:64).
- Let the default `RenderGraph::windowResizeHandler` handle resizes rather than recomputing viewports manually (app/RenderGraph.h:73-75).

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| Calling `viewer->compile()` before assigning command graphs | `assignRecordAndSubmitTaskAndPresentation({cg})` then `compile()` | `compile()` returns an empty `CompileResult` when no tasks are assigned (src/vsg/app/Viewer.cpp:282-285) |
| Building `View`/`RenderGraph`/`CommandGraph` by hand and forgetting the render pass | `createCommandGraphForView(window, camera, scene)` | The builder nests a `RenderGraph` (begin/end render pass) over the `View` for you (src/vsg/app/CommandGraph.cpp:141-148) |
| `recordAndSubmit()` then `update()` then `present()` | `update()` -> `recordAndSubmit()` -> `present()` | `update()` runs update ops/animations and `updateViewer` before recording (src/vsg/app/Viewer.cpp:793-818) |
| Adding scene nodes mid-loop and recording them with no compile | compile via `viewer->compileManager` + `updateViewer(...)` | New subgraphs have no GPU objects until compiled (app/CompileTraversal.h:32; src/vsg/app/Viewer.cpp:887-892) |
| `while (true)` loop that never checks shutdown | `while (viewer->advanceToNextFrame())` | `advanceToNextFrame()` returns false once inactive, also polling events and advancing fences (src/vsg/app/Viewer.cpp:155-206) |

## Source references

- include/vsg/app/Viewer.h
- include/vsg/app/Window.h
- include/vsg/app/CommandGraph.h
- include/vsg/app/RenderGraph.h
- include/vsg/app/RecordAndSubmitTask.h
- include/vsg/app/View.h
- include/vsg/app/Camera.h
- include/vsg/app/CompileManager.h
- include/vsg/app/CompileTraversal.h
- include/vsg/app/RecordTraversal.h
- src/vsg/app/Viewer.cpp
- src/vsg/app/RecordAndSubmitTask.cpp
- src/vsg/app/CommandGraph.cpp
