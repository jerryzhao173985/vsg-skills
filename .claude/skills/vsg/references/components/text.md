# Text

The `vsg/text` module provides high-quality GPU text rendering for a VSG scene graph using a signed-distance-field glyph atlas. An app author reaches for it whenever a label, HUD readout, or world-space caption must be drawn: build a `Text` node carrying a string, a `Font`, and a `TextLayout`, call `setup()` to construct the Vulkan rendering backend, then add the node to the scene graph. For large numbers of labels sharing one font, use `TextGroup` to batch them.

## Public includes

```cpp
#include <vsg/all.h>            // pulls in the whole library, including all text headers

// or target just what you need:
#include <vsg/text/Text.h>           // Text node (also includes Font.h, TextLayout.h, TextTechnique.h)
#include <vsg/text/TextGroup.h>      // batched Text rendering (includes Text.h)
#include <vsg/text/Font.h>           // Font, loaded via vsg::read_cast
#include <vsg/text/StandardLayout.h> // StandardLayout (includes TextLayout.h)
#include <vsg/text/CpuLayoutTechnique.h>  // optional explicit technique
#include <vsg/text/GpuLayoutTechnique.h>  // optional explicit technique
```

`Text.h` already pulls in `Font.h`, `TextLayout.h`, and `TextTechnique.h` (text/Text.h:16-18), and `StandardLayout.h` pulls in `TextLayout.h` (text/StandardLayout.h:16).

## Key classes

### vsg::Text

A renderable scene-graph node that draws a string using an SDF glyph atlas (text/Text.h:24-29). It does not do view-frustum culling or LOD itself; decorate it with a `CullNode`/`LOD` if needed (text/Text.h:25-28).

- `ref_ptr<Font> font;` — the font used to render the glyphs (text/Text.h:46).
- `ref_ptr<TextLayout> layout;` — controls position, alignment, size and color (text/Text.h:49).
- `ref_ptr<Data> text;` — the string to render, held as VSG data (e.g. a `stringValue`) (text/Text.h:50).
- `ref_ptr<ShaderSet> shaderSet;` — optional; the shaders to use, otherwise a default text shader set is created (text/Text.h:47).
- `ref_ptr<TextTechnique> technique;` — the rendering backend; created by `setup()` if not already set (text/Text.h:48).
- `virtual void setup(uint32_t minimumAllocation = 0, ref_ptr<const Options> options = {});` — builds the rendering backend; must be called after `font`, `layout`, and `text` are assigned and before the node is rendered. `minimumAllocation` hints how many glyphs to reserve space for (text/Text.h:52-54).

### vsg::TextGroup

A node for high-performance batched rendering of many `Text` labels that share one `Font`, `ShaderSet`, and `TextTechnique` (text/TextGroup.h:19-26). Per-child settings for those three are discarded; each child's local `TextLayout` still positions it (text/TextGroup.h:20-24).

- `ref_ptr<Font> font;` — the font shared by all children (text/TextGroup.h:48).
- `ref_ptr<ShaderSet> shaderSet;` — optional shared shader set (text/TextGroup.h:49).
- `Children children;` — vector of `ref_ptr<Text>` (text/TextGroup.h:52-53).
- `void addChild(ref_ptr<Text> text);` — append a `Text` child (text/TextGroup.h:55).
- `virtual void setup(uint32_t minimumAllocation = 0, ref_ptr<const Options> options = {});` — builds the shared rendering backend for all children (text/TextGroup.h:57-59).

### vsg::Font

Holds the SDF glyph atlas and per-glyph metrics for a typeface (text/Font.h:23). Normally loaded from a `.vsgb` font file via the IO system rather than built by hand.

- `float ascender;`, `float descender;`, `float height;` — vertical font metrics; `height` is the distance between consecutive baselines (text/Font.h:31-33).
- `ref_ptr<Data> atlas;` — the glyph atlas image data (text/Font.h:35).
- `ref_ptr<GlyphMetricsArray> glyphMetrics;` — per-glyph metrics (text/Font.h:36).
- `uint32_t glyphIndexForCharcode(uint32_t charcode) const;` — maps a character code to its glyph-metrics index (text/Font.h:42-47).

### vsg::TextLayout

Abstract base describing how a string is laid out into renderable quads (text/TextLayout.h:34). App authors normally instantiate `StandardLayout` rather than this base.

- `virtual bool requiresBillboard() const;` — whether the layout billboards toward the viewer (text/TextLayout.h:37).
- `virtual void layout(const Data* text, const Font& font, TextQuads& texQuads) = 0;` — produces the per-glyph quads (text/TextLayout.h:38).
- `virtual dbox extents(const Data* text, const Font& font) const = 0;` — the bounding box of the laid-out text, useful for sizing a `CullNode`/`LOD` (text/TextLayout.h:40).

### vsg::StandardLayout

The standard concrete `TextLayout`: positions, aligns, orients, colors and optionally billboards text (text/StandardLayout.h:21).

- `vec3 position;` — origin of the text in local space (default `(0,0,0)`) (text/StandardLayout.h:47).
- `vec3 horizontal;`, `vec3 vertical;` — text advance and line directions; their length controls glyph size (defaults `(1,0,0)` and `(0,1,0)`) (text/StandardLayout.h:48-49).
- `Alignment horizontalAlignment;`, `Alignment verticalAlignment;` — alignment relative to `position`; values `BASELINE_ALIGNMENT`, `LEFT_ALIGNMENT`/`TOP_ALIGNMENT`, `CENTER_ALIGNMENT`, `RIGHT_ALIGNMENT`/`BOTTOM_ALIGNMENT` (text/StandardLayout.h:27-35, 44-45).
- `GlyphLayout glyphLayout;` — `LEFT_TO_RIGHT_LAYOUT`, `RIGHT_TO_LEFT_LAYOUT`, or `VERTICAL_LAYOUT` (text/StandardLayout.h:37-42, 46).
- `vec4 color;` — glyph fill color (default opaque white) (text/StandardLayout.h:50).
- `vec4 outlineColor;`, `float outlineWidth;` — outline color and width (outline off when width is 0) (text/StandardLayout.h:51-52).
- `bool billboard;`, `float billboardAutoScaleDistance;` — billboarding toward the viewer and its auto-scale distance (text/StandardLayout.h:53-54).

## Idioms

Build a single label and add it to the scene:

```cpp
auto font = vsg::read_cast<vsg::Font>("fonts/times.vsgb", options);

auto layout = vsg::StandardLayout::create();
layout->position = vsg::vec3(0.0f, 0.0f, 0.0f);
layout->horizontal = vsg::vec3(1.0f, 0.0f, 0.0f);
layout->vertical = vsg::vec3(0.0f, 1.0f, 0.0f);
layout->color = vsg::vec4(1.0f, 1.0f, 1.0f, 1.0f);
layout->horizontalAlignment = vsg::StandardLayout::CENTER_ALIGNMENT;

auto text = vsg::Text::create();
text->font = font;
text->layout = layout;
text->text = vsg::stringValue::create("Hello, VSG!");
text->setup();   // builds the rendering backend; call after font/layout/text are set

scene->addChild(text);
```

Batch many labels sharing one font with a `TextGroup`:

```cpp
auto font = vsg::read_cast<vsg::Font>("fonts/times.vsgb", options);

auto group = vsg::TextGroup::create();
group->font = font;

for (auto& [pos, str] : labels)
{
    auto layout = vsg::StandardLayout::create();
    layout->position = pos;

    auto label = vsg::Text::create();
    label->layout = layout;
    label->text = vsg::stringValue::create(str);
    group->addChild(label);
}

group->setup();   // builds one shared backend for all children
scene->addChild(group);
```

Make a screen-facing billboarded label:

```cpp
auto layout = vsg::StandardLayout::create();
layout->billboard = true;
layout->billboardAutoScaleDistance = 1000.0f;
layout->position = vsg::vec3(10.0f, 20.0f, 5.0f);
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `text->text = "hi";` | `text->text = vsg::stringValue::create("hi");` | `Text::text` is a `ref_ptr<Data>`, not a `std::string`; wrap the string in a VSG data object (text/Text.h:50). |
| Add `Text` to the scene without `setup()` | Call `text->setup()` after setting `font`/`layout`/`text` | `setup()` creates the rendering backend (`technique`); without it there is nothing to record (text/Text.h:52-54). |
| Set per-child `font`/`shaderSet` on `TextGroup` children expecting them to apply | Set `font`/`shaderSet` on the `TextGroup` itself | A `TextGroup` shares one font/shaderSet/technique; local Text entries for these are discarded (text/TextGroup.h:20-22, 48-49). |
| Use a raw `new vsg::Text(...)` | `vsg::Text::create(...)` | Every VSG object is constructed via the `create()` factory from `Inherit`, returning a `ref_ptr` (text/Text.h:29). |
| Resize text by scaling the node transform only | Set `layout->horizontal`/`layout->vertical` length | Glyph size is driven by the layout's horizontal/vertical vectors (text/StandardLayout.h:48-49). |

## Things to never invent

- `Text` has no `setString()`/`getString()` or `setFont()` accessors — the public surface is plain `ref_ptr` fields (`font`, `layout`, `text`, `shaderSet`, `technique`) and `setup()` (text/Text.h:46-54).
- `TextLayout`/`StandardLayout` have no `setColor()`/`setAlignment()` methods — these are public fields (`color`, `horizontalAlignment`, etc.) on `StandardLayout` (text/StandardLayout.h:44-54).
- There is no `fontSize` field; sizing comes from `horizontal`/`vertical` vector length (text/StandardLayout.h:48-49).
- `StandardLayout::Alignment` does not include `JUSTIFY` or `MIDDLE`; the only enumerators are `BASELINE_ALIGNMENT`, `LEFT_ALIGNMENT`/`TOP_ALIGNMENT`, `CENTER_ALIGNMENT`, `RIGHT_ALIGNMENT`/`BOTTOM_ALIGNMENT` (text/StandardLayout.h:27-35).

## Source references

- include/vsg/text/Text.h
- include/vsg/text/TextGroup.h
- include/vsg/text/Font.h
- include/vsg/text/TextLayout.h
- include/vsg/text/StandardLayout.h
- include/vsg/text/TextTechnique.h
- include/vsg/text/CpuLayoutTechnique.h
- include/vsg/text/GpuLayoutTechnique.h
