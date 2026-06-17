# Vulkan State & Pipelines

This module maps Vulkan's pipeline and descriptor objects onto VSG scene-graph objects. An app author reaches for it whenever they need to define *how* geometry is rendered: which shaders run, how vertices are laid out, how blending/depth/culling behave, and what textures and uniform buffers the shaders see. The two things you actually attach to the scene graph are **state commands** (`BindGraphicsPipeline`, `BindComputePipeline`, `BindDescriptorSet`, `PushConstants`) — these are `StateCommand`s that go inside a `vsg::StateGroup` to apply state to a subgraph. Everything else (the pipeline, its layout, the `*State` structs, descriptors, samplers, image views) are the configuration objects those bind commands reference. VSG compiles these into real Vulkan handles lazily; you build the description, add the bind command to a `StateGroup`, and the viewer's compile traversal does the rest.

## Public includes

```cpp
#include <vsg/all.h>   // pulls in everything below
```

Or include the specific headers:

```cpp
#include <vsg/state/GraphicsPipeline.h>     // GraphicsPipeline, GraphicsPipelineState, BindGraphicsPipeline
#include <vsg/state/ComputePipeline.h>      // ComputePipeline, BindComputePipeline
#include <vsg/state/ShaderStage.h>          // ShaderStage, ShaderStages
#include <vsg/state/ShaderModule.h>         // ShaderModule, ShaderCompileSettings
#include <vsg/state/PipelineLayout.h>       // PipelineLayout
#include <vsg/state/DescriptorSetLayout.h>  // DescriptorSetLayout
#include <vsg/state/DescriptorSet.h>        // DescriptorSet
#include <vsg/state/BindDescriptorSet.h>    // BindDescriptorSet, BindDescriptorSets
#include <vsg/state/Descriptor.h>           // Descriptor (base)
#include <vsg/state/DescriptorImage.h>      // DescriptorImage
#include <vsg/state/DescriptorBuffer.h>     // DescriptorBuffer
#include <vsg/state/Sampler.h>              // Sampler
#include <vsg/state/ImageInfo.h>            // ImageInfo
#include <vsg/state/ImageView.h>            // ImageView
#include <vsg/state/VertexInputState.h>     // plus InputAssemblyState, RasterizationState,
#include <vsg/state/ColorBlendState.h>      //   ColorBlendState, DepthStencilState,
#include <vsg/state/PushConstants.h>        //   ViewportState, MultisampleState, DynamicState
#include <vsg/nodes/StateGroup.h>           // StateGroup that the Bind* commands attach to
```

## Key classes

### vsg::GraphicsPipeline
Encapsulates a graphics `VkPipeline` plus the `VkGraphicsPipelineCreateInfo` used to build it (state/GraphicsPipeline.h:55-56).
- Construct with `GraphicsPipeline::create(layout, shaderStages, pipelineStates, subpass=0)` (state/GraphicsPipeline.h:61).
- `ref_ptr<PipelineLayout> layout` — the pipeline layout (descriptor set layouts + push-constant ranges) (state/GraphicsPipeline.h:72).
- `ShaderStages stages` — the shader stages (vertex, fragment, ...) (state/GraphicsPipeline.h:70).
- `GraphicsPipelineStates pipelineStates` — the fixed-function `*State` objects that configure the pipeline (state/GraphicsPipeline.h:71).
- `uint32_t subpass` — subpass index this pipeline targets (state/GraphicsPipeline.h:73).
- You do not bind this directly; wrap it in a `BindGraphicsPipeline` (state/GraphicsPipeline.h:109-110).

### vsg::BindGraphicsPipeline
State command wrapping `vkCmdBindPipeline` for a `GraphicsPipeline`; this is the object you add to a `StateGroup` (state/GraphicsPipeline.h:109-110).
- Construct with `BindGraphicsPipeline::create(graphicsPipeline)` (state/GraphicsPipeline.h:113).
- `ref_ptr<GraphicsPipeline> pipeline` — the pipeline to bind (state/GraphicsPipeline.h:116).

### vsg::GraphicsPipelineState
Abstract base for the fixed-function `*State` objects; subclasses listed are ColorBlendState, DepthStencilState, DynamicState, InputAssemblyState, MultisampleState, RasterizationState, TessellationState, VertexInputState and ViewportState (state/GraphicsPipeline.h:25-28). Each contributes to `VkGraphicsPipelineCreateInfo` via its `apply` override (state/GraphicsPipeline.h:38). You place instances of these in the `GraphicsPipelineStates` vector you pass to `GraphicsPipeline::create`.

### vsg::ComputePipeline
Encapsulates a compute `VkPipeline` and its `VkComputePipelineCreateInfo` (state/ComputePipeline.h:22-23).
- Construct with `ComputePipeline::create(pipelineLayout, shaderStage)` (state/ComputePipeline.h:27).
- `ref_ptr<PipelineLayout> layout` and `ref_ptr<ShaderStage> stage` — the layout and single compute shader stage (state/ComputePipeline.h:35-36).
- Bind via `BindComputePipeline::create(computePipeline)` (state/ComputePipeline.h:67).

### vsg::ShaderStage
Encapsulates `VkPipelineShaderStageCreateInfo` — one shader stage of a pipeline (state/ShaderStage.h:22-23).
- Construct from a module: `ShaderStage::create(stage, entryPointName, shaderModule)` (state/ShaderStage.h:28).
- Construct from GLSL source: `ShaderStage::create(stage, entryPointName, source, hints={})` (state/ShaderStage.h:29).
- Construct from SPIR-V: `ShaderStage::create(stage, entryPointName, spirv)` (state/ShaderStage.h:30).
- Load from file: `ShaderStage::read(stage, entryPointName, filename, options={})` (state/ShaderStage.h:45).
- `VkShaderStageFlagBits stage`, `std::string entryPointName`, `ref_ptr<ShaderModule> module` (state/ShaderStage.h:40-42).
- `SpecializationConstants specializationConstants` — map of `uint32_t` -> `ref_ptr<Data>` for specialization constants (state/ShaderStage.h:33,43).
- `ShaderStages` is `std::vector<ref_ptr<ShaderStage>>` (state/ShaderStage.h:65).

### vsg::ShaderModule
Encapsulates `VkShaderModule`; holds GLSL source and/or SPIR-V code, and is assigned to a ShaderStage (state/ShaderModule.h:71-73).
- Construct from source: `ShaderModule::create(source, hints={})` (state/ShaderModule.h:80).
- Construct from SPIR-V: `ShaderModule::create(code)` where `code` is `SPIRV` (`std::vector<uint32_t>`) (state/ShaderModule.h:76,81).
- `std::string source`, `ref_ptr<ShaderCompileSettings> hints`, `SPIRV code` (state/ShaderModule.h:84-88).

### vsg::ShaderCompileSettings
Settings passed to glslang when compiling GLSL/HLSL to SPIR-V (state/ShaderModule.h:25-27).
- `Language language` (GLSL/HLSL), `SpirvTarget target`, `int defaultVersion = 450`, `std::set<std::string> defines` (state/ShaderModule.h:51-58). Pass a configured instance as the `hints` argument to a `ShaderStage`/`ShaderModule`.

### vsg::PipelineLayout
Encapsulates `VkPipelineLayout` — the descriptor set layouts and push-constant ranges a pipeline uses (state/PipelineLayout.h:25-26).
- Construct with `PipelineLayout::create(setLayouts, pushConstantRanges, flags=0)` (state/PipelineLayout.h:31).
- `DescriptorSetLayouts setLayouts` — vector of descriptor set layouts (state/PipelineLayout.h:35).
- `PushConstantRanges pushConstantRanges` — `std::vector<VkPushConstantRange>` (state/PipelineLayout.h:23,36).

### vsg::DescriptorSetLayout
Encapsulates `VkDescriptorSetLayout` describing the bindings (types/counts/stages) a descriptor set exposes (state/DescriptorSetLayout.h:27-28).
- Construct with `DescriptorSetLayout::create(bindings)` where `bindings` is `DescriptorSetLayoutBindings` (`std::vector<VkDescriptorSetLayoutBinding>`) (state/DescriptorSetLayout.h:23,33).
- `DescriptorSetLayoutBindings bindings` field (state/DescriptorSetLayout.h:42).
- `DescriptorSetLayouts` is the vector type used by PipelineLayout (state/DescriptorSetLayout.h:83).

### vsg::DescriptorSet
Encapsulates `VkDescriptorSet` — binds a set layout to the actual descriptors (textures/buffers) (state/DescriptorSet.h:24-25).
- Construct with `DescriptorSet::create(descriptorSetLayout, descriptors)` (state/DescriptorSet.h:30).
- `ref_ptr<DescriptorSetLayout> setLayout`, `Descriptors descriptors` (state/DescriptorSet.h:33-34).

### vsg::BindDescriptorSet
State command wrapping `vkCmdBindDescriptorSets` for a single descriptor set; add to a `StateGroup` (state/BindDescriptorSet.h:89-91).
- `BindDescriptorSet::create(bindPoint, pipelineLayout, descriptorSet)` (state/BindDescriptorSet.h:106).
- `BindDescriptorSet::create(bindPoint, pipelineLayout, firstSet, descriptorSet)` (state/BindDescriptorSet.h:97).
- Convenience overload that builds the DescriptorSet for you from descriptors: `BindDescriptorSet::create(bindPoint, pipelineLayout, firstSet, descriptors)` (state/BindDescriptorSet.h:116).
- Fields: `pipelineBindPoint`, `ref_ptr<PipelineLayout> layout`, `firstSet`, `ref_ptr<DescriptorSet> descriptorSet` (state/BindDescriptorSet.h:126-129).
- `BindDescriptorSets` is the multi-set variant (state/BindDescriptorSet.h:23-24,30).

### vsg::Descriptor
Base class for descriptor types (DescriptorBuffer/DescriptorImage/...); descriptors are assigned to descriptor sets / bind commands (state/Descriptor.h:22-25).
- `uint32_t dstBinding`, `uint32_t dstArrayElement`, `VkDescriptorType descriptorType` (state/Descriptor.h:32-34).
- `Descriptors` is `std::vector<ref_ptr<Descriptor>>` (state/Descriptor.h:49).

### vsg::DescriptorImage
Descriptor that passes textures (combined image samplers) to shaders (state/DescriptorImage.h:21-23).
- Most common: `DescriptorImage::create(sampler, imageData, dstBinding=0, dstArrayElement=0, descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)` (state/DescriptorImage.h:29).
- From a prebuilt `ImageInfo`: `DescriptorImage::create(imageInfo, dstBinding=0, ...)` (state/DescriptorImage.h:35).
- `ImageInfoList imageInfoList` field (state/DescriptorImage.h:39).

### vsg::DescriptorBuffer
Descriptor that passes uniform or storage buffers to shaders (state/DescriptorBuffer.h:21-23).
- `DescriptorBuffer::create(data, dstBinding=0, dstArrayElement=0, descriptorType=VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)` (state/DescriptorBuffer.h:29).

### vsg::Sampler
Encapsulates `VkSampler` and its `VkSamplerCreateInfo` (state/Sampler.h:23-24).
- Construct with `Sampler::create()` then set fields (all have sensible defaults) (state/Sampler.h:27).
- Fields: `magFilter`/`minFilter` (default `VK_FILTER_LINEAR`), `mipmapMode`, `addressModeU/V/W` (default `VK_SAMPLER_ADDRESS_MODE_REPEAT`), `anisotropyEnable`, `maxAnisotropy`, `minLod`, `maxLod`, `borderColor` (state/Sampler.h:31-44).

### vsg::ImageView
Encapsulates `VkImageView` over an `Image` (state/ImageView.h:23-24).
- `ImageView::create(image)` or `ImageView::create(image, aspectFlags)` (state/ImageView.h:27-28).
- Fields: `ref_ptr<Image> image`, `VkImageViewType viewType` (default `VK_IMAGE_VIEW_TYPE_2D`), `VkFormat format`, `VkImageSubresourceRange subresourceRange` (state/ImageView.h:32-36).
- `createImageView(context, image, aspectFlags)` convenience allocates device memory too (state/ImageView.h:65).

### vsg::VertexInputState
Configures `VkPipelineVertexInputStateCreateInfo` — vertex buffer bindings and attributes (state/VertexInputState.h:20-21).
- `VertexInputState::create(bindings, attributes)` where the types are `Bindings` (`std::vector<VkVertexInputBindingDescription>`) and `Attributes` (`std::vector<VkVertexInputAttributeDescription>`) (state/VertexInputState.h:24-25,29).
- Fields `vertexBindingDescriptions`, `vertexAttributeDescriptions` (state/VertexInputState.h:32-33).

### vsg::InputAssemblyState
Configures `VkPipelineInputAssemblyStateCreateInfo` — primitive topology (state/InputAssemblyState.h:20-21).
- `InputAssemblyState::create()` (default `VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST`) or `InputAssemblyState::create(topology, primitiveRestart=VK_FALSE)` (state/InputAssemblyState.h:26,29).
- Fields `topology`, `primitiveRestartEnable` (state/InputAssemblyState.h:29-30).

### vsg::RasterizationState
Configures `VkPipelineRasterizationStateCreateInfo` — polygon fill, culling, line width (state/RasterizationState.h:20-21).
- `RasterizationState::create()` then set fields (state/RasterizationState.h:24).
- Fields: `polygonMode` (default `VK_POLYGON_MODE_FILL`), `cullMode` (default `VK_CULL_MODE_BACK_BIT`), `frontFace` (default `VK_FRONT_FACE_COUNTER_CLOCKWISE`), `lineWidth` (default 1.0) (state/RasterizationState.h:30-37).

### vsg::ColorBlendState
Configures `VkPipelineColorBlendStateCreateInfo` — per-attachment blending (state/ColorBlendState.h:20-21).
- `ColorBlendState::create()` or `ColorBlendState::create(colorBlendAttachments)` (state/ColorBlendState.h:26,28).
- `configureAttachments(bool blendEnable)` sets up standard src-alpha/one-minus-src-alpha blending when true, else disables it (state/ColorBlendState.h:36-37).
- Fields `logicOpEnable`, `attachments`, `blendConstants[4]` (state/ColorBlendState.h:31-34).

### vsg::DepthStencilState
Configures `VkPipelineDepthStencilStateCreateInfo` — depth/stencil testing (state/DepthStencilState.h:20-21).
- `DepthStencilState::create()` (state/DepthStencilState.h:24).
- Fields: `depthTestEnable` (default `VK_TRUE`), `depthWriteEnable` (default `VK_TRUE`), `depthCompareOp` (default `VK_COMPARE_OP_GREATER`), `stencilTestEnable` (state/DepthStencilState.h:28-32). Note VSG defaults to a reverse-Z compare op.

### vsg::ViewportState
Configures `VkPipelineViewportStateCreateInfo` — viewport(s) and scissor(s) (state/ViewportState.h:22-23).
- `ViewportState::create(extent)` makes a single viewport+scissor covering a window of that `VkExtent2D` (state/ViewportState.h:30).
- `ViewportState::create(x, y, width, height)` for an explicit rectangle (state/ViewportState.h:33).
- `set(x, y, width, height)`, `getViewport()`, `getScissor()` helpers (state/ViewportState.h:40-46).
- Fields `Viewports viewports`, `Scissors scissors` (state/ViewportState.h:36-37).

### vsg::PushConstants
State command wrapping `vkCmdPushConstants`; add to a `StateGroup` to push small per-draw data such as a transform matrix (state/PushConstants.h:22-23).
- `PushConstants::create(shaderStageFlags, offset, data)` (state/PushConstants.h:27).
- Fields `VkShaderStageFlags stageFlags`, `uint32_t offset`, `ref_ptr<Data> data` (state/PushConstants.h:29-31).

## Idioms

Build a graphics pipeline from shaders and fixed-function state, then bind it inside a StateGroup:

```cpp
auto vert = vsg::ShaderStage::read(VK_SHADER_STAGE_VERTEX_BIT, "main", "shader.vert.spv");
auto frag = vsg::ShaderStage::read(VK_SHADER_STAGE_FRAGMENT_BIT, "main", "shader.frag.spv");

// descriptor set layout: binding 0 = combined image sampler for the fragment shader
auto dsl = vsg::DescriptorSetLayout::create(vsg::DescriptorSetLayoutBindings{
    {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}});

// push-constant range for a 64-byte mat4 in the vertex shader
auto layout = vsg::PipelineLayout::create(
    vsg::DescriptorSetLayouts{dsl},
    vsg::PushConstantRanges{{VK_SHADER_STAGE_VERTEX_BIT, 0, 64}});

vsg::VertexInputState::Bindings bindings{
    {0, sizeof(float) * 3, VK_VERTEX_INPUT_RATE_VERTEX}};
vsg::VertexInputState::Attributes attributes{
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0}};

vsg::GraphicsPipelineStates states{
    vsg::VertexInputState::create(bindings, attributes),
    vsg::InputAssemblyState::create(),
    vsg::RasterizationState::create(),
    vsg::MultisampleState::create(),
    vsg::ColorBlendState::create(),
    vsg::DepthStencilState::create()};

auto pipeline = vsg::GraphicsPipeline::create(layout, vsg::ShaderStages{vert, frag}, states);

auto stateGroup = vsg::StateGroup::create();
stateGroup->add(vsg::BindGraphicsPipeline::create(pipeline));
// ... add draw commands as children of stateGroup
```

Bind a texture as a descriptor set, plus a push-constant transform:

```cpp
auto sampler = vsg::Sampler::create();             // linear filter, repeat wrap
auto texture = vsg::DescriptorImage::create(sampler, imageData, 0, 0,
                                            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

auto descriptorSet = vsg::DescriptorSet::create(dsl, vsg::Descriptors{texture});
stateGroup->add(vsg::BindDescriptorSet::create(
    VK_PIPELINE_BIND_POINT_GRAPHICS, layout, descriptorSet));

stateGroup->add(vsg::PushConstants::create(
    VK_SHADER_STAGE_VERTEX_BIT, 0, vsg::mat4Value::create(modelMatrix)));
```

A compute pipeline:

```cpp
auto computeStage = vsg::ShaderStage::read(VK_SHADER_STAGE_COMPUTE_BIT, "main", "comp.spv");
auto computeLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{dsl}, vsg::PushConstantRanges{});
auto computePipeline = vsg::ComputePipeline::create(computeLayout, computeStage);
auto bindCompute = vsg::BindComputePipeline::create(computePipeline);
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `new vsg::GraphicsPipeline(...)` | `vsg::GraphicsPipeline::create(layout, stages, states)` (state/GraphicsPipeline.h:61) | All VSG objects are heap-managed via `create()` returning `ref_ptr`; raw `new` breaks reference counting. |
| Adding a `GraphicsPipeline` directly to a `StateGroup` | Add a `BindGraphicsPipeline::create(pipeline)` (state/GraphicsPipeline.h:113) | The pipeline is config; only the `BindGraphicsPipeline` `StateCommand` records the `vkCmdBindPipeline` call. |
| Assuming depth test uses `VK_COMPARE_OP_LESS` | VSG's `DepthStencilState::depthCompareOp` defaults to `VK_COMPARE_OP_GREATER` (state/DepthStencilState.h:30) | VSG uses reverse-Z by default; override the field if your depth buffer is conventional. |
| Passing a `DescriptorSetLayout` whose bindings disagree with the shader | Match `DescriptorSetLayout::create(bindings)` binding numbers/types/stage flags to the shader and the `PipelineLayout::setLayouts` order (state/DescriptorSetLayout.h:33, state/PipelineLayout.h:35) | The `firstSet`/binding indices must line up across layout, set, and shader or validation fails. |
| Setting blend fields by hand for standard alpha blending | Call `ColorBlendState::configureAttachments(true)` (state/ColorBlendState.h:37) | The helper fills the per-attachment state correctly; `attachments` is empty by default. |

## Things to never invent

- `GraphicsPipeline` has no `BindPipeline()`/`bind()` method — binding is done by the separate `BindGraphicsPipeline` command (state/GraphicsPipeline.h:109-113).
- There is no `setShaders()` / `addState()` setter on `GraphicsPipeline`; the shader stages and states are constructor arguments / public vector fields `stages` and `pipelineStates` (state/GraphicsPipeline.h:61,70-71).
- `Sampler` has no `create(magFilter, ...)` parameterized factory in this header — only `Sampler::create()` then field assignment (state/Sampler.h:27).
- `DescriptorSetLayout` exposes `bindings` (a vector) and `addBinding(...)`; there is no `addDescriptor()` method (state/DescriptorSetLayout.h:42,47).
- `PushConstants` does not store a `VkPushConstantRange`; the range lives on `PipelineLayout::pushConstantRanges` while `PushConstants` carries `stageFlags`, `offset`, `data` (state/PipelineLayout.h:36, state/PushConstants.h:29-31).

## Source references

- include/vsg/state/GraphicsPipeline.h
- include/vsg/state/ComputePipeline.h
- include/vsg/state/ShaderStage.h
- include/vsg/state/ShaderModule.h
- include/vsg/state/PipelineLayout.h
- include/vsg/state/DescriptorSetLayout.h
- include/vsg/state/DescriptorSet.h
- include/vsg/state/BindDescriptorSet.h
- include/vsg/state/Descriptor.h
- include/vsg/state/DescriptorImage.h
- include/vsg/state/DescriptorBuffer.h
- include/vsg/state/Sampler.h
- include/vsg/state/ImageInfo.h
- include/vsg/state/ImageView.h
- include/vsg/state/VertexInputState.h
- include/vsg/state/InputAssemblyState.h
- include/vsg/state/RasterizationState.h
- include/vsg/state/ColorBlendState.h
- include/vsg/state/DepthStencilState.h
- include/vsg/state/ViewportState.h
- include/vsg/state/MultisampleState.h
- include/vsg/state/DynamicState.h
- include/vsg/state/PushConstants.h
- include/vsg/state/StateCommand.h
- include/vsg/nodes/StateGroup.h
