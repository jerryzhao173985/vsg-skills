# Lighting & shadows

VSG models light sources as scene-graph nodes. You create a `vsg::Light` subclass (`AmbientLight`, `DirectionalLight`, `PointLight`, or `SpotLight`), set its color and intensity, and add it to the scene graph (typically under a transform that positions it). During recording, the `ViewDependentState` attached to the `View` gathers all lights it encounters and packs them into the uniform data the shaders read, so lighting "just works" for pipelines built with the standard `ViewDescriptorSetLayout`. To cast shadows from a light, attach a `ShadowSettings` subclass (`HardShadows`, `SoftShadows`, or `PercentageCloserSoftShadows`) to the light's `shadowSettings` field. Reach for this module whenever you need illumination, headlights, or shadowing in a 3D scene.

## Public includes

```cpp
#include <vsg/all.h>                          // pulls in everything below

// or target the specific headers you use:
#include <vsg/lighting/Light.h>               // base class + createHeadlight()
#include <vsg/lighting/AmbientLight.h>
#include <vsg/lighting/DirectionalLight.h>
#include <vsg/lighting/PointLight.h>
#include <vsg/lighting/SpotLight.h>
#include <vsg/lighting/ShadowSettings.h>
#include <vsg/lighting/HardShadows.h>
#include <vsg/lighting/SoftShadows.h>
#include <vsg/lighting/PercentageCloserSoftShadows.h>
```

`<vsg/all.h>` includes all nine lighting headers (all.h:104-112).

## Key classes

### vsg::Light

Base node class for all light types; provides the name, color, intensity, and optional shadow settings shared by every light (lighting/Light.h:22-25).

- `std::string name` — identifier for the light source (lighting/Light.h:31).
- `vec3 color = vec3(1.0f, 1.0f, 1.0f)` — RGB color, default white (lighting/Light.h:32).
- `float intensity = 1.0f` — scalar multiplier applied to the color (lighting/Light.h:33).
- `ref_ptr<ShadowSettings> shadowSettings` — optional; assign a shadow technique here to make the light cast shadows (lighting/Light.h:34).
- `Light` is a `Node` (`Inherit<Node, Light>`), so you add it to the scene graph like any other node (lighting/Light.h:25). It is abstract in practice — instantiate one of the subclasses below.
- Free function `ref_ptr<vsg::Node> createHeadlight()` — builds a subgraph with a white `AmbientLight` (intensity 0.05) and `DirectionalLight` (intensity 0.95) for headlight illumination (lighting/Light.h:48-49).

### vsg::AmbientLight

Represents an ambient (non-directional, uniform) light source (lighting/AmbientLight.h:22-23).

- Adds no fields of its own; uses the inherited `color` and `intensity` from `Light` (lighting/AmbientLight.h:23-34).
- Construct via `vsg::AmbientLight::create()` (lighting/AmbientLight.h:30).

### vsg::DirectionalLight

Light treated as infinitely distant (e.g. sun/moon); has a direction but no position (lighting/DirectionalLight.h:22-23).

- `dvec3 direction = dvec3(0.0, 0.0, -1.0)` — direction the light travels (lighting/DirectionalLight.h:29).
- `float angleSubtended = 0.0090f` — angular size of the light source, used for soft-shadow penumbra (lighting/DirectionalLight.h:30).
- Inherits `color`/`intensity`; no `position` field (it is directional) (lighting/DirectionalLight.h:23-29).

### vsg::PointLight

Local point source radiating from a single position in all directions (lighting/PointLight.h:22-23).

- `dvec3 position = dvec3(0.0, 0.0, 0.0)` — world/local position of the source (lighting/PointLight.h:29).
- `double radius = 0.0` — physical radius of the light source (lighting/PointLight.h:30).
- No `direction` field (it radiates omnidirectionally) (lighting/PointLight.h:29-30).

### vsg::SpotLight

Local source whose intensity is shaped into a cone (lighting/SpotLight.h:22-23).

- `dvec3 position = dvec3(0.0, 0.0, 0.0)` — apex of the cone (lighting/SpotLight.h:29).
- `dvec3 direction = dvec3(0.0, 0.0, -1.0)` — axis the cone points along (lighting/SpotLight.h:30).
- `double innerAngle = radians(30.0)` — half-angle of the full-intensity inner cone (lighting/SpotLight.h:31).
- `double outerAngle = radians(45.0)` — half-angle where intensity falls to zero; set with `vsg::radians(...)` (lighting/SpotLight.h:32).
- `double radius = 0.0` — physical radius of the source (lighting/SpotLight.h:33).

### vsg::ShadowSettings

Base configuration object attached to a `Light::shadowSettings` to enable shadow casting (lighting/ShadowSettings.h:20-23).

- `explicit ShadowSettings(uint32_t shadowMaps = 1)` — constructor selecting how many shadow maps (cascades) to use (lighting/ShadowSettings.h:23).
- `uint32_t shadowMapCount = 1` — number of shadow maps allocated for the light (lighting/ShadowSettings.h:26).
- Abstract base; assign one of the concrete techniques below, not `ShadowSettings` itself (lighting/ShadowSettings.h:20).

#### Shadow technique subclasses (brief)

- `vsg::HardShadows` — sharp-edged shadows; `HardShadows::create(uint32_t in_shadowMaps = 1)` (lighting/HardShadows.h:20-23).
- `vsg::SoftShadows` — soft penumbra; `SoftShadows::create(uint32_t in_shadowMaps = 1, float in_penumbraRadius = 0.05f)`, with field `float penumbraRadius = 0.05f` (lighting/SoftShadows.h:20-26).
- `vsg::PercentageCloserSoftShadows` — PCSS soft shadows; `PercentageCloserSoftShadows::create(uint32_t in_shadowMaps = 1)` (lighting/PercentageCloserSoftShadows.h:20-23).

## Idioms

```cpp
// Ambient + directional "sun" added to the scene graph.
auto scene = vsg::Group::create();

auto ambient = vsg::AmbientLight::create();
ambient->name = "ambient";
ambient->color = vsg::vec3(1.0f, 1.0f, 1.0f);
ambient->intensity = 0.05f;
scene->addChild(ambient);

auto sun = vsg::DirectionalLight::create();
sun->name = "sun";
sun->color = vsg::vec3(1.0f, 1.0f, 1.0f);
sun->intensity = 0.95f;
sun->direction = vsg::dvec3(0.0, 0.0, -1.0);
scene->addChild(sun);
```

```cpp
// A spot light that casts hard shadows.
auto spot = vsg::SpotLight::create();
spot->position = vsg::dvec3(0.0, 0.0, 10.0);
spot->direction = vsg::dvec3(0.0, 0.0, -1.0);
spot->innerAngle = vsg::radians(20.0);
spot->outerAngle = vsg::radians(30.0);
spot->shadowSettings = vsg::HardShadows::create(1);
scene->addChild(spot);
```

```cpp
// A point light with soft shadows, positioned via the light's own field.
auto bulb = vsg::PointLight::create();
bulb->position = vsg::dvec3(2.0, 1.0, 3.0);
bulb->color = vsg::vec3(1.0f, 0.9f, 0.8f);
bulb->intensity = 5.0f;
bulb->shadowSettings = vsg::SoftShadows::create(1, 0.1f);
scene->addChild(bulb);
```

```cpp
// Quick camera-attached illumination.
scene->addChild(vsg::createHeadlight());
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `new vsg::DirectionalLight()` | `vsg::DirectionalLight::create()` | All VSG objects are made via the static `create()` factory returning a `ref_ptr` (lighting/DirectionalLight.h:33). |
| Setting a `position` on a `DirectionalLight` | Set `direction` instead (lighting/DirectionalLight.h:29) | Directional lights are infinitely distant and have no position field; only `direction` and `angleSubtended` exist. |
| `light->shadowSettings = vsg::ShadowSettings::create()` to "turn on shadows" | Assign a concrete technique, e.g. `vsg::HardShadows::create()` (lighting/HardShadows.h:23) | `ShadowSettings` is the abstract base; the technique subclass selects the actual shadowing algorithm. |
| Passing degrees to `SpotLight::outerAngle` | `spot->outerAngle = vsg::radians(45.0)` (lighting/SpotLight.h:32) | The cone angles are stored in radians; the defaults are written via `radians(...)`. |
| Creating a light but never adding it to the scene graph | `scene->addChild(light)` | Lights are nodes; `ViewDependentState` only gathers lights it traverses during recording (state/ViewDependentState.h:98-99). |

## Things to never invent

- No `enabled`/`on` boolean or `setColor()`/`setIntensity()` setters — `color` and `intensity` are public fields (lighting/Light.h:32-33).
- No `attenuation`/`constant`/`linear`/`quadratic` falloff fields on `PointLight` — only `position` and `radius` exist (lighting/PointLight.h:29-30).
- No `position` field on `DirectionalLight`, and no `direction` field on `PointLight` or `AmbientLight` (lighting/DirectionalLight.h:29, lighting/PointLight.h:29).
- `maxShadowDistance` and `shadowMapBias` are fields of `ViewDependentState`, not of `Light` or `ShadowSettings` (state/ViewDependentState.h:154-155).
- `penumbraRadius` exists on `SoftShadows`, not on `HardShadows` or `PercentageCloserSoftShadows` (lighting/SoftShadows.h:26).

## Source references

- include/vsg/lighting/Light.h
- include/vsg/lighting/AmbientLight.h
- include/vsg/lighting/DirectionalLight.h
- include/vsg/lighting/PointLight.h
- include/vsg/lighting/SpotLight.h
- include/vsg/lighting/ShadowSettings.h
- include/vsg/lighting/HardShadows.h
- include/vsg/lighting/SoftShadows.h
- include/vsg/lighting/PercentageCloserSoftShadows.h
- include/vsg/state/ViewDependentState.h (for `maxShadowDistance`/`shadowMapBias` context)
- include/vsg/all.h (umbrella include)
