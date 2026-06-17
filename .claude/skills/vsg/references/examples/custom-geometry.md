---
title: custom_geometry
description: The hand-built rendering path — own vertex arrays + ShaderSet + GraphicsPipelineConfigurator + a material descriptor, without vsg::Builder.
---

# Custom geometry & pipeline — `custom_geometry`

When `vsg::Builder`'s primitives aren't enough (your own meshes, your own
material/shaders), this is the path: build a `ShaderSet`, drive a
`GraphicsPipelineConfigurator` to produce the pipeline state, attach it to a
`StateGroup`, and draw with a `VertexIndexDraw` leaf. Source of truth (compile-verified
twin): `examples/custom_geometry.cpp` — the excerpt below is the load-bearing portion;
the viewer/camera/loop around it is identical to [hello_vsg](./hello-vsg.md).

```cpp
auto shaderSet = vsg::createPhongShaderSet();                        // utils/ShaderSet.h:210
auto gpConf = vsg::GraphicsPipelineConfigurator::create(shaderSet);  // utils/GraphicsPipelineConfigurator.h:100

auto vertices = vsg::vec3Array::create({
    {-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}});
auto normals = vsg::vec3Array::create(4, vsg::vec3(0.0f, 0.0f, 1.0f));
auto texcoords = vsg::vec2Array::create({{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}});
auto colors = vsg::vec4Value::create(vsg::vec4(1.0f, 1.0f, 1.0f, 1.0f));   // per-instance
auto indices = vsg::ushortArray::create({0, 1, 2, 2, 3, 0});

auto material = vsg::PhongMaterialValue::create();                  // state/material.h:90
material->value().diffuse = vsg::vec4(0.8f, 0.2f, 0.2f, 1.0f);
gpConf->assignDescriptor("material", material);                     // utils/GraphicsPipelineConfigurator.h:119

vsg::DataList vertexArrays;
gpConf->assignArray(vertexArrays, "vsg_Vertex",    VK_VERTEX_INPUT_RATE_VERTEX,   vertices);   // .h:118
gpConf->assignArray(vertexArrays, "vsg_Normal",    VK_VERTEX_INPUT_RATE_VERTEX,   normals);
gpConf->assignArray(vertexArrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX,   texcoords);
gpConf->assignArray(vertexArrays, "vsg_Color",     VK_VERTEX_INPUT_RATE_INSTANCE, colors);

gpConf->init();                                                     // .h:139 — after all assigns

auto stateGroup = vsg::StateGroup::create();                        // nodes/StateGroup.h:31
gpConf->copyTo(stateGroup);                                         // .h:148 (returns bool)

auto vid = vsg::VertexIndexDraw::create();                          // nodes/VertexIndexDraw.h:24
vid->assignArrays(vertexArrays);                                    // .h:42 — pass the SAME DataList
vid->assignIndices(indices);                                        // .h:43
vid->indexCount = static_cast<uint32_t>(indices->size());
vid->instanceCount = 1;                                             // MUST be >= 1, defaults to 0
stateGroup->addChild(vid);
scene->addChild(stateGroup);
```

## What to copy

- **The order is load-bearing**: every `assignArray`/`assignDescriptor`/`assignTexture` must come **before** `init()` — they toggle shader defines that `init()` bakes into the pipeline. Assigning after `init()` is silently ignored.
- **Standard array names** are fixed by the shader set: `vsg_Vertex`, `vsg_Normal`, `vsg_TexCoord0`, `vsg_Color` (per-instance as a `vec4Value`). A typo'd name makes `assignArray` return `false` and silently skip the attribute.
- **`copyTo` returns `bool`** and also sets `stateGroup->prototypeArrayState`; read `gpConf->graphicsPipeline`/`->layout` only *after* `init()`.
- **`VertexIndexDraw::instanceCount` defaults to 0** — set it to ≥1 or nothing draws. Pass the *same* `DataList` to `assignArrays` that you fed to `assignArray`.
- For a **texture** instead of/with the material: `gpConf->assignTexture("diffuseMap", imageData)` (utils/GraphicsPipelineConfigurator.h:121); a default `Sampler` is created if none is passed.

## Key APIs

- `createPhongShaderSet` / `createFlatShadedShaderSet` / `createPhysicsBasedRenderingShaderSet` — `references/components/utils.md` (utils/ShaderSet.h:210, 207, 213).
- `GraphicsPipelineConfigurator` (`assignArray`/`assignDescriptor`/`assignTexture`/`init`/`copyTo`) — `references/components/utils.md` (utils/GraphicsPipelineConfigurator.h:100, 118-121, 139, 148).
- `StateGroup`, `VertexIndexDraw` (`assignArrays`/`assignIndices`/`indexCount`/`instanceCount`) — `references/components/scene-graph.md` (nodes/StateGroup.h:31, nodes/VertexIndexDraw.h:24, 42-43, 32-33).
- `PhongMaterialValue` — `references/components/state.md` (state/material.h:90).
- The surrounding viewer/camera/loop — [hello_vsg](./hello-vsg.md) and `references/foundations/rendering-model.md`.
