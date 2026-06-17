# vsg-skills

A [Claude Code](https://claude.com/claude-code) **skill** that teaches an agent to build
native C++ applications with [VulkanSceneGraph](https://github.com/vsg-dev/VulkanSceneGraph)
(VSG) — correctly, idiomatically, and without inventing APIs.

It is an *adapter*, not a copy of the doxygen reference: it tells the agent what to read,
which APIs are public, which sources are authoritative, and how to verify generated code
actually compiles. Every rule cites a real `include/vsg/<module>/<Header>.h:line` (the
authoritative API) or `src/vsg/<module>/<File>.cpp:line` (idiomatic wiring), and the
documented API surface is proven by a real C++ compile-probe — not a name grep.

## What's inside

```
.claude/skills/vsg/
  SKILL.md                     orchestrator: setup, import rules, routing table, hard rules
  AGENTS.md                    one-screen spine: the 3 idioms + the canonical render loop
  references/
    foundations/   (3)         object-model · rendering-model · scene-and-state ("why")
    components/    (12)         core, scene-graph, app, state, maths, io, commands,
                               utils, text, lighting, animation, threading ("what")
    examples/      (4)         compile-verified exemplar docs (hello/view-model/builder + index)
    patterns.md                copy-paste recipes
    anti-patterns.md           Bad | Good | Why registry
  probe/                       C++ compile-probe (the validation gate) + CMake + runner
  examples/                    runnable example programs (build against installed VSG)
  scripts/                     check-skill-docs.sh (audit) + check-citations.py (resolver)

examples-consumer/             a downstream app built SOLELY from the skill (dogfood proof)
```

## Install

Copy the skill into the project you want the agent to work in:

```bash
cp -R vsg-skills/.claude/skills/vsg <your-project>/.claude/skills/vsg
```

It then auto-triggers whenever you write C++ that includes `<vsg/...>`, set up a
`vsg::Viewer`, or ask for VulkanSceneGraph code. Try:

> "Build a VSG app that loads a model from the command line and views it with a trackball camera and a directional light."

## Validate

Two independent grounding gates ship with the skill.

- **Compile / link** (needs the installed VSG, via `find_package(vsg)`; no VSG source required):

  ```bash
  .claude/skills/vsg/probe/run-probe.sh            # PROBE_RESULT=PASS
  .claude/skills/vsg/examples/build-examples.sh    # EXAMPLES_RESULT=PASS
  ```

  Override the install prefix with `VSG_PREFIX=/path` (default `/usr/local`).

- **Citation audit** (re-resolves every `file:line` against the VSG source tree the
  citations reference). Point it at a VulkanSceneGraph checkout:

  ```bash
  VSG_SRC=/path/to/VulkanSceneGraph .claude/skills/vsg/scripts/check-skill-docs.sh --probe
  ```

  If a `VulkanSceneGraph` checkout sits beside this repo, `VSG_SRC` is auto-detected.

## Provenance & version

- **Authoritative source**: the VSG headers (`include/vsg`, master `v1.1.15`); idiomatic
  wiring is grounded in `src/vsg`. The skill documents the public C++ API and its wiring.
- **Validated against**: a locally installed `vsg::vsg` `1.1.14` (one patch behind the
  cited master — the stable core API is identical across that delta; see
  `.claude/skills/vsg/probe/README.md`).
- Last full audit: `CHECK_SKILL_DOCS=PASS` — 1281/1281 citations resolve, 0 `[VERIFY]`,
  probe + examples + the `examples-consumer/` downstream app all compile & link, and the
  consumer ran headless for one frame (loaded a real model, compiled, presented, exit 0).

## License

MIT — see [LICENSE](LICENSE). VulkanSceneGraph itself is MIT-licensed by Robert Osfield;
this repository contains only the skill (documentation + validation tooling), not VSG's
source.
