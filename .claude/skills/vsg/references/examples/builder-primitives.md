---
title: builder_primitives
description: Compose a scene of Builder primitives positioned with MatrixTransforms and lit by ambient + directional lights.
---

# Builder primitives + lights — `builder_primitives`

Builds a row of `vsg::Builder` primitives (box, sphere, capsule), each positioned with a
`vsg::MatrixTransform`, lit by an `AmbientLight` + a `DirectionalLight`. Shows how lights
are ordinary scene-graph nodes and how transforms place subgraphs. Compile-verified twin:
`examples/builder_primitives.cpp`.

```cpp
#include <vsg/all.h>

int main(int argc, char** argv)
{
    auto scene = vsg::Group::create();

    // lighting — lights are nodes added to the graph
    auto ambient = vsg::AmbientLight::create();
    ambient->color = vsg::vec3(1.0f, 1.0f, 1.0f);
    ambient->intensity = 0.2f;
    scene->addChild(ambient);

    auto directional = vsg::DirectionalLight::create();
    directional->color = vsg::vec3(1.0f, 1.0f, 1.0f);
    directional->intensity = 0.8f;
    directional->direction = vsg::vec3(0.5f, 1.0f, -1.0f);
    scene->addChild(directional);

    // primitives positioned with MatrixTransforms
    auto builder = vsg::Builder::create();
    vsg::StateInfo stateInfo;

    auto placeAt = [&](vsg::ref_ptr<vsg::Node> node, const vsg::dvec3& position) {
        auto transform = vsg::MatrixTransform::create();
        transform->matrix = vsg::translate(position);
        transform->addChild(node);
        scene->addChild(transform);
    };

    vsg::GeometryInfo box;     placeAt(builder->createBox(box, stateInfo),         vsg::dvec3(-2.0, 0.0, 0.0));
    vsg::GeometryInfo sphere;  placeAt(builder->createSphere(sphere, stateInfo),   vsg::dvec3( 0.0, 0.0, 0.0));
    vsg::GeometryInfo capsule; placeAt(builder->createCapsule(capsule, stateInfo), vsg::dvec3( 2.0, 0.0, 0.0));

    // ... viewer + camera + render loop (see hello-vsg.md) ...
}
```

(The viewer/camera/loop section is identical to `hello-vsg.md`; the full program is in `examples/builder_primitives.cpp`.)

## What to copy

- **Lights are nodes**: `AmbientLight`/`DirectionalLight` are added to the graph with `addChild` like any other node; their contribution is gathered during traversal. Set `color` (a `vec3`) and `intensity` (a `float`); `DirectionalLight` also takes a `direction`.
- **Position with `MatrixTransform`**: wrap a subgraph in a `MatrixTransform` whose `matrix` is built from `vsg::translate` / `vsg::rotate` / `vsg::scale`. Transforms accumulate down the graph.
- **Builder factories**: `createBox`, `createSphere`, `createCapsule` (and `createCylinder`/`createCone`/`createQuad`) each take a `GeometryInfo` + `StateInfo` and return a ready-to-render subgraph (utils/Builder.h:123-130).

## Key APIs

- `Builder`, `GeometryInfo`, `StateInfo` — see `references/components/utils.md`.
- `AmbientLight`/`DirectionalLight` color/intensity/direction — see `references/components/lighting.md`.
- `MatrixTransform`, `addChild` — see `references/components/scene-graph.md` (nodes/Group.h:39) and `references/foundations/scene-and-state.md`.
- `vsg::translate` and the maths free functions — see `references/components/maths.md`.
