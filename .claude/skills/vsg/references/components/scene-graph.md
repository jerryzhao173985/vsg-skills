# Scene graph (nodes)

The `vsg/nodes/` module is the structural backbone of a VSG application: it provides the node types you assemble into a scene graph. Internal nodes (groups, transforms, switches, LODs) organize and position subgraphs, while leaf command nodes (`Geometry`, `VertexIndexDraw`, `VertexDraw`) carry the actual vertex arrays and Vulkan draw commands recorded each frame. You reach for this module whenever you build the content that a `vsg::Viewer` renders: parent `Group`s hold children, a `StateGroup` binds a graphics pipeline and descriptors to everything below it, a `MatrixTransform` positions a subgraph, and geometry leaves draw it. Every class derives (directly or indirectly) from `vsg::Node` (nodes/Node.h:23) and is created through the static `ClassName::create(...)` factory returning a `ref_ptr`.

## Public includes

```cpp
#include <vsg/all.h>            // pulls in every node type plus the rest of VSG

// or include only what you use:
#include <vsg/nodes/Group.h>
#include <vsg/nodes/StateGroup.h>
#include <vsg/nodes/MatrixTransform.h>
#include <vsg/nodes/Switch.h>
#include <vsg/nodes/LOD.h>
#include <vsg/nodes/VertexIndexDraw.h>
#include <vsg/nodes/Geometry.h>
```

`<vsg/all.h>` is the umbrella header. The specific headers live under `include/vsg/nodes/` (e.g. nodes/Group.h:16, nodes/StateGroup.h:31).

## Key classes

### vsg::Node
Base class for all internal scene-graph nodes (nodes/Node.h:21). You rarely instantiate it directly; you derive from it or use its subclasses.
- Derives from `Inherit<Object, Node>`, so it gets the static `Node::create(...)` factory and `ref_ptr` semantics (nodes/Node.h:23).
- `ref_ptr<Object> clone(const CopyOp& copyop = {}) const` for deep/shallow copies (nodes/Node.h:29).
- Uses a custom allocator affinity (`MEMORY_NODES_OBJECTS`) via overridden `operator new`/`delete` (nodes/Node.h:32-33); just use `create()` and this is automatic.

### vsg::Group
Group node providing a list of children — the standard branch node (nodes/Group.h:22).
- `Children children;` a `std::vector<ref_ptr<Node>>` of child nodes (nodes/Group.h:36-37).
- `void addChild(vsg::ref_ptr<Node> child)` appends a child (nodes/Group.h:39).
- `explicit Group(size_t numChildren = 0)` constructor reserves child slots; prefer `Group::create()` (nodes/Group.h:26).
- You can also iterate/modify `children` directly since it is public (nodes/Group.h:37).

### vsg::StateGroup
A `Group` that binds Vulkan state (e.g. a `BindGraphicsPipeline`, `BindDescriptorSets`) to its entire subgraph during the record traversal (nodes/StateGroup.h:31). State is pushed before children record and popped after.
- `StateCommands stateCommands;` the list of state commands applied to the subgraph (nodes/StateGroup.h:37).
- `void add(ref_ptr<StateCommand> stateCommand)` appends a state command (nodes/StateGroup.h:48).
- Inherits `addChild` / `children` from `Group` for the subgraph the state applies to (nodes/Group.h:39).
- `ref_ptr<ArrayState> prototypeArrayState;` optional custom mapping when array 0 is not xyz vertices (nodes/StateGroup.h:40).
- `template<class T> bool contains(const T value) const` and `remove(value)` to query/remove a command (nodes/StateGroup.h:43, nodes/StateGroup.h:54).

### vsg::Transform
Pure-virtual base for positioning/scaling/rotating subgraphs; it is a `Group` (nodes/Transform.h:21). You use a concrete subclass such as `MatrixTransform`.
- `virtual dmat4 transform(const dmat4& mv) const = 0;` returns the matrix to push, given the inherited modelview (nodes/Transform.h:34).
- `bool subgraphRequiresLocalFrustum = true;` set to `false` to skip frustum transform when the subgraph has no culling nodes (nodes/Transform.h:30).
- Inherits `children` / `addChild` from `Group` (nodes/Group.h:37).

### vsg::MatrixTransform
A `Transform` that applies a 4x4 `dmat4` to its subgraph (nodes/MatrixTransform.h:23). During the record traversal the matrix is multiplied onto the modelview stack.
- `dmat4 matrix;` the transform applied to children (nodes/MatrixTransform.h:30).
- `explicit MatrixTransform(const dmat4& in_matrix)` constructor to set the matrix at creation; usable as `MatrixTransform::create(matrix)` (nodes/MatrixTransform.h:28).
- `dmat4 transform(const dmat4& mv) const final { return mv * matrix; }` pre-multiplies inherited modelview (nodes/MatrixTransform.h:32).
- Inherits `addChild` / `children` (nodes/Group.h:39).

### vsg::Switch
Node for toggling on/off the recording of children via masks (nodes/Switch.h:24).
- `void addChild(Mask mask, ref_ptr<Node> child)` and `void addChild(bool enabled, ref_ptr<Node> child)` add children (nodes/Switch.h:40, nodes/Switch.h:43).
- `void setAllChildren(bool enabled)` turns every child on/off (nodes/Switch.h:46).
- `void setSingleChildOn(size_t index)` turns one child on, all others off (nodes/Switch.h:49).
- `Children children;` a vector of `Child{ Mask mask; ref_ptr<Node> node; }` (nodes/Switch.h:30-37).
- Note: `Switch` is a `Node`, not a `Group`; its children carry masks (nodes/Switch.h:36).

### vsg::LOD
Level-of-Detail node: selects at most one child based on the screen-space size of its bound sphere (nodes/LOD.h:32). Order children highest-resolution first.
- `dsphere bound;` bounding sphere tested for culling and screen-height ratio (nodes/LOD.h:46).
- `Children children;` vector of `Child{ double minimumScreenHeightRatio; ref_ptr<Node> node; }` (nodes/LOD.h:38-47).
- `void addChild(const Child& lodChild)` appends a LOD child (nodes/LOD.h:49).
- A `minimumScreenHeightRatio` of `0.0` means always visible (nodes/LOD.h:40); LOD also performs view-frustum culling, so no separate CullNode is needed (nodes/LOD.h:31).

### vsg::PagedLOD
Like `LOD` but pages a high-resolution child in from an external file on demand (nodes/PagedLOD.h:35). Exactly two children.
- `Path filename;` external file loaded when `children[0]` is null (nodes/PagedLOD.h:48).
- `Children children;` a `std::array<Child, 2>` of `Child{ double minimumScreenHeightRatio; ref_ptr<Node> node; }` (nodes/PagedLOD.h:41-53).
- `dsphere bound;` bounding sphere for culling and screen-height test (nodes/PagedLOD.h:50).
- `ref_ptr<Options> options;` read options used when paging the file (nodes/PagedLOD.h:84).
- The paging state (priority, request status, frame counters) is managed internally by the viewer's database pager; app authors normally just set `filename`, `bound`, and the low-res `children[1]` (nodes/PagedLOD.h:86-110).

### vsg::CullGroup
A `Group` that enables view-frustum culling over its children against a bounding sphere (nodes/CullGroup.h:22).
- `dsphere bound;` the sphere tested against the frustum; if outside, the subgraph is skipped (nodes/CullGroup.h:29).
- `explicit CullGroup(const dsphere& bound)` constructor; usable as `CullGroup::create(bound)` (nodes/CullGroup.h:27).
- Inherits `addChild` / `children` from `Group` (nodes/Group.h:39).

### vsg::CullNode
View-frustum culling for a single child node (nodes/CullNode.h:24). Lighter than `CullGroup` when you only have one child.
- `dsphere bound;` and `ref_ptr<vsg::Node> child;` (nodes/CullNode.h:31-32).
- `CullNode(const dsphere& in_bound, Node* in_child)` constructor; usable as `CullNode::create(bound, child)` (nodes/CullNode.h:29).
- A valid `child` must always be set — there are no null checks for performance reasons (nodes/CullNode.h:22-23).

### vsg::DepthSorted
Decorates a single child to place it into a numbered bin for back-to-front depth sorting, typically for translucent geometry (nodes/DepthSorted.h:26).
- `int32_t binNumber = 0;` target bin (nodes/DepthSorted.h:33).
- `dsphere bound;` sphere tested against the frustum (nodes/DepthSorted.h:34).
- `ref_ptr<Node> child;` the decorated subgraph (nodes/DepthSorted.h:35).
- `DepthSorted(int32_t in_binNumber, const dsphere& in_bound, ref_ptr<Node> in_child)` constructor; usable as `DepthSorted::create(binNumber, bound, child)` (nodes/DepthSorted.h:31).

### vsg::Geometry
A leaf command node holding vertex arrays, an optional index array, and a list of draw commands recorded during the record traversal (nodes/Geometry.h:29). Lower CPU overhead than separate `BindVertexBuffers`/`BindIndexBuffer`/draw commands.
- `void assignArrays(const DataList& in_arrays)` sets the vertex arrays (nodes/Geometry.h:43).
- `void assignIndices(ref_ptr<vsg::Data> in_indices)` sets the index array (nodes/Geometry.h:44).
- `DrawCommands commands;` a `std::vector<ref_ptr<Command>>` of draw commands, e.g. `DrawIndexed` (nodes/Geometry.h:35, nodes/Geometry.h:41).
- `BufferInfoList arrays;` / `ref_ptr<BufferInfo> indices;` the underlying buffers (set via the assign methods) (nodes/Geometry.h:39-40).
- `uint32_t firstBinding = 0;` first vertex-buffer binding index (nodes/Geometry.h:38).
- It is a `Command` (not a `Group`), so place it under a `StateGroup` that binds the pipeline (nodes/Geometry.h:29).

### vsg::VertexIndexDraw
Lightweight leaf that binds vertex arrays and indices, then issues a single `vkCmdDrawIndexed` (nodes/VertexIndexDraw.h:24). Higher performance than separate bind + draw commands.
- `void assignArrays(const DataList& in_arrays)` and `void assignIndices(ref_ptr<Data> in_indices)` (nodes/VertexIndexDraw.h:42-43).
- Draw parameters: `uint32_t indexCount`, `instanceCount`, `firstIndex`, `vertexOffset`, `firstInstance` (nodes/VertexIndexDraw.h:32-36).
- `uint32_t firstBinding = 0;` first vertex-buffer binding (nodes/VertexIndexDraw.h:38).
- Set `indexCount` to match the index array, otherwise nothing is drawn (nodes/VertexIndexDraw.h:32).

### vsg::VertexDraw
Like `VertexIndexDraw` but with no index buffer; binds vertex arrays and issues `vkCmdDraw` (nodes/VertexDraw.h:24).
- `void assignArrays(const DataList& in_arrays)` sets the vertex arrays (nodes/VertexDraw.h:40).
- Draw parameters: `uint32_t vertexCount`, `instanceCount`, `firstVertex`, `firstInstance` (nodes/VertexDraw.h:32-35).
- `uint32_t firstBinding = 0;` first vertex-buffer binding (nodes/VertexDraw.h:37).

## Idioms

Build a positioned, state-bound geometry subgraph:

```cpp
auto scene = vsg::StateGroup::create();
scene->add(bindGraphicsPipeline);   // a ref_ptr<StateCommand>

auto transform = vsg::MatrixTransform::create(vsg::translate(0.0, 0.0, 5.0));
scene->addChild(transform);

auto vid = vsg::VertexIndexDraw::create();
vid->assignArrays(vsg::DataList{vertices, normals, texcoords});
vid->assignIndices(indices);
vid->indexCount = static_cast<uint32_t>(indices->valueCount());
vid->instanceCount = 1;
transform->addChild(vid);
```

Geometry leaf with explicit draw commands:

```cpp
auto geometry = vsg::Geometry::create();
geometry->assignArrays(vsg::DataList{vertices});
geometry->assignIndices(indices);
geometry->commands.push_back(vsg::DrawIndexed::create(indexCount, 1, 0, 0, 0));
```

Toggle visibility with a Switch:

```cpp
auto sw = vsg::Switch::create();
sw->addChild(true, dayScene);     // enabled
sw->addChild(false, nightScene);  // disabled
sw->setSingleChildOn(1);          // show only nightScene
```

Level-of-detail with frustum culling built in:

```cpp
auto lod = vsg::LOD::create();
lod->bound = vsg::dsphere(center, radius);
lod->addChild(vsg::LOD::Child{0.5, highResModel});  // when sphere >= 50% screen height
lod->addChild(vsg::LOD::Child{0.0, lowResModel});   // always-visible fallback
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `new vsg::Group()` | `vsg::Group::create()` (nodes/Node.h:23) | All VSG objects use the `create()` factory returning a `ref_ptr`; raw `new` bypasses allocator affinity and reference counting. |
| Putting `Geometry`/`VertexIndexDraw` directly under a bare `Group` with no pipeline bound above | Place leaves under a `StateGroup` that has `add(bindGraphicsPipeline)` (nodes/StateGroup.h:48) | Command leaves only draw correctly when a graphics pipeline + descriptors are pushed by an enclosing `StateGroup`. |
| Calling `group->addChild(mask, child)` on a `Group` | Use `vsg::Switch` and its `addChild(Mask, child)` (nodes/Switch.h:40) | Only `Switch` carries per-child masks; `Group::addChild` takes a single `ref_ptr<Node>` (nodes/Group.h:39). |
| Creating an `LOD` and wrapping it in a `CullNode` | Set `lod->bound` and rely on LOD's built-in frustum culling (nodes/LOD.h:31) | `LOD`/`PagedLOD` already perform view-frustum culling against `bound`, so an extra cull node is redundant. |
| Leaving `VertexIndexDraw::indexCount` at 0 | Set `vid->indexCount = indices->valueCount()` (nodes/VertexIndexDraw.h:32) | The draw issues `vkCmdDrawIndexed` with `indexCount`; zero draws nothing. |

## Things to never invent

- `Group` has no `removeChild`, `getChild`, or `getNumChildren` accessor in this header — manipulate the public `children` vector directly (nodes/Group.h:37).
- `Geometry`/`VertexIndexDraw` have no `setArrays`/`setIndices` setters; the methods are `assignArrays` and `assignIndices` (nodes/Geometry.h:43-44, nodes/VertexIndexDraw.h:42-43).
- `MatrixTransform` exposes `matrix` as a public field, not `setMatrix`/`getMatrix` accessors (nodes/MatrixTransform.h:30).
- `StateGroup` uses `add(...)` for state commands, not `addStateCommand` (nodes/StateGroup.h:48).
- `Switch` is not a `Group`; do not expect `Group::children` (a `vector<ref_ptr<Node>>`) — its `children` is a vector of `Switch::Child` with masks (nodes/Switch.h:36).

## Source references

- include/vsg/nodes/Node.h
- include/vsg/nodes/Group.h
- include/vsg/nodes/StateGroup.h
- include/vsg/nodes/Transform.h
- include/vsg/nodes/MatrixTransform.h
- include/vsg/nodes/AbsoluteTransform.h
- include/vsg/nodes/CoordinateFrame.h
- include/vsg/nodes/Switch.h
- include/vsg/nodes/LOD.h
- include/vsg/nodes/PagedLOD.h
- include/vsg/nodes/CullGroup.h
- include/vsg/nodes/CullNode.h
- include/vsg/nodes/DepthSorted.h
- include/vsg/nodes/Geometry.h
- include/vsg/nodes/VertexIndexDraw.h
- include/vsg/nodes/VertexDraw.h
- include/vsg/nodes/QuadGroup.h
- include/vsg/nodes/Bin.h
- include/vsg/nodes/Layer.h
- include/vsg/nodes/Compilable.h
