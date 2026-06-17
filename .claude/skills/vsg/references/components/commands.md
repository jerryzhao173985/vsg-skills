# Commands

The `vsg/commands` module provides the leaf objects that record Vulkan `vkCmd*` calls into a command buffer during the record traversal. An app author reaches for these when building the geometry-recording portion of a scene graph: binding vertex/index buffers, issuing draws or compute dispatches, setting dynamic state (viewport/scissor), or inserting synchronization barriers. They are typically held inside a `vsg::Commands` group (or a higher-level geometry node) which sits under the pipeline/descriptor state nodes that configure how they execute.

## Public includes

```cpp
#include <vsg/all.h> // pulls in everything below

// or target specific headers:
#include <vsg/commands/Command.h>
#include <vsg/commands/Commands.h>
#include <vsg/commands/Draw.h>
#include <vsg/commands/DrawIndexed.h>
#include <vsg/commands/BindVertexBuffers.h>
#include <vsg/commands/BindIndexBuffer.h>
#include <vsg/commands/Dispatch.h>
#include <vsg/commands/SetViewport.h>
#include <vsg/commands/SetScissor.h>
#include <vsg/commands/PipelineBarrier.h>
```

## Key classes

### vsg::Command
Abstract base for objects that encapsulate `vkCmd*` calls and associated settings (commands/Command.h:21-22).
- Derives from `Compilable` via `Inherit<Compilable, Command>`, so commands participate in the compile traversal as well as recording (commands/Command.h:22, via `#include <vsg/nodes/Compilable.h>` at commands/Command.h:15).
- `virtual void record(CommandBuffer& commandBuffer) const = 0;` — pure virtual; every concrete command implements this to emit its Vulkan call (commands/Command.h:29).
- You never instantiate `Command` directly; use a concrete subclass via its `::create(...)` factory.

### vsg::Commands
A command that acts as a container for other commands; functionally equivalent to `vsg::Group` but faster because it has lower CPU overhead applying the state stack before the contained `Command` calls (commands/Commands.h:25-28).
- `static ref_ptr<Commands> create(size_t numChildren = 0)` — factory inherited from `Inherit`; the constructor takes an optional child count (commands/Commands.h:31).
- `Children children;` where `using Children = std::vector<ref_ptr<vsg::Command>>;` — the ordered list of child commands recorded in sequence (commands/Commands.h:48-49).
- `void addChild(vsg::ref_ptr<Command> child)` — append a child command (commands/Commands.h:51-54).
- `void compile(Context& context) override;` — compiles children (commands/Commands.h:56).
- `void record(CommandBuffer& commandBuffer) const override;` — records each child in order (commands/Commands.h:57).

### vsg::Draw
Encapsulates `vkCmdDraw` and its parameters (commands/Draw.h:22-23).
- `static ref_ptr<Draw> create(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)` — factory matching the 4-arg constructor (commands/Draw.h:28-32).
- Public fields, all default `0`: `uint32_t vertexCount`, `instanceCount`, `firstVertex`, `firstInstance` (commands/Draw.h:44-47).
- `record` emits `vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance)` (commands/Draw.h:39-42).

### vsg::DrawIndexed
Encapsulates `vkCmdDrawIndexed` and its parameters; use after a `BindIndexBuffer` (commands/DrawIndexed.h:22-23).
- `static ref_ptr<DrawIndexed> create(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)` (commands/DrawIndexed.h:28-33).
- Public fields: `uint32_t indexCount`, `instanceCount`, `firstIndex`, `vertexOffset`, `firstInstance` (commands/DrawIndexed.h:45-49). Note the field `vertexOffset` is declared `uint32_t` here even though the constructor parameter is `int32_t` (commands/DrawIndexed.h:28, commands/DrawIndexed.h:48).
- `record` emits `vkCmdDrawIndexed(...)` (commands/DrawIndexed.h:40-43).

### vsg::BindVertexBuffers
Encapsulates `vkCmdBindVertexBuffers`; binds one or more vertex arrays to consecutive binding slots (commands/BindVertexBuffers.h:23-24).
- `static ref_ptr<BindVertexBuffers> create(uint32_t firstBinding, const DataList& arrays)` — matching the constructor; `DataList` is `std::vector<ref_ptr<Data>>` (commands/BindVertexBuffers.h:28, core/Data.h:239).
- `uint32_t firstBinding = 0;` — the first binding slot (commands/BindVertexBuffers.h:30).
- `BufferInfoList arrays;` — the bound arrays, where `BufferInfoList = std::vector<ref_ptr<BufferInfo>>` (commands/BindVertexBuffers.h:31, state/BufferInfo.h:81).
- `void assignArrays(const DataList& in_arrays);` — set/replace the arrays from raw `Data` (commands/BindVertexBuffers.h:33).
- `void compile(Context& context) override;` — uploads the vertex buffers to the GPU (commands/BindVertexBuffers.h:40).
- `void record(CommandBuffer& commandBuffer) const override;` (commands/BindVertexBuffers.h:42).

### vsg::BindIndexBuffer
Encapsulates `vkCmdBindIndexBuffer`; binds the index buffer consumed by a subsequent `DrawIndexed` (commands/BindIndexBuffer.h:26-27).
- `static ref_ptr<BindIndexBuffer> create(ref_ptr<Data> indices)` — single-arg form; the index type is deduced from the data's value size (commands/BindIndexBuffer.h:32). There is also a `BindIndexBuffer(VkIndexType, ref_ptr<BufferInfo>)` form (commands/BindIndexBuffer.h:33).
- `VkIndexType indexType = VK_INDEX_TYPE_UINT16;` — defaults to 16-bit indices (commands/BindIndexBuffer.h:35).
- `ref_ptr<BufferInfo> indices;` — the bound index data (commands/BindIndexBuffer.h:36).
- `void assignIndices(ref_ptr<vsg::Data> in_indices);` (commands/BindIndexBuffer.h:38).
- Free function `VkIndexType computeIndexType(const Data* indices);` derives the Vulkan index type from a data source's value size (commands/BindIndexBuffer.h:24).
- `compile` and `record` overrides (commands/BindIndexBuffer.h:45, commands/BindIndexBuffer.h:47).

### vsg::Dispatch
Encapsulates `vkCmdDispatch`, used to dispatch a compute pipeline (commands/Dispatch.h:21-22).
- `static ref_ptr<Dispatch> create(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)` (commands/Dispatch.h:27-30).
- Public fields, all default `0`: `uint32_t groupCountX`, `groupCountY`, `groupCountZ` (commands/Dispatch.h:37-39).
- `void record(CommandBuffer& commandBuffer) const override;` (commands/Dispatch.h:35).

### vsg::SetViewport
Encapsulates `vkCmdSetViewport`, for dynamic updating of a `GraphicsPipeline`'s `ViewportState` (commands/SetViewport.h:21-22).
- `static ref_ptr<SetViewport> create(uint32_t firstViewport, const Viewports& viewports)` where `Viewports = std::vector<VkViewport>` (commands/SetViewport.h:26, state/ViewportState.h:19).
- `uint32_t firstViewport = 0;` (commands/SetViewport.h:28).
- `Viewports viewports;` (commands/SetViewport.h:29).
- `void record(CommandBuffer& commandBuffer) const override;` (commands/SetViewport.h:31).

### vsg::SetScissor
Encapsulates `vkCmdSetScissor`, for dynamic updating of a `GraphicsPipeline`'s `ViewportState` (commands/SetScissor.h:21-22).
- `static ref_ptr<SetScissor> create(uint32_t firstScissor, const Scissors& scissors)` where `Scissors = std::vector<VkRect2D>` (commands/SetScissor.h:26, state/ViewportState.h:20).
- `uint32_t firstScissor = 0;` (commands/SetScissor.h:28).
- `Scissors scissors;` (commands/SetScissor.h:29).
- `void record(CommandBuffer& commandBuffer) const override;` (commands/SetScissor.h:31).

### vsg::PipelineBarrier
Encapsulates `vkCmdPipelineBarrier` plus its lists of memory/buffer/image barriers; insert it to synchronize GPU work and perform image layout transitions (commands/PipelineBarrier.h:125-126).
- Variadic factory `static ref_ptr<PipelineBarrier> create(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask, VkDependencyFlags dependencyFlags, Args&&... barriers)` — extra args are forwarded to `add(...)` (commands/PipelineBarrier.h:131-138).
- `void add(ref_ptr<MemoryBarrier>)`, `void add(ref_ptr<BufferMemoryBarrier>)`, `void add(ref_ptr<ImageMemoryBarrier>)` — append barriers of each kind (commands/PipelineBarrier.h:142-144).
- Fields: `VkPipelineStageFlags srcStageMask`, `dstStageMask`, `VkDependencyFlags dependencyFlags` (commands/PipelineBarrier.h:146-148); the barrier lists `MemoryBarriers memoryBarriers`, `BufferMemoryBarriers bufferMemoryBarriers`, `ImageMemoryBarriers imageMemoryBarriers` (commands/PipelineBarrier.h:150-152).
- Supporting structs (each with `::create(...)`): `MemoryBarrier(srcAccessMask, dstAccessMask)` (commands/PipelineBarrier.h:32-44), `BufferMemoryBarrier(...)` (commands/PipelineBarrier.h:49-76), `ImageMemoryBarrier(srcAccessMask, dstAccessMask, oldLayout, newLayout, srcQueueFamilyIndex, dstQueueFamilyIndex, image, subresourceRange)` (commands/PipelineBarrier.h:81-111).

## Idioms

Build a geometry command list (bind buffers, then draw indexed) inside a `Commands` group:

```cpp
auto commands = vsg::Commands::create();
commands->addChild(vsg::BindVertexBuffers::create(0, vsg::DataList{vertices, colors}));
commands->addChild(vsg::BindIndexBuffer::create(indices)); // index type deduced
commands->addChild(vsg::DrawIndexed::create(
    static_cast<uint32_t>(indices->valueCount()), // indexCount
    1,  // instanceCount
    0,  // firstIndex
    0,  // vertexOffset
    0)); // firstInstance
```

A non-indexed draw of a triangle:

```cpp
auto commands = vsg::Commands::create();
commands->addChild(vsg::BindVertexBuffers::create(0, vsg::DataList{vertices}));
commands->addChild(vsg::Draw::create(3, 1, 0, 0)); // 3 verts, 1 instance
```

Dispatch a compute pipeline:

```cpp
auto dispatch = vsg::Dispatch::create(groupsX, groupsY, 1);
```

Transition an image layout with a PipelineBarrier:

```cpp
auto barrier = vsg::ImageMemoryBarrier::create(
    0, VK_ACCESS_SHADER_READ_BIT,
    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
    image,
    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

auto pipelineBarrier = vsg::PipelineBarrier::create(
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0, barrier);
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `new vsg::Draw(3,1,0,0)` | `vsg::Draw::create(3, 1, 0, 0)` | All VSG objects are created via the static `::create(...)` factory returning a `ref_ptr` (commands/Draw.h:28, Inherit pattern). |
| Issuing `DrawIndexed` without first binding indices | `commands->addChild(vsg::BindIndexBuffer::create(indices))` before the `DrawIndexed` | `DrawIndexed` records `vkCmdDrawIndexed` which requires a bound index buffer (commands/DrawIndexed.h:40-43, commands/BindIndexBuffer.h:47). |
| Relying on the default `VK_INDEX_TYPE_UINT16` while passing 32-bit indices | Use `vsg::BindIndexBuffer::create(indices)` so the type is computed from the data, or set `indexType` explicitly | The single-arg form derives the index type via `computeIndexType` from the data's value size (commands/BindIndexBuffer.h:24, commands/BindIndexBuffer.h:32). |
| Adding a `Node`/`StateGroup` to `Commands::children` | Only add `ref_ptr<Command>` subclasses via `addChild` | `Commands::Children` is `std::vector<ref_ptr<vsg::Command>>`; it holds commands, not arbitrary nodes (commands/Commands.h:48-49). |

## Things to never invent

- There is no `Draw::create()` overload taking a vertex array directly; `Draw` carries only the four `uint32_t` counts and a separate `BindVertexBuffers` supplies the data (commands/Draw.h:28-47).
- `Command` has no public `execute()` or `run()` method — the recording entry point is `record(CommandBuffer&)` (commands/Command.h:29).
- `Commands` is not a `Group` and does not expose a generic `Node` child list; do not assume `addChild(ref_ptr<Node>)` (commands/Commands.h:48-54).
- `BindVertexBuffers`/`BindIndexBuffer` expose `assignArrays`/`assignIndices`, not an `assign(...)` setter named differently (commands/BindVertexBuffers.h:33, commands/BindIndexBuffer.h:38).

## Source references
- include/vsg/commands/Command.h
- include/vsg/commands/Commands.h
- include/vsg/commands/Draw.h
- include/vsg/commands/DrawIndexed.h
- include/vsg/commands/BindVertexBuffers.h
- include/vsg/commands/BindIndexBuffer.h
- include/vsg/commands/Dispatch.h
- include/vsg/commands/SetViewport.h
- include/vsg/commands/SetScissor.h
- include/vsg/commands/PipelineBarrier.h
- include/vsg/state/ViewportState.h (Viewports, Scissors typedefs)
- include/vsg/state/BufferInfo.h (BufferInfoList typedef)
- include/vsg/core/Data.h (DataList typedef)
