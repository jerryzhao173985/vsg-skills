# Scene graph composition, transforms & state

A VSG scene is a directed acyclic graph of `vsg::Node` objects. Internal nodes describe structure and context — grouping (`Group`), spatial position (`MatrixTransform`, `CoordinateFrame`, `AbsoluteTransform`), and which GPU state applies (`StateGroup`) — while leaf command nodes (`Geometry`, `VertexIndexDraw`) carry the vertex data and the Vulkan draw calls that actually render. The central contract an app author must internalize is the separation between *what the scene contains* (the node hierarchy, built once on the CPU) and *how it is rendered* (a `RecordTraversal` that walks the graph each frame, pushing inherited transform and state onto stacks and replaying them into a Vulkan command buffer when it reaches a draw). Get this separation right and composition is declarative; get it wrong and state leaks across subtrees or transforms accumulate incorrectly.

## The model

Every internal scene node derives from `vsg::Node`, itself `Inherit<Object, Node>` (nodes/Node.h:23). The workhorse container is `Group`, which holds `Children children` — a vector of `ref_ptr<Node>` — and exposes `addChild(ref_ptr<Node>)` to append (nodes/Group.h:36-42). Composition is literally nesting: a `Group` whose children are other groups, transforms, state groups, and ultimately command leaves. Traversal is uniform: `Group::traverse` simply calls `child->accept(visitor)` for every child (nodes/Group.h:48-56), so any visitor — including the per-frame `RecordTraversal` — descends the whole subtree automatically.

**Transforms accumulate down the graph.** `Transform` is a pure-virtual base (`Inherit<Group, Transform>`) declaring `virtual dmat4 transform(const dmat4& mv) const = 0` (nodes/Transform.h:21,34). The argument `mv` is the modelview matrix inherited from above; the method returns the new matrix for the subtree. `MatrixTransform` stores a `dmat4 matrix` and implements `transform` as `mv * matrix` (nodes/MatrixTransform.h:30-32), so nesting transforms post-multiplies — the deepest transform is applied to geometry first. During recording, `RecordTraversal::apply(const MatrixTransform&)` pushes the node onto `state->modelviewMatrixStack`, traverses the subgraph, then pops it (src/vsg/app/RecordTraversal.cpp:423-442); the generic `apply(const Transform&)` does the same for any subclass (src/vsg/app/RecordTraversal.cpp:401-419). All transform matrices are `dmat4` (double precision; maths/mat4.h:123) — VSG keeps positions in doubles on the CPU so that whole-earth scenes do not lose precision before the matrix is reduced to floats for the GPU.

**Whole-earth transforms.** For astronomically large coordinates, `AbsoluteTransform` ignores the inherited matrix entirely — `transform(const dmat4&) const` returns its own `matrix` (nodes/AbsoluteTransform.h:30-32) — so its subgraph is positioned in an absolute frame regardless of ancestors. `CoordinateFrame` carries a `dvec3 origin` and `dquat rotation` (nodes/CoordinateFrame.h:28-29); at record time, if a view is active, the traversal rebases it against the camera's view matrix via `viewMatrix->transform(cf.origin) * vsg::rotate(cf.rotation)` (src/vsg/app/RecordTraversal.cpp:455), keeping the eye near the origin to preserve floating-point precision.

**State binds to a subtree.** `StateGroup` is `Inherit<Group, StateGroup>` plus a `StateCommands stateCommands` list and an `add(ref_ptr<StateCommand>)` method (nodes/StateGroup.h:31-51). A `StateCommand` is the base for Vulkan state-binding commands such as binding a graphics pipeline or descriptor sets (state/StateCommand.h:23). When the `RecordTraversal` reaches a `StateGroup` it calls `state->push(begin, end)` over the state commands, traverses the children, then `state->pop(begin, end)` (src/vsg/app/RecordTraversal.cpp:515-529). That push/pop scoping is the whole point: pipeline and descriptor bindings declared on a `StateGroup` apply to its entire subtree and are unwound on exit, so siblings do not inherit each other's state.

**Pipelines describe the GPU configuration.** `GraphicsPipeline` encapsulates the `VkPipeline` and its `VkGraphicsPipelineCreateInfo` — `ShaderStages stages`, `GraphicsPipelineStates pipelineStates`, and a `ref_ptr<PipelineLayout> layout` (state/GraphicsPipeline.h:56-73). You do not put a `GraphicsPipeline` directly in the graph; you wrap it in a `BindGraphicsPipeline` state command (`Inherit<StateCommand, BindGraphicsPipeline>`, holding `ref_ptr<GraphicsPipeline> pipeline`; state/GraphicsPipeline.h:110-116) and add that to a `StateGroup`.

**Leaves carry data and draw calls.** `Geometry` is `Inherit<Command, Geometry>` — a command, not a group — holding `BufferInfoList arrays`, an optional `ref_ptr<BufferInfo> indices`, and a `DrawCommands commands` list, populated via `assignArrays(const DataList&)` / `assignIndices(ref_ptr<Data>)` (nodes/Geometry.h:29-44). `VertexIndexDraw` is the lighter-weight equivalent that binds arrays and indices then issues a single `vkCmdDrawIndexed`, exposing the draw parameters directly as fields: `indexCount`, `instanceCount`, `firstIndex`, `vertexOffset`, `firstInstance` (nodes/VertexIndexDraw.h:24-43). When the traversal reaches either leaf it first flushes pending state with `state->record()`, then calls the leaf's `record(*commandBuffer)` (src/vsg/app/RecordTraversal.cpp:343-359). This is the moment the accumulated transform stack and the pushed state commands are materialized into Vulkan calls immediately before the draw.

So the data flow each frame is: descend the graph → transforms push onto the modelview stack → state groups push state commands → at a leaf, flush state and record the draw → on the way back up, pop everything. Nodes are immutable structure built once; the traversal is the engine that turns structure into commands.

## Key types & calls

- **`vsg::Node`** — base class for all internal scene nodes, `Inherit<Object, Node>` (nodes/Node.h:23). Constructed via subclasses' `::create`.
- **`vsg::Group`** — holds children (nodes/Group.h:23). Touch: `children` (the `std::vector<ref_ptr<Node>>`, nodes/Group.h:36-37); `addChild(ref_ptr<Node>)` to append (nodes/Group.h:39).
- **`vsg::Transform`** — pure-virtual spatial base (nodes/Transform.h:21). Touch: `transform(const dmat4& mv)` override contract (nodes/Transform.h:34); `subgraphRequiresLocalFrustum` flag to skip frustum work when the subtree has no culling nodes (nodes/Transform.h:30).
- **`vsg::MatrixTransform`** — relative transform holding `dmat4 matrix`; `transform` returns `mv * matrix` (nodes/MatrixTransform.h:30-32). Construct with `MatrixTransform::create(dmat4)` (nodes/MatrixTransform.h:28).
- **`vsg::AbsoluteTransform`** — replaces inherited matrix with its own `dmat4 matrix` (nodes/AbsoluteTransform.h:30-32).
- **`vsg::CoordinateFrame`** — large-coordinate transform with `dvec3 origin` and `dquat rotation` (nodes/CoordinateFrame.h:28-29).
- **`vsg::CullGroup`** — group with a `dsphere bound` enabling view-frustum culling of its children (nodes/CullGroup.h:22-29).
- **`vsg::StateGroup`** — binds state to a subtree. Touch: `stateCommands` list (nodes/StateGroup.h:37); `add(ref_ptr<StateCommand>)` (nodes/StateGroup.h:48); optional `prototypeArrayState` for custom vertex-array mapping (nodes/StateGroup.h:40).
- **`vsg::StateCommand`** — base for state-binding Vulkan commands; carries a `uint32_t slot` (state/StateCommand.h:23,36).
- **`vsg::GraphicsPipeline`** — the pipeline object: `stages`, `pipelineStates`, `layout` (state/GraphicsPipeline.h:56-72).
- **`vsg::BindGraphicsPipeline`** — state command wrapping a `ref_ptr<GraphicsPipeline> pipeline` (state/GraphicsPipeline.h:110-116).
- **`vsg::Geometry`** — leaf command with `arrays`, `indices`, `commands`; `assignArrays`/`assignIndices` (nodes/Geometry.h:39-44).
- **`vsg::VertexIndexDraw`** — leaf command with explicit draw fields and `assignArrays`/`assignIndices` (nodes/VertexIndexDraw.h:32-43).
- **`vsg::translate` / `rotate` / `scale`** — build `t_mat4<T>` matrices for composition (maths/transform.h:21,45,67,86). **`vsg::inverse`** computes a matrix inverse (maths/transform.h:251,254). `dmat4`'s default constructor is the identity (maths/mat4.h:32-37).

## Idiomatic wiring

The graph is built bottom-up with `::create` (never raw `new`; every type uses `vsg::Inherit`), then assembled by `addChild`. `Group::addChild` is a plain `push_back` into the children vector (nodes/Group.h:39-42), and `StateGroup::add` is a `push_back` into `stateCommands` (nodes/StateGroup.h:48-51). The standard arrangement is: a `StateGroup` at the top binding the pipeline and descriptors, containing one or more `MatrixTransform` nodes for placement, each holding geometry leaves. At record time the traversal pushes the state commands (src/vsg/app/RecordTraversal.cpp:524), pushes each transform onto `modelviewMatrixStack` (src/vsg/app/RecordTraversal.cpp:427), and at the leaf flushes with `state->record()` before recording the draw (src/vsg/app/RecordTraversal.cpp:357-358) — then pops back out (src/vsg/app/RecordTraversal.cpp:441,528).

```cpp
#include <vsg/all.h>

// State scope: bind a graphics pipeline to everything below.
auto stateGroup = vsg::StateGroup::create();
stateGroup->add(vsg::BindGraphicsPipeline::create(graphicsPipeline)); // graphicsPipeline: ref_ptr<vsg::GraphicsPipeline>

// Position a subtree with a double-precision matrix (translate then rotate).
auto transform = vsg::MatrixTransform::create(
    vsg::translate(10.0, 0.0, 0.0) * vsg::rotate(vsg::PI * 0.5, 0.0, 0.0, 1.0));

// Leaf: vertex/index arrays plus the indexed draw.
auto vid = vsg::VertexIndexDraw::create();
vid->assignArrays(vsg::DataList{vertices});  // vertices: ref_ptr<vsg::vec3Array>, etc.
vid->assignIndices(indices);                 // indices: ref_ptr<vsg::ushortArray>
vid->indexCount = static_cast<uint32_t>(indices->size());
vid->instanceCount = 1;

transform->addChild(vid);
stateGroup->addChild(transform);
// stateGroup is now a renderable subgraph: add it under the scene root.
```

## Rules

- Construct every node and command with `ClassName::create(...)` and hold it in a `ref_ptr`; never use raw `new` (nodes/Node.h:23, via `vsg::Inherit`).
- Bind a pipeline or descriptors by adding a `StateCommand` to a `StateGroup`, not by inserting a bare `GraphicsPipeline` into the graph (nodes/StateGroup.h:48, state/GraphicsPipeline.h:110).
- Place a `BindGraphicsPipeline` on the `StateGroup` above every draw leaf in its subtree, because pushed state is popped when the subtree ends (src/vsg/app/RecordTraversal.cpp:524-528).
- Use `MatrixTransform` (relative, `mv * matrix`) for normal placement and `AbsoluteTransform` only when the subtree must ignore inherited transforms (nodes/MatrixTransform.h:32, nodes/AbsoluteTransform.h:32).
- Order matrix factors as `translate * rotate * scale` since `MatrixTransform::transform` post-multiplies the inherited matrix (nodes/MatrixTransform.h:32, maths/transform.h:67,45,86).
- Keep transform matrices as `dmat4` and prefer `CoordinateFrame`/`AbsoluteTransform` for whole-earth scenes to avoid float precision loss (nodes/MatrixTransform.h:30, nodes/CoordinateFrame.h:28).
- Populate leaves through `assignArrays`/`assignIndices` rather than poking `arrays`/`indices` directly, so internal buffer setup is consistent (nodes/Geometry.h:43-44, nodes/VertexIndexDraw.h:42-43).
- Set `indexCount` (and `instanceCount`) on a `VertexIndexDraw`; it defaults to 0 and would draw nothing (nodes/VertexIndexDraw.h:32-33).
- Set `subgraphRequiresLocalFrustum = false` on a transform only when its subtree contains no culling nodes (LOD/CullGroup), to skip frustum transformation work (nodes/Transform.h:27-30, src/vsg/app/RecordTraversal.cpp:430).

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| Adding a `GraphicsPipeline` directly as a child of a `Group` | Wrap it in `BindGraphicsPipeline::create(pipeline)` and `StateGroup::add(...)` | `GraphicsPipeline` is `Inherit<Object, ...>`, not a `Node`/`StateCommand`; only `StateCommand`s are pushed during recording (state/GraphicsPipeline.h:56,110, src/vsg/app/RecordTraversal.cpp:524) |
| Putting draws as siblings of a `StateGroup` and expecting its pipeline to apply | Make the draws children (descendants) of the `StateGroup` | State is pushed on entry and popped on exit of the `StateGroup`'s subtree, so only descendants see it (src/vsg/app/RecordTraversal.cpp:524-528) |
| Building a placement matrix as `rotate * translate` expecting world-space translation | Use `vsg::translate(...) * vsg::rotate(...)` | `MatrixTransform::transform` returns `mv * matrix`, post-multiplying, so the rightmost factor is applied to vertices first (nodes/MatrixTransform.h:32) |
| Creating a `VertexIndexDraw` with arrays but leaving `indexCount` at its default | Set `vid->indexCount = indices->size()` after `assignIndices` | `indexCount` defaults to 0, so the `vkCmdDrawIndexed` renders nothing (nodes/VertexIndexDraw.h:32) |
| Setting `node.children.push_back(...)` on a raw pointer | Call `parent->addChild(child)` on a `ref_ptr` | `addChild` stores a `ref_ptr<Node>`, keeping the child alive via reference counting (nodes/Group.h:39-42) |

## Source references

- include/vsg/nodes/Node.h
- include/vsg/nodes/Group.h
- include/vsg/nodes/StateGroup.h
- include/vsg/nodes/Transform.h
- include/vsg/nodes/MatrixTransform.h
- include/vsg/nodes/AbsoluteTransform.h
- include/vsg/nodes/CoordinateFrame.h
- include/vsg/nodes/CullGroup.h
- include/vsg/nodes/Geometry.h
- include/vsg/nodes/VertexIndexDraw.h
- include/vsg/state/StateCommand.h
- include/vsg/state/GraphicsPipeline.h
- include/vsg/maths/transform.h
- include/vsg/maths/mat4.h
- src/vsg/nodes/Group.cpp
- src/vsg/nodes/StateGroup.cpp
- src/vsg/nodes/MatrixTransform.cpp
- src/vsg/app/RecordTraversal.cpp
- https://vsg-dev.github.io/vsgTutorial/scenegraph (project tutorial framing)
