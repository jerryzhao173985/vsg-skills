# Maths

VSG's maths module provides the header-only vector, matrix, quaternion, and bounding-volume types that every VSG application uses to position cameras, build model transforms, and compute bounds. Reach for it whenever you set up a view (`lookAt`/`perspective`), feed a `MatrixTransform` (`translate`/`rotate`/`scale`), interpolate or rotate vectors, or test/accumulate spatial bounds. All types are plain value structs (not `vsg::Object` subclasses), so you construct them directly with `{...}` or their constructors and pass them by value — there is no `::create()` factory here.

## Public includes

```cpp
#include <vsg/all.h>            // pulls in every maths header (and the rest of VSG)
// or include only what you need:
#include <vsg/maths/vec3.h>      // vec3/dvec3 + length, normalize, dot, cross, mix
#include <vsg/maths/vec4.h>      // vec4/dvec4
#include <vsg/maths/mat4.h>      // mat4/dmat4 and matrix * vector/matrix operators
#include <vsg/maths/mat3.h>      // mat3/dmat3
#include <vsg/maths/quat.h>      // quat/dquat
#include <vsg/maths/transform.h> // lookAt, perspective, translate, rotate, scale, inverse, decompose
#include <vsg/maths/box.h>       // box/dbox axis-aligned bounding box
#include <vsg/maths/sphere.h>    // sphere/dsphere bounding sphere
#include <vsg/maths/common.h>    // radians/degrees, mix, smoothstep, PI/PIf
```

These types are value structs, not reference-counted `vsg::Object`s, so unlike the rest of VSG you never call `::create()` on them — construct them directly.

## Key classes

### vsg::vec3 (and dvec3)
3D vector; `vec3` is `t_vec3<float>` and `dvec3` is `t_vec3<double>` (maths/vec3.h:157-158).
- Construct with `vec3(x, y, z)` or `{x, y, z}`; default-constructs to zero (maths/vec3.h:54-60).
- Access components via `.x/.y/.z`, `.r/.g/.b`, `.s/.t/.p`, or `operator[]` — they alias the same storage (maths/vec3.h:42-51, maths/vec3.h:72-73).
- `set(x, y, z)`, `data()` for a raw `T*`, and `size()` returning 3 (maths/vec3.h:84-92, maths/vec3.h:70).
- Arithmetic operators `+ - *` (component-wise) and `*`/`/` by scalar; `+= -= *= /=` in place (maths/vec3.h:94-152, maths/vec3.h:200-235).
- Free functions: `length(v)`, `length2(v)`, `normalize(v)`, `dot(a,b)`, `cross(a,b)`, `mix(start,end,r)`, `orthogonal(v)` (maths/vec3.h:238-294).
- Other typedefs: `ivec3`, `uivec3`, `ubvec3`, `bvec3`, `svec3`, `usvec3`, `ldvec3` (maths/vec3.h:159-165).

### vsg::vec4 (and dvec4)
4D vector; `vec4` is `t_vec4<float>`, `dvec4` is `t_vec4<double>` (maths/vec4.h:187-188).
- Construct with `vec4(x,y,z,w)`, `{...}`, or from a `vec3` plus w: `vec4(v3, w)` (maths/vec4.h:63-79).
- Components `.x/.y/.z/.w`, `.r/.g/.b/.a`; the `.xyz`/`.rgb` members expose the leading vec3 (maths/vec4.h:43-55).
- Convertible to/from Vulkan's `VkClearColorValue` (maths/vec4.h:66-67, maths/vec4.h:182).
- Same arithmetic and `length`/`normalize`/`mix` free functions as vec3 (maths/vec4.h:269-295). Note: `vec4` has no `cross`.

### vsg::mat4 (and dmat4)
Column-major 4x4 matrix; `mat4` is `t_mat4<float>`, `dmat4` is `t_mat4<double>` (maths/mat4.h:122-123). Default constructs to identity (maths/mat4.h:32-36).
- `value` is a `column_type[4]` of `t_vec4<T>`; `m[c]` returns column c, `m(c,r)` returns the element at column c, row r (maths/mat4.h:30, maths/mat4.h:82-86).
- 16-scalar constructor takes values **in column order** (maths/mat4.h:44-53); `set(...)` mirrors it (maths/mat4.h:98-107).
- `data()` gives a contiguous `T*` of 16 values suitable for upload (maths/mat4.h:118-119).
- `operator*` composes two matrices, transforms a `vec4`, and transforms a `vec3` with perspective divide (maths/mat4.h:170-176, maths/mat4.h:179-185, maths/mat4.h:227-233).

### vsg::mat3 (and dmat3)
Column-major 3x3 matrix; `mat3` is `t_mat3<float>`, `dmat3` is `t_mat3<double>` (maths/mat3.h:109-110). Default constructs to identity (maths/mat3.h:30-33). Same `m[c]`, `m(c,r)`, `data()`, and `*` operators as mat4 but 3x3; useful for normal matrices via `inverse_3x3` (maths/mat3.h:73-86, maths/mat3.h:150-163).

### vsg::quat (and dquat)
Quaternion; `quat` is `t_quat<float>`, `dquat` is `t_quat<double>` (maths/quat.h:145-146). Default constructs to identity `(0,0,0,1)` (maths/quat.h:47-48).
- Construct from `(angle_radians, axis)` for an axis-angle rotation, or `(from, to)` to rotate one direction onto another (maths/quat.h:53-60).
- Components `.x/.y/.z/.w` and `set(...)` overloads matching the constructors (maths/quat.h:38-45, maths/quat.h:86-112).
- `operator*` composes two quaternions and rotates a `vec3` directly: `q * v` (maths/quat.h:203-225).
- Free functions: `normalize`, `conjugate`, `inverse`, `dot`, `length`, and `mix` (spherical/linear blend) (maths/quat.h:184-265, maths/quat.h:268-293).
- Convert to a rotation matrix with `vsg::rotate(q)` from transform.h (maths/transform.h:20-41).

### vsg::box (and dbox)
Axis-aligned bounding box of `min`/`max` `vec3` corners; `box` is `t_box<float>` (maths/box.h:91-93).
- Default constructs to an "empty/invalid" box (`min` huge, `max` lowest); `valid()` is true once a point is added (maths/box.h:28-29, maths/box.h:50).
- `add(x,y,z)`, `add(vec3)`, and `add(otherBox)` grow the box to enclose the input (maths/box.h:63-88).
- `reset()` returns it to the empty state (maths/box.h:57-61).

### vsg::sphere (and dsphere)
Bounding sphere; `sphere` is `t_sphere<float>`, `dsphere` is `t_sphere<double>` (maths/sphere.h:123-124).
- Access via `.center` (a vec3) and `.radius`, or `.x/.y/.z/.r`, or `.vec` (the vec4) — all alias the same storage (maths/sphere.h:39-55).
- Construct with `sphere(center, radius)` or `sphere(x,y,z,r)` (maths/sphere.h:69-75).
- Default constructs invalid (radius `-1`); `valid()` is true when radius >= 0 (maths/sphere.h:57-58, maths/sphere.h:109).

### transform.h free functions
Factory free functions that build `t_mat4<T>` transforms and a Vulkan-correct reverse-depth projection.
- `translate(x,y,z)` / `translate(vec3)` build a translation matrix (maths/transform.h:67-82).
- `rotate(angle_radians, x,y,z)` / `rotate(angle_radians, vec3)` / `rotate(quat)` build a rotation matrix (maths/transform.h:44-63, maths/transform.h:20-41).
- `scale(s)` / `scale(sx,sy,sz)` / `scale(vec3)` build a scale matrix (maths/transform.h:85-113).
- `lookAt(eye, center, up)` builds a view matrix (maths/transform.h:178-196).
- `perspective(fovy_radians, aspectRatio, zNear, zFar)` builds a **reverse-depth** projection with Y inverted for Vulkan (maths/transform.h:139-150).
- `orthographic(left,right,bottom,top,zNear,zFar)` for orthographic projection (maths/transform.h:167-176).
- `transpose(m)`, `inverse(m)`, `inverse_4x3(m)`, `inverse_4x4(m)`, `inverse_3x3(m)`, `determinant(m)` (maths/transform.h:126-132, maths/transform.h:233-260).
- `decompose(m, translation, rotation, scale)` splits a matrix into TRS components (maths/transform.h:262-275).
- `computeTransform(nodePath)` accumulates the transforms along a node path into a `dmat4` (maths/transform.h:297-301).

### common.h helpers
- `radians(degrees)` / `degrees(radians)` for float and double (maths/common.h:31-36).
- `square(v)`, `mix(start,end,r)`, `smoothstep(...)` (maths/common.h:39-71).
- Constants `vsg::PI` (double) and `vsg::PIf` (float) (maths/common.h:27-28).

## Idioms

```cpp
#include <vsg/maths/transform.h>

// Build a model matrix: translate * rotate * scale (applied right-to-left).
vsg::dmat4 model = vsg::translate(vsg::dvec3(10.0, 0.0, 0.0)) *
                   vsg::rotate(vsg::radians(45.0), vsg::dvec3(0.0, 0.0, 1.0)) *
                   vsg::scale(2.0);
```

```cpp
// Camera view + reverse-depth Vulkan projection.
auto view = vsg::lookAt(vsg::dvec3(0.0, -10.0, 5.0),  // eye
                        vsg::dvec3(0.0, 0.0, 0.0),     // center
                        vsg::dvec3(0.0, 0.0, 1.0));    // up
double aspect = 1280.0 / 720.0;
auto proj = vsg::perspective(vsg::radians(60.0), aspect, 0.1, 1000.0);
```

```cpp
#include <vsg/maths/quat.h>
// Axis-angle quaternion, then rotate a vector with it.
vsg::dquat q(vsg::radians(90.0), vsg::dvec3(0.0, 0.0, 1.0));
vsg::dvec3 rotated = q * vsg::dvec3(1.0, 0.0, 0.0);  // ~ (0,1,0)
```

```cpp
#include <vsg/maths/box.h>
// Accumulate a bounding box over points.
vsg::dbox bounds;                       // starts invalid
bounds.add(vsg::dvec3(-1.0, -1.0, -1.0));
bounds.add(vsg::dvec3( 1.0,  1.0,  1.0));
if (bounds.valid()) { auto centre = (bounds.min + bounds.max) * 0.5; }
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `vsg::rotate(45.0, axis)` | `vsg::rotate(vsg::radians(45.0), axis)` | `rotate` takes radians, not degrees; convert with `vsg::radians` (maths/transform.h:44, maths/common.h:32). |
| `auto m = vsg::mat4::create();` | `vsg::mat4 m;` | Maths types are plain value structs with no `::create()` factory; default-constructs to identity (maths/mat4.h:32-36). |
| `vsg::mat4 m(a,b,c,d, ...)` assuming row order | fill in **column order** | The 16-scalar `t_mat4` constructor is column-major (maths/mat4.h:44-53). |
| Mixing `vec3` (float) with `dvec3` (double) in one expression | keep one precision, or convert explicitly e.g. `vsg::vec3(dv)` | Cross-type vec constructors are `explicit`; operators are same-type (maths/vec3.h:67, maths/vec3.h:200). |
| Calling `cross(a, b)` on `vec4` | use `vec3` for cross products | `cross` is defined for `t_vec3`/`t_vec2`, not `t_vec4` (maths/vec3.h:262, vec4.h has none). |

## Things to never invent

- There is no `vsg::vec3::create()` (or any maths `::create()`); these are not `vsg::Object` subclasses — construct directly (maths/vec3.h:33-60).
- No `cross()` for `vec4` — only `vec2` and `vec3` define it (maths/vec3.h:262, maths/vec2.h:254).
- `perspective` is reverse-depth (1->0) with Y flipped for Vulkan; do not assume an OpenGL-style projection or hand-roll `glm::perspective` semantics (maths/transform.h:134-150).
- `lookAt`/`perspective` are free functions in `vsg::`, not methods on a matrix type (maths/transform.h:139, maths/transform.h:178).

## Source references

- include/vsg/maths/vec2.h
- include/vsg/maths/vec3.h
- include/vsg/maths/vec4.h
- include/vsg/maths/mat3.h
- include/vsg/maths/mat4.h
- include/vsg/maths/quat.h
- include/vsg/maths/transform.h
- include/vsg/maths/box.h
- include/vsg/maths/sphere.h
- include/vsg/maths/numbers.h
- include/vsg/maths/common.h
- include/vsg/maths/color.h
