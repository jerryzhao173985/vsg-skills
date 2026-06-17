# Object model & memory

Every persistent thing in VulkanSceneGraph — nodes, vertex arrays, uniforms, visitors, state — derives from `vsg::Object`, which carries an intrusive atomic reference count and routes its allocations through a custom allocator singleton. You never call `new` or `delete` directly: you construct with `ClassName::create(...)`, which hands back a `vsg::ref_ptr<>` smart pointer, and lifetime ends automatically when the last `ref_ptr` drops. Before writing any VSG code you must internalize four contracts that all sit on top of `Object`: intrusive ref-counting via `ref()`/`unref()`, the `Inherit<Parent,Self>` CRTP base that injects `create()`/`accept()`/RTTI, the `ref_ptr` vs `observer_ptr` ownership distinction (and why cycles need the weak one), the `Visitor` double-dispatch traversal pattern, and the `Data`/`Value`/`Array` containers that hand CPU memory to the GPU.

## The model

`vsg::Object` holds a single `mutable std::atomic_uint _referenceCount` (core/Object.h:190). `ref()` does a relaxed atomic increment (core/Object.h:122); `unref()` does a `seq_cst` decrement and, when the count falls to one or below, calls `_attemptDelete()` (core/Object.h:123-126). The reference count is *intrusive* — it lives inside the object, not in a separate control block — which is why `ref_ptr` is "half size and faster" than `std::shared_ptr` (core/ref_ptr.h:18-19). The destructor `~Object()` is `protected` and `virtual` (core/Object.h:182), so stack-allocating an `Object` subclass or calling `delete` on one is a compile error by design: objects must be heap-allocated through `create()` and owned by `ref_ptr`. `_attemptDelete()` itself defers to an optional `Auxiliary`: if no auxiliary is attached it `delete this`, otherwise it asks the auxiliary whether deletion is appropriate (src/vsg/core/Object.cpp:90-99) — this is the hook that makes `observer_ptr` (weak references) work.

Concrete classes do not write the boilerplate above themselves; they inherit through `vsg::Inherit<ParentClass, Subclass>` (core/Inherit.h:26-27), a Curiously Recurring Template Pattern base. `Inherit` perfect-forwards constructor arguments to the parent (core/Inherit.h:30-32) and synthesizes the static factory `create(...)` returning `ref_ptr<Subclass>(new Subclass(...))` (core/Inherit.h:34-38), plus `sizeofObject()`, `className()`, `type_info()` and `is_compatible()` overrides (core/Inherit.h:47-50). `is_compatible` walks the inheritance chain by OR-ing the subclass `typeid` against the parent's `is_compatible` (core/Inherit.h:50), which powers VSG's own RTTI: `Object::cast<T>()` uses `is_compatible(typeid(T))` for a `static_cast` fast path when dynamic_cast is disabled (core/Object.h:95-99).

The traversal mechanism is classic double dispatch. `Inherit` overrides `accept(Visitor&)` to call `visitor.apply(static_cast<Subclass&>(*this))` (core/Inherit.h:70) — the static_cast to the *most-derived* type is what selects the correct `apply` overload on the visitor at compile time. So `node->accept(myVisitor)` resolves to `myVisitor.apply(ConcreteType&)`. The visitor's `apply` then typically calls `node.traverse(*this)` to descend into children, giving the `accept -> apply -> traverse` cycle. `Visitor` declares a large family of virtual `apply(T&)` overloads (core/Visitor.h:193-219), and `ConstVisitor` mirrors them with `const` references for read-only traversal (core/ConstVisitor.h:193-200). `Object` itself provides the three `accept` entry points (mutable `Visitor`, `const ConstVisitor`, `const RecordTraversal`) and matching no-op `traverse` defaults (core/Object.h:109-116).

`Data` is the abstract base for anything destined for the GPU (core/Data.h:114). It carries a `Properties` struct describing `VkFormat`, `stride`, mip levels, block compression, origin and a `DataVariance` hint (core/Data.h:120-140), plus a `ModifiedCount` for change tracking (core/Data.h:235). The two concrete families are `Value<T>` for a single value (core/Value.h:39) and `Array<T>` (plus Array2D/Array3D) for buffers of elements (core/Array.h:35). Both expose the `dataPointer()`/`dataSize()`/`valueCount()` virtuals that the rendering backend reads to upload to Vulkan (core/Data.h:173-191). When you mutate a dynamic `Data`, you must call `dirty()` to bump the modified count so the transfer machinery re-uploads it (core/Data.h:206).

Finally, all of this memory comes from the `Allocator` singleton (core/Allocator.h:41,51). `Object::operator new` routes through `vsg::allocate(count, ALLOCATOR_AFFINITY_OBJECTS)` (src/vsg/core/Object.cpp:236-239) while `Data::operator new` uses `ALLOCATOR_AFFINITY_DATA` (src/vsg/core/Data.cpp:46-48). These *affinities* (core/Allocator.h:31-38) let the allocator pool objects of similar lifetime/locality together for cache coherency. The default singleton is an `IntrusiveAllocator` (src/vsg/core/Allocator.cpp:24-28).

## Key types & calls

- `vsg::Object` — common base providing intrusive ref-counting, RTTI, metadata and visitor entry points (core/Object.h:59). App authors touch: `ref()`/`unref()` only indirectly via `ref_ptr` (core/Object.h:122-127); `cast<T>()` for safe downcasts (core/Object.h:95-99); `setValue`/`getValue`/`setObject`/`getObject` for attaching named metadata (core/Object.h:132-160); `accept(visitor)` to run a traversal (core/Object.h:109).
- `vsg::Inherit<Parent,Self>` — CRTP base every concrete class derives from to get `create()` and RTTI for free (core/Inherit.h:26). Touch: the static `create(args...)` factory returning `ref_ptr<Self>` (core/Inherit.h:34-38) and `create_if(flag, args...)` (core/Inherit.h:40-45).
- `vsg::ref_ptr<T>` — strong owning smart pointer; copying calls `ref()`, destruction calls `unref()` (core/ref_ptr.h:21,29-33,67-70). Touch: `operator->`/`operator*`/`get()` (core/ref_ptr.h:169-173), `valid()`/`operator bool` (core/ref_ptr.h:160-162), `reset()` (core/ref_ptr.h:72-76), `cast<R>()` (core/ref_ptr.h:192-193).
- `vsg::observer_ptr<T>` — non-owning weak pointer that does not keep the object alive (core/observer_ptr.h:23). Touch: construct from a `ref_ptr<>` (core/observer_ptr.h:51-56); `ref_ptr()` or implicit conversion to `ref_ptr<R>` to safely re-acquire a strong reference under lock (core/observer_ptr.h:172-186); `valid()` checks the object is still connected (core/observer_ptr.h:168).
- `vsg::Visitor` / `vsg::ConstVisitor` — base classes you subclass and override `apply(ConcreteType&)` on to operate on a graph (core/Visitor.h:178, core/ConstVisitor.h:178). Touch: override the `apply()` overloads (core/Visitor.h:193-219); call `node.traverse(*this)` inside `apply` to descend; `traversalMask`/`overrideMask` to filter (core/Visitor.h:188-189).
- `vsg::Data` — abstract GPU-data base with `Properties` (format/stride/variance) and `dirty()` change tracking (core/Data.h:114,120,206). Touch: `properties` field (core/Data.h:169), `dirty()` (core/Data.h:206), `dataPointer()`/`dataSize()`/`valueCount()` (core/Data.h:173-180).
- `vsg::Value<T>` — wraps a single value as `Data` (core/Value.h:39). Touch: `create(args...)` (core/Value.h:55-59), `value()`/`set()`/implicit `value_type&` conversion (core/Value.h:156-162). Aliases like `vsg::floatValue`, `vsg::vec3Value`, `vsg::mat4Value` (core/Value.h:213,217,254).
- `vsg::Array<T>` — a strided 1D buffer as `Data`, the workhorse for vertices/indices (core/Array.h:35). Touch: `create(numElements, ...)` / `create({...})` initializer-list / `create(numElements, value)` (core/Array.h:115-138), `operator[]`/`at()`/`set()`/`data()` (core/Array.h:329-335), `size()` (core/Array.h:214), `begin()`/`end()` (core/Array.h:340-344). Aliases like `vsg::vec3Array`, `vsg::uintArray`, `vsg::floatArray` (core/Array.h:402,394,395).
- `vsg::Allocator` — CPU-memory allocator singleton with per-affinity pools (core/Allocator.h:41). Touch: rarely directly; `Allocator::instance()` (core/Allocator.h:51), and the affinity enums `ALLOCATOR_AFFINITY_OBJECTS`/`_DATA`/`_NODES` (core/Allocator.h:31-38).

## Idiomatic wiring

`create()` is the only construction path. `Inherit::create` builds the object with `new Subclass(...)` and immediately wraps it in a `ref_ptr<Subclass>`, whose constructor takes the initial reference (core/Inherit.h:34-38; the `ref_ptr(T*)` ctor calls `_ptr->ref()` at core/ref_ptr.h:50-54). Object memory comes from the affinity-tagged allocator — `Object::operator new` calls `vsg::allocate(count, ALLOCATOR_AFFINITY_OBJECTS)` (src/vsg/core/Object.cpp:236-239), and `Data::operator new` uses `ALLOCATOR_AFFINITY_DATA` (src/vsg/core/Data.cpp:46-48), both resolving to `Allocator::instance()->allocate(...)` (src/vsg/core/Allocator.cpp:34-37). When the last `ref_ptr` is destroyed, `~ref_ptr` calls `unref()` (core/ref_ptr.h:67-70), the atomic count reaches zero, and `unref()` invokes `_attemptDelete()` which `delete this` unless an `Auxiliary` vetoes it (src/vsg/core/Object.cpp:90-99). `Array`'s own element storage likewise comes from `vsg::allocate(..., ALLOCATOR_AFFINITY_DATA)` by default (core/Array.h:360-361) and is returned via `vsg::deallocate` on destruction (core/Array.h:364-373).

```cpp
#include <vsg/core/Array.h>
#include <vsg/core/Value.h>

// Construct via create() -> ref_ptr, never `new`.
auto vertices = vsg::vec3Array::create({
    {-0.5f, -0.5f, 0.0f},
    { 0.5f, -0.5f, 0.0f},
    { 0.0f,  0.5f, 0.0f}
}); // ref_ptr<vsg::vec3Array>, storage from ALLOCATOR_AFFINITY_DATA

// A single uniform-style value.
auto scale = vsg::floatValue::create(2.0f); // ref_ptr<vsg::floatValue>

// Mutate then signal the change so the transfer machinery re-uploads.
vertices->at(0) = vsg::vec3{-0.6f, -0.5f, 0.0f};
vertices->dirty(); // core/Data.h:206

// Safe downcast through VSG RTTI.
vsg::ref_ptr<vsg::Data> asData = vertices;
if (auto arr = asData.cast<vsg::vec3Array>()) {
    auto n = arr->size(); // core/Array.h:214
}
```

To break ownership cycles (e.g. a child that wants to refer back to a parent), hold the back-reference as an `observer_ptr` and re-acquire a `ref_ptr` only when you need to use it:

```cpp
vsg::observer_ptr<vsg::Object> weakParent(parentRefPtr); // core/observer_ptr.h:51
// later, safely promote to a strong ref under the auxiliary's lock:
if (vsg::ref_ptr<vsg::Object> parent = weakParent) { // core/observer_ptr.h:176-186
    parent->accept(myVisitor);
}
```

## Rules

- Construct every VSG object with `ClassName::create(...)` and store the result in a `ref_ptr<>`; never use raw `new` or `delete` (core/Inherit.h:34-38, core/Object.h:182).
- Derive any new persistent class from `vsg::Inherit<ParentClass, YourClass>` so it gets `create()`, RTTI and `accept()` automatically (core/Inherit.h:26-50).
- Hold owning references as `ref_ptr<T>` and back-references that could form a cycle as `observer_ptr<T>`, since `ref_ptr` keeps the object alive but `observer_ptr` does not (core/ref_ptr.h:29-33, core/observer_ptr.h:23).
- Promote an `observer_ptr` to a `ref_ptr` before dereferencing it, and check the result for validity, because the target may already have been deleted (core/observer_ptr.h:176-186,168).
- Use `object.cast<T>()` or `ref_ptr::cast<R>()` for downcasts rather than C++ `dynamic_cast`, to use VSG's `is_compatible` RTTI (core/Object.h:95-99, core/ref_ptr.h:192-193).
- In a custom visitor, override the most-derived `apply(ConcreteType&)` overload and call `node.traverse(*this)` to continue descending (core/Visitor.h:193-219, core/Inherit.h:70).
- Call `dirty()` on a `Data`/`Value`/`Array` after modifying its contents so the modified count advances and the GPU copy is refreshed (core/Data.h:206).
- Treat the `protected virtual ~Object()` as off-limits — let `unref()`/`_attemptDelete()` manage destruction (core/Object.h:182, src/vsg/core/Object.cpp:84-100).
- Prefer the `ref_ptr<>` interface over `observer_ptr::get()` for accessing the raw pointer, since `get()` does not protect against the object being deleted (core/observer_ptr.h:188-189).

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `auto* n = new vsg::Group();` | `auto n = vsg::Group::create();` | `~Object()` is protected and `new` bypasses the affinity allocator; `create()` returns a `ref_ptr` and uses `Object::operator new` (core/Object.h:182, core/Inherit.h:34-38). |
| `vsg::Group g;` (stack) | `auto g = vsg::Group::create();` | Stack/value construction is impossible with a protected virtual destructor; objects must be heap-owned by `ref_ptr` (core/Object.h:182). |
| Storing a parent back-pointer as `ref_ptr` | Store it as `observer_ptr<>` and promote on use | A `ref_ptr` cycle keeps both objects alive forever (leak); `observer_ptr` is non-owning (core/observer_ptr.h:23, core/ref_ptr.h:29-33). |
| `dynamic_cast<vsg::Array<vsg::vec3>*>(p)` | `p->cast<vsg::vec3Array>()` | VSG provides `cast<T>()`/`is_compatible` that works even when dynamic_cast is compiled out (core/Object.h:95-99). |
| Editing `array->at(i)` then rendering | Edit then call `array->dirty()` | Without `dirty()` the `ModifiedCount` is unchanged and the transfer machinery will not re-upload (core/Data.h:206-221). |
| Calling `weak.get()->doThing()` on an `observer_ptr` | `if (auto s = weak.ref_ptr()) s->doThing();` | `get()` does not verify the object is still alive; converting to `ref_ptr` locks and checks the connected object (core/observer_ptr.h:168,176-189). |

## Source references

- include/vsg/core/Object.h — `Object` base, ref counting, RTTI/`cast`, metadata, accept/traverse.
- include/vsg/core/Inherit.h — `Inherit<Parent,Self>` CRTP: `create`, `sizeofObject`, `className`, `type_info`, `is_compatible`, `accept`.
- include/vsg/core/ref_ptr.h — strong intrusive smart pointer semantics.
- include/vsg/core/observer_ptr.h — weak pointer and safe promotion to `ref_ptr`.
- include/vsg/core/Visitor.h — `Visitor` apply-overload family and double-dispatch contract.
- include/vsg/core/ConstVisitor.h — const-correct visitor mirror.
- include/vsg/core/Data.h — `Data` base, `Properties`, `DataVariance`, `dirty()`/`ModifiedCount`.
- include/vsg/core/Value.h — `Value<T>` single-value container and typedefs.
- include/vsg/core/Array.h — `Array<T>` strided buffer, accessors, allocation/typedefs.
- include/vsg/core/type_name.h — compile-time type names used by `className()`/RTTI.
- include/vsg/core/Allocator.h — allocator singleton, affinities, `vsg::allocate`/`deallocate`.
- src/vsg/core/Object.cpp — `_attemptDelete`, `operator new`/`delete` wiring to the allocator.
- src/vsg/core/Data.cpp — `Data::operator new`/`delete` with `ALLOCATOR_AFFINITY_DATA`.
- src/vsg/core/Allocator.cpp — `Allocator::instance()` (`IntrusiveAllocator`), `vsg::allocate`/`deallocate`.
