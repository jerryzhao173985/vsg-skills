# Utils

The `vsg/utils` module bundles the high-level helpers an application author uses to *build* and *operate* a scene graph without hand-writing every Vulkan state object. Reach for it when you need: ready-to-render primitive subgraphs (`Builder`), pipeline/descriptor setup driven by shader introspection (`GraphicsPipelineConfigurator` + `ShaderSet`), command-line parsing (`CommandLine`), bounds computation for framing a camera (`ComputeBounds`), mouse picking / ray intersection (`LineSegmentIntersector`), deduplication of shared GPU state (`SharedObjects`), and runtime GLSL-to-SPIR-V compilation (`ShaderCompiler`).

## Public includes

```cpp
#include <vsg/all.h>                                   // pulls in everything below

// or target the specific headers:
#include <vsg/utils/Builder.h>                         // Builder, GeometryInfo, StateInfo
#include <vsg/utils/GraphicsPipelineConfigurator.h>    // GraphicsPipelineConfigurator, DescriptorConfigurator
#include <vsg/utils/ShaderSet.h>                        // ShaderSet, createPhongShaderSet, ...
#include <vsg/utils/ComputeBounds.h>                    // ComputeBounds
#include <vsg/utils/CommandLine.h>                      // CommandLine
#include <vsg/utils/Intersector.h>                      // Intersector (base)
#include <vsg/utils/LineSegmentIntersector.h>           // LineSegmentIntersector
#include <vsg/utils/SharedObjects.h>                    // SharedObjects
#include <vsg/utils/ShaderCompiler.h>                   // ShaderCompiler
```

## Key classes

### vsg::Builder

Creates subgraphs that render primitive geometries: Box, Capsule, Cone, Cylinder, Disk, Quad, Sphere and HeightField, guided by a `GeometryInfo` and `StateInfo` (utils/Builder.h:106-108).

- Construct via `Builder::create()` (it derives from `Inherit<Object, Builder>`, utils/Builder.h:109).
- Shape factories each return a `ref_ptr<Node>`: `createBox`, `createCapsule`, `createCone`, `createCylinder`, `createDisk`, `createQuad`, `createSphere`, `createHeightField`, all taking `const GeometryInfo&` and `const StateInfo&` defaulted to `{}` (utils/Builder.h:123-130).
- `createStateGroup(const StateInfo&)` returns just the `ref_ptr<StateGroup>` carrying the render state (utils/Builder.h:132).
- Public fields you set before building: `ref_ptr<Options> options`, `ref_ptr<SharedObjects> sharedObjects`, `ref_ptr<ShaderSet> shaderSet`, and `bool verbose` (utils/Builder.h:118-121).
- `assignCompileTraversal(ref_ptr<CompileTraversal> ct)` enables on-the-fly GPU compilation of built subgraphs (utils/Builder.h:135).

### vsg::GeometryInfo (struct)

Supplies geometry placement/size/color to `Builder` (utils/Builder.h:52-53).

- `vec3 position`, basis vectors `dx`/`dy`/`dz`, `vec4 color`, and `mat4 transform` (utils/Builder.h:63-68).
- `set(const t_box<T>&)` / `set(const t_sphere<T>&)` derive position and extents from a box or sphere; matching constructors exist (utils/Builder.h:58-89).
- `ref_ptr<Data> positions` for instancing (vec3Array) or billboards (vec4Array), and `ref_ptr<Data> colors` for per-instance colors (utils/Builder.h:91-93).
- `bool cullNode` decorates the result with a CullNode when true (utils/Builder.h:70-71).

### vsg::StateInfo (struct)

Supplies render-state toggles to `Builder` (utils/Builder.h:24-25).

- Flags: `lighting`, `two_sided`, `blending`, `greyscale`, `wireframe`, `instance_colors_vec4`, `instance_positions_vec3`, `billboard` (utils/Builder.h:27-34).
- `ref_ptr<Data> image` (base color texture) and `ref_ptr<Data> displacementMap` (utils/Builder.h:36-37).

### vsg::GraphicsPipelineConfigurator

Sets up pipeline state, arrays and descriptors using a `ShaderSet` as the guide for required layouts/bindings, so you don't enumerate every Vulkan state object by hand (utils/GraphicsPipelineConfigurator.h:96-97).

- Construct with a shader set: `GraphicsPipelineConfigurator::create(shaderSet)` (utils/GraphicsPipelineConfigurator.h:100).
- `enableArray(name, vertexInputRate, stride, format)` and `enableDescriptor(name)` / `enableTexture(name)` mark inputs as used (utils/GraphicsPipelineConfigurator.h:114-116).
- `assignArray(arrays, name, vertexInputRate, array)` adds a vertex array; `assignDescriptor(name, data, ...)` and `assignTexture(name, textureData, sampler, ...)` bind data by shader-defined name (utils/GraphicsPipelineConfigurator.h:118-122).
- `init()` builds the state objects; afterward read `layout`, `graphicsPipeline`, and `bindGraphicsPipeline` (utils/GraphicsPipelineConfigurator.h:139, 151-154).
- `copyTo(ref_ptr<StateGroup>, sharedObjects)` (or the `StateCommands` overload) attaches the configured pipeline state to your scene; returns true if state was added (utils/GraphicsPipelineConfigurator.h:145-148).
- Inputs/fields: `GraphicsPipelineStates pipelineStates`, `uint32_t subpass`, `ref_ptr<ShaderSet> shaderSet`, and the `descriptorConfigurator` produced by assign calls (utils/GraphicsPipelineConfigurator.h:106-110, 133).

### vsg::DescriptorConfigurator

Sets up descriptor sets using a `ShaderSet` as a guide (utils/GraphicsPipelineConfigurator.h:35-36); usually created for you by `GraphicsPipelineConfigurator`, but usable directly via `DescriptorConfigurator::create(shaderSet)` (utils/GraphicsPipelineConfigurator.h:39).

- `assignTexture(name, textureData, sampler, dstArrayElement)` and `assignDescriptor(name, data, dstArrayElement)` bind by name (utils/GraphicsPipelineConfigurator.h:50, 54).
- `assignDefaults(inheritedSets)` adds descriptors enabled by default after explicit assignment (utils/GraphicsPipelineConfigurator.h:66).
- Result: `std::vector<ref_ptr<DescriptorSet>> descriptorSets` (utils/GraphicsPipelineConfigurator.h:70).

### vsg::ShaderSet

A collection of shader-related settings providing a form of shader introspection (attribute/descriptor/push-constant bindings) used by the configurators (utils/ShaderSet.h:113-114).

- Construct with `ShaderSet::create(stages, hints)` or use the free factories `createFlatShadedShaderSet`, `createPhongShaderSet`, `createPhysicsBasedRenderingShaderSet`, each returning `ref_ptr<ShaderSet>` (utils/ShaderSet.h:118, 207-213).
- Declarative binding setup: `addAttributeBinding(name, define, location, format, data, ...)`, `addDescriptorBinding(name, define, set, binding, descriptorType, descriptorCount, stageFlags, data, ...)`, `addPushConstantRange(name, define, stageFlags, offset, size)` (utils/ShaderSet.h:139, 142, 147).
- Introspection getters: `getAttributeBinding(name)`, `getDescriptorBinding(name)` (utils/ShaderSet.h:150, 156).
- `getShaderStages(scs)` returns the compiled variant for the given `ShaderCompileSettings` (utils/ShaderSet.h:169).
- `createPipelineLayout(defines)` / `createDescriptorSetLayout(defines, set)` build Vulkan layout objects (utils/ShaderSet.h:178, 184).
- Public fields include `ShaderStages stages`, `defaultGraphicsPipelineStates`, and `defaultShaderHints` (utils/ShaderSet.h:121, 128, 131).

### vsg::ComputeBounds

A `ConstVisitor` that traverses a scene graph and computes an overall bounding box enclosing all geometry — used to frame a camera on a loaded model (utils/ComputeBounds.h:21-22).

- Construct via `ComputeBounds::create()` (utils/ComputeBounds.h:22, 25).
- Result is in the public `dbox bounds` field after the scene `accept`s the visitor (utils/ComputeBounds.h:28).
- `bool useNodeBounds` (default true) uses Cull/LOD node bounds for speed instead of descending into their subgraphs (utils/ComputeBounds.h:30-32).

### vsg::CommandLine

Parses `argc`/`argv`, consuming matched arguments as it goes (utils/CommandLine.h:41-43).

- Construct with pointers to your main args: `vsg::CommandLine arguments(&argc, argv)` (utils/CommandLine.h:46). Note: this is a plain class, not a `vsg::Object`, so there is no `create()`.
- `operator[](int i)` returns the i-th raw argument as a `char*` — e.g. `arguments[1]` for the first positional argument, usable directly as a file path to `read_cast` (utils/CommandLine.h:54-55).
- `read(match, args...)` returns true and removes the matched option and its values when `match` is present; supports multiple typed values, including vec/mat element counts (utils/CommandLine.h:124-152).
- `read({alias1, alias2}, args...)` matches any of several option spellings (utils/CommandLine.h:154-160).
- `value(defaultValue, match, args...)` returns the parsed value or the supplied default (utils/CommandLine.h:162-168).
- `errors()`, `getErrorMessages()`, and `writeErrorMessages(out)` report parse failures (utils/CommandLine.h:205-215).

### vsg::Intersector

Base `ConstVisitor` for intersecting the scene graph; `LineSegmentIntersector` is the common subclass (utils/Intersector.h:21-23).

- Subclasses implement `pushTransform`/`popTransform`, `intersects(dsphere)`, `intersectDraw(...)`, `intersectDrawIndexed(...)` (utils/Intersector.h:64-75).
- `localToWorldStack()` / `worldToLocalStack()` expose the current transform stacks (utils/Intersector.h:78-81).

### vsg::LineSegmentIntersector

An `Intersector` subclass that computes intersections between a line segment and scene geometry — the standard tool for mouse picking (utils/LineSegmentIntersector.h:30-31).

- Construct from world-space endpoints: `LineSegmentIntersector::create(start, end)` (utils/LineSegmentIntersector.h:34), or from a camera and window pixel coords for picking: `LineSegmentIntersector::create(camera, x, y)` (utils/LineSegmentIntersector.h:35).
- Have the scene `accept(*intersector)`, then read the public `Intersections intersections` vector (utils/LineSegmentIntersector.h:57-58).
- Each `Intersection` (a `ref_ptr` element) exposes `localIntersection`, `worldIntersection`, `ratio`, `localToWorld`, `nodePath`, `arrays`, `indexRatios`, and `instanceIndex` (utils/LineSegmentIntersector.h:43-51); `operator bool` is true when its `nodePath` is non-empty (utils/LineSegmentIntersector.h:54).

### vsg::SharedObjects

Facilitates sharing of object instances that have the same properties, so identical pipelines/state/samplers are deduplicated rather than duplicated on the GPU (utils/SharedObjects.h:30-31).

- Construct via `SharedObjects::create()` (utils/SharedObjects.h:31, 34).
- `share(ref_ptr<T>& object)` replaces `object` with an existing equivalent if one is held, otherwise records it (utils/SharedObjects.h:39-40, 156-171).
- `share(ref_ptr<T>& object, Func init)` runs `init(object)` to populate the object only when it is new (utils/SharedObjects.h:42-43, 174-200).
- `shared_default<T>()` returns a shared default-constructed instance of `T` (utils/SharedObjects.h:36-37).
- `clear()` empties the cache; `prune()` drops singly-referenced objects; `report(output)` writes held-object stats (utils/SharedObjects.h:67-73).

### vsg::ShaderCompiler

Integrates with glslang to compile GLSL shaders to the SPIR-V that Vulkan consumes (utils/ShaderCompiler.h:11-15).

- Construct via `ShaderCompiler::create()` (utils/ShaderCompiler.h:15, 18).
- `supported()` returns whether this VSG build has compilation support (mirrors the `VSG_SUPPORTS_ShaderCompiler` define) (utils/ShaderCompiler.h:21-23).
- `compile(ShaderStages&, defines, options)` or `compile(ref_ptr<ShaderStage>, defines, options)` perform the compilation (utils/ShaderCompiler.h:28-29).
- `ref_ptr<ShaderCompileSettings> defaults` holds default compile settings (utils/ShaderCompiler.h:26).

## Idioms

```cpp
// 1. Build a textured, lit box and add it to the scene.
auto builder = vsg::Builder::create();
vsg::GeometryInfo geom;
geom.position = {0.0f, 0.0f, 0.0f};
geom.color = {1.0f, 1.0f, 1.0f, 1.0f};
vsg::StateInfo state;
state.lighting = true;
auto scene = vsg::Group::create();
scene->addChild(builder->createBox(geom, state));
```

```cpp
// 2. Parse the command line; consumed args are removed from argv.
vsg::CommandLine arguments(&argc, argv);
auto numFrames = arguments.value<int>(-1, "--frames");
bool fullscreen = arguments.read({"--fullscreen", "-f"});
if (arguments.errors()) return arguments.writeErrorMessages(std::cerr);
```

```cpp
// 3. Frame a camera on a loaded model by computing its bounds.
auto computeBounds = vsg::ComputeBounds::create();
scene->accept(*computeBounds);
vsg::dvec3 centre = (computeBounds->bounds.min + computeBounds->bounds.max) * 0.5;
double radius = vsg::length(computeBounds->bounds.max - computeBounds->bounds.min) * 0.5;
```

```cpp
// 4. Pick into the scene from a window pixel using a camera.
auto intersector = vsg::LineSegmentIntersector::create(*camera, x, y);
scene->accept(*intersector);
for (auto& hit : intersector->intersections)
{
    if (*hit) { /* use hit->worldIntersection, hit->nodePath */ }
}
```

## Common mistakes

| Bad | Good | Why |
|-----|------|-----|
| `auto b = new vsg::Builder;` | `auto b = vsg::Builder::create();` (utils/Builder.h:109) | Every `vsg::Object` is created through the `Inherit` `create()` factory returning a `ref_ptr`; raw `new` breaks reference counting. |
| Reading `configurator->graphicsPipeline` before calling `init()` | Call `init()` first, then read `graphicsPipeline`/`bindGraphicsPipeline` (utils/GraphicsPipelineConfigurator.h:139, 153-154) | These members are populated by `init()` (commented "setup by init()"), so they are empty beforehand. |
| `arguments.value<int>(0, "--frames");` then re-`read` the same flag | Read each flag once; `read`/`value` *remove* the matched args from argv (utils/CommandLine.h:139, 162-168) | After a successful match the option is consumed, so a second read returns the default. |
| Hand-instancing every duplicate `Sampler`/pipeline | `sharedObjects->share(object)` (utils/SharedObjects.h:156-171) | Sharing deduplicates identical GPU state instead of allocating duplicates. |
| Assuming `ShaderCompiler::compile` always works | Guard with `if (shaderCompiler->supported())` (utils/ShaderCompiler.h:21-23) | Compilation requires a glslang-enabled build; otherwise it is unsupported. |

## Things to never invent

- `vsg::Builder::createPyramid` / `createTorus` / `createPlane` — the only shape factories are box, capsule, cone, cylinder, disk, quad, sphere, height field (utils/Builder.h:123-130).
- `CommandLine::create()` — `CommandLine` is not a `vsg::Object`; construct it directly with `(&argc, argv)` (utils/CommandLine.h:43-46).
- `ComputeBounds::getBounds()` — the result is the public `bounds` field, there is no getter (utils/ComputeBounds.h:28).
- `LineSegmentIntersector::getIntersections()` — read the public `intersections` vector directly (utils/LineSegmentIntersector.h:57-58).
- `ShaderSet::addUniformBinding` as the current API — it exists only as a deprecated alias for `addDescriptorBinding` (utils/ShaderSet.h:144).

## Source references

- include/vsg/utils/Builder.h
- include/vsg/utils/GraphicsPipelineConfigurator.h
- include/vsg/utils/ShaderSet.h
- include/vsg/utils/ComputeBounds.h
- include/vsg/utils/CommandLine.h
- include/vsg/utils/Intersector.h
- include/vsg/utils/LineSegmentIntersector.h
- include/vsg/utils/SharedObjects.h
- include/vsg/utils/ShaderCompiler.h
