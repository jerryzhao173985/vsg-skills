# Examples — complete, compile-verified VSG programs

Each example is a whole, runnable program (not a snippet). The annotated source lives
here for teaching; its compile-verified twin lives at `examples/<name>.cpp` and is built
against the installed `vsg::vsg` by `examples/build-examples.sh` (`EXAMPLES_RESULT=PASS`).

| Example | Teaches | Reference | Source |
|---|---|---|---|
| Minimal app | window + scene + camera + trackball + the compile-then-loop model | `references/examples/hello-vsg.md` | `examples/hello_vsg.cpp` |
| Load & view a model | `CommandLine`, `read_cast<Node>`, framing from `ComputeBounds` | `references/examples/view-model.md` | `examples/view_model.cpp` |
| Builder primitives + lights | `Builder` geometry, `MatrixTransform` placement, light nodes | `references/examples/builder-primitives.md` | `examples/builder_primitives.cpp` |
| Custom geometry & pipeline | hand-built path: `ShaderSet` + `GraphicsPipelineConfigurator` + `VertexIndexDraw` + material descriptor (no Builder) | `references/examples/custom-geometry.md` | `examples/custom_geometry.cpp` |

Build them all:

```bash
examples/build-examples.sh          # configure + build; prints EXAMPLES_RESULT=PASS|FAIL
```

For the per-class API behind any line, follow the routing table in `SKILL.md` to the
relevant `references/components/<area>.md` and `references/foundations/<topic>.md`.
