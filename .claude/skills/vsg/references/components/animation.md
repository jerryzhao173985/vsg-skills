# Animation

VSG's animation module drives scene-graph objects (transforms, cameras, skinned joints, morph targets) from time-based keyframe data. An app author reaches for it to play loaded animations (e.g. from glTF imports), to build animations programmatically (camera fly-throughs, moving transforms), or to record/replay camera paths. The core flow is: an `Animation` holds one or more `AnimationSampler`s; each sampler samples its keyframes at the current animation time and writes the result into a bound scene-graph `object`; the `AnimationManager` (reachable from the `Viewer`) starts animations and updates them every frame.

## Public includes

```cpp
#include <vsg/all.h>   // pulls in everything below

// or target specific headers:
#include <vsg/animation/Animation.h>          // Animation, AnimationSampler
#include <vsg/animation/AnimationManager.h>   // AnimationManager
#include <vsg/animation/AnimationGroup.h>     // AnimationGroup node
#include <vsg/animation/TransformSampler.h>   // TransformSampler, TransformKeyframes
#include <vsg/animation/CameraSampler.h>      // CameraSampler, CameraKeyframes
#include <vsg/animation/Joint.h>              // Joint
#include <vsg/animation/JointSampler.h>       // JointSampler
#include <vsg/animation/FindAnimations.h>     // FindAnimations visitor
#include <vsg/animation/CameraAnimationHandler.h>
```

The `AnimationManager` header is also pulled in transitively by `<vsg/app/Viewer.h>` (app/Viewer.h:15).

## Key classes

### vsg::Animation
Controls a single animation made up of one or more samplers (animation/Animation.h:43).

- `std::string name` — human-readable name (animation/Animation.h:49).
- `enum Mode { ONCE, REPEAT, FORWARD_AND_BACK }` and `Mode mode = REPEAT` — how time outside the sampler's `maxTime` period is handled (animation/Animation.h:51, animation/Animation.h:59).
- `double speed = 1.0` — multiplier mapping simulation-time delta to animation-time delta (animation/Animation.h:65).
- `double time = 0.0` — current local animation time, updated automatically by `update(..)` (animation/Animation.h:62).
- `Samplers samplers` — `std::vector<ref_ptr<AnimationSampler>>`; the samplers that drive objects (animation/Animation.h:68-69).
- `virtual bool start(double simulationTime, double startTime = 0.0)` — initialize the start time (animation/Animation.h:72).
- `virtual bool update(double simulationTime)` — advance time and update all samplers (animation/Animation.h:75).
- `virtual bool stop(double simulationTime)` — signal the animation to stop (animation/Animation.h:78).
- `bool active() const` — true while being updated (animation/Animation.h:81).
- `virtual double maxTime() const` — the longest keyframe time across samplers (animation/Animation.h:84).

Prefer driving start/stop through the `AnimationManager` rather than calling these directly.

### vsg::AnimationManager
Plays/updates animations as part of `Viewer::update()` (animation/AnimationManager.h:23).

- `std::list<ref_ptr<Animation>> animations` — animations currently playing (animation/AnimationManager.h:29).
- `virtual bool play(ref_ptr<Animation> animation, double startTime = 0.0)` — start playing an animation (animation/AnimationManager.h:37).
- `virtual bool stop(ref_ptr<Animation> animation)` — stop one animation (animation/AnimationManager.h:40).
- `virtual bool stop()` — stop all running animations (animation/AnimationManager.h:43).
- `virtual void run(ref_ptr<FrameStamp> frameStamp)` — update all playing animations; called automatically by `Viewer::update()` (animation/AnimationManager.h:49).

A default `AnimationManager` is held by the viewer as `viewer->animationManager` (app/Viewer.h:91), so an app author normally just calls `viewer->animationManager->play(animation)`.

### vsg::AnimationGroup
A `Group` node that additionally carries a list of animations to animate its children (animation/AnimationGroup.h:23).

- `Animations animations` — the animations associated with this subgraph (animation/AnimationGroup.h:29).
- Inherits `Group::children`; traversal visits both animations and children (animation/AnimationGroup.h:35-43).

Loaders (e.g. glTF) commonly produce `AnimationGroup` nodes; use `FindAnimations` to collect their animations.

### vsg::TransformSampler
Samples position/rotation/scale keyframes and writes them to a bound transform/joint/camera object (animation/TransformSampler.h:69).

- `ref_ptr<TransformKeyframes> keyframes` — the keyframe data to sample (animation/TransformSampler.h:75).
- `ref_ptr<Object> object` — the scene-graph object to drive (e.g. a `MatrixTransform`) (animation/TransformSampler.h:76).
- `dvec3 position; dquat rotation; dvec3 scale` — the sampled values for the current time (animation/TransformSampler.h:79-81).
- `void update(double time) override` — sample keyframes at `time` and apply to `object` (animation/TransformSampler.h:83).
- `dmat4 transform() const` — composed `translate*rotate*scale` matrix (animation/TransformSampler.h:86).
- `apply(...)` overrides write the sampled transform into a `MatrixTransform`, `Joint`, `LookAt`, `Camera`, or `mat4Value/dmat4Value` (animation/TransformSampler.h:95-100).

`TransformKeyframes` holds `positions`, `rotations`, `scales` vectors and convenience `add(time, position, rotation[, scale])` methods (animation/TransformSampler.h:35-61).

### vsg::CameraSampler
Samples camera keyframes and writes view/projection state into a camera object (animation/CameraSampler.h:110).

- `ref_ptr<CameraKeyframes> keyframes` — keyframe data (animation/CameraSampler.h:116).
- `ref_ptr<Object> object` — the object to drive, e.g. a `Camera`, `LookAt`, `LookDirection`, or `Perspective` (animation/CameraSampler.h:117).
- Sampled values `origin`, `position`, `rotation`, `fieldOfView`, `nearFar` (animation/CameraSampler.h:120-124).
- `void update(double time) override` (animation/CameraSampler.h:126); `dmat4 transform() const` (animation/CameraSampler.h:129).
- `apply(...)` overrides for `LookAt`, `LookDirection`, `Perspective`, `Camera`, `mat4Value/dmat4Value` (animation/CameraSampler.h:138-143).

`CameraKeyframes` provides overloaded `add(...)` helpers for position/rotation/fov/nearFar keys (animation/CameraSampler.h:59-102).

### vsg::Joint
A skeleton node forming a joint hierarchy for skinned animation (animation/Joint.h:21).

- `unsigned int index = 0` — index into the joint-matrix array uploaded to the GPU (animation/Joint.h:27).
- `std::string name` (animation/Joint.h:28); `dmat4 matrix` — the joint's local matrix (animation/Joint.h:29).
- `Children children` and `void addChild(ref_ptr<Node>)` (animation/Joint.h:31-37).

### vsg::JointSampler
Accumulates a `Joint` hierarchy's transforms and writes the resulting joint matrices to a GPU array (animation/JointSampler.h:22).

- `ref_ptr<mat4Array> jointMatrices` — the array uploaded to the GPU skinning shader (animation/JointSampler.h:28).
- `std::vector<dmat4> offsetMatrices` — per-joint inverse-bind / offset matrices (animation/JointSampler.h:29).
- `ref_ptr<Node> subgraph` — the joint subgraph to traverse and accumulate (animation/JointSampler.h:30).
- `void update(double time) override` — traverse `subgraph` and fill `jointMatrices` (animation/JointSampler.h:32).

## Idioms

Play a loaded animation through the viewer:

```cpp
auto scene = vsg::read_cast<vsg::Node>(filename, options);

// collect animations from the loaded subgraph
auto findAnimations = vsg::visit<vsg::FindAnimations>(scene);

viewer->compile();
if (!findAnimations.animations.empty())
    viewer->animationManager->play(findAnimations.animations.front());

// in the frame loop, Viewer::update() drives animationManager->run(...)
while (viewer->advanceToNextFrame())
{
    viewer->handleEvents();
    viewer->update();   // updates all playing animations
    viewer->recordAndSubmit();
    viewer->present();
}
```

Build a transform animation programmatically and bind it to a MatrixTransform:

```cpp
auto transform = vsg::MatrixTransform::create();

auto keyframes = vsg::TransformKeyframes::create();
keyframes->add(0.0, vsg::dvec3(0.0, 0.0, 0.0), vsg::dquat());
keyframes->add(2.0, vsg::dvec3(5.0, 0.0, 0.0), vsg::dquat());

auto sampler = vsg::TransformSampler::create();
sampler->keyframes = keyframes;
sampler->object = transform;            // sampler drives this object

auto animation = vsg::Animation::create();
animation->name = "slide";
animation->mode = vsg::Animation::REPEAT;
animation->samplers.push_back(sampler);

viewer->animationManager->play(animation);
```

Record/playback a camera path with CameraAnimationHandler:

```cpp
auto cameraAnimation = vsg::CameraAnimationHandler::create(camera, "saved_animation.vsgt", options);
viewer->addEventHandler(cameraAnimation);
// press 'r' to toggle recording, 'p' to toggle playback (default keys)
```

## Common mistakes

| Bad | Good | Why |
|-----|------|-----|
| `auto a = new vsg::Animation();` | `auto a = vsg::Animation::create();` | All VSG classes are created via the static `create(...)` factory returning a `ref_ptr` (animation/Animation.h:87). |
| Calling `animation->update(t)` yourself each frame | `viewer->animationManager->play(animation)` and let `viewer->update()` run it | `AnimationManager::run(...)` is called automatically by `Viewer::update()` and tracks simulation time (animation/AnimationManager.h:49). |
| Creating a `TransformSampler` but leaving `object` unset | Set `sampler->object = transform;` | The sampler only drives the object bound to its `object` field (animation/TransformSampler.h:76). |
| Manually walking the scene to find animations | `vsg::visit<vsg::FindAnimations>(scene)` then read `.animations` | `FindAnimations` collects `Animation`/`AnimationGroup` instances via a visitor (animation/FindAnimations.h:20-28). |

## Things to never invent

- `MorphSampler` exists but its update is not yet implemented — do not rely on it for working morph animation (animation/MorphSampler.h:46).
- There is no `Animation::play()` method; play/stop are on `AnimationManager` (animation/AnimationManager.h:37-43); `Animation` only has `start`/`update`/`stop`/`active`/`maxTime` (animation/Animation.h:72-84).
- `AnimationSampler` exposes `update(double)` and `maxTime()`; there is no generic `setObject(...)` — bind via the concrete sampler's public `object` field (animation/Animation.h:31-32, animation/TransformSampler.h:76).

## Source references

- include/vsg/animation/Animation.h
- include/vsg/animation/AnimationManager.h
- include/vsg/animation/AnimationGroup.h
- include/vsg/animation/TransformSampler.h
- include/vsg/animation/CameraSampler.h
- include/vsg/animation/Joint.h
- include/vsg/animation/JointSampler.h
- include/vsg/animation/MorphSampler.h
- include/vsg/animation/CameraAnimationHandler.h
- include/vsg/animation/FindAnimations.h
- include/vsg/animation/time_value.h
- include/vsg/app/Viewer.h
