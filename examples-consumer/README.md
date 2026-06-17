# vsg_consumer_test — downstream dogfood project

A fresh, external native C++ VulkanSceneGraph application written **solely from the
guidance in `../VulkanSceneGraph/.claude/skills/vsg/`** (SKILL.md + references/patterns.md
+ components/). It exists to prove the `vsg` skill is sufficient to build a real project,
not just internally consistent.

It exercises, using the API forms exactly as the skill documents them:

- command-line parsing (`vsg::CommandLine`)
- viewer + window setup (`WindowTraits::create("title")`, `Window::create`, `Viewer::create`)
- camera setup (2-arg `vsg::Camera::create(perspective, lookAt)`)
- command graph creation (`createCommandGraphForView` + `assignRecordAndSubmitTaskAndPresentation`)
- scene-graph structure (a `Group` hierarchy)
- transforms (`MatrixTransform` + `vsg::translate`/`vsg::scale`)
- state + Builder usage (`GeometryInfo.position`/`.color`, `StateInfo.lighting`, `createBox`/`createSphere`/`createCapsule`)
- model loading from a local asset (`read_cast<vsg::Node>` on `teapot.vsgt`)

## Build

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/usr/local
cmake --build build -j
```

Result: **compiles and links cleanly** against the installed `vsg::vsg`, with zero changes
required to the skill. Running needs a display + Vulkan driver; building does not.
