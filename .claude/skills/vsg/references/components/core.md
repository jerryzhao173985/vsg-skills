# Core / Object Model

This module is the foundation every VSG application is built on. It defines the intrusively reference-counted base class `vsg::Object`, the CRTP helper `vsg::Inherit<Parent,Self>` that injects the `create()` factory, `accept()` dispatch and RTTI into every scene-graph class, the smart pointers `ref_ptr` (strong/owning) and `observer_ptr` (weak/non-owning), the `Data`/`Value`/`Array` containers that hold vertex, index, uniform and image data destined for Vulkan, and the `Visitor`/`ConstVisitor` double-dispatch traversal classes. You reach for this module whenever you allocate any VSG object (always via `Class::create(...)`), hold references to it, store CPU data for the GPU, or walk/process a scene graph.

## Public includes

```cpp
#include <vsg/all.h>            // pulls in everything below (simplest for apps)

// or the specific headers:
#include <vsg/core/Object.h>       // vsg::Object base, meta-data, CopyOp
#include <vsg/core/Inherit.h>      // vsg::Inherit<> CRTP base (create/accept/RTTI)
#include <vsg/core/ref_ptr.h>      // vsg::ref_ptr<> strong smart pointer
#include <vsg/core/observer_ptr.h> // vsg::observer_ptr<> weak smart pointer
#include <vsg/core/Data.h>         // vsg::Data base + Properties, DataVariance
#include <vsg/core/Value.h>        // vsg::Value<> + floatValue, vec3Value, etc.
#include <vsg/core/Array.h>        // vsg::Array<> + floatArray, vec3Array, etc.
#include <vsg/core/Visitor.h>      // vsg::Visitor (mutable traversal)
#include <vsg/core/ConstVisitor.h> // vsg::ConstVisitor (const traversal)
#include <vsg/core/visit.h>        // vsg::visit<>() convenience helpers
#include <vsg/core/Allocator.h>    // vsg::Allocator, vsg::allocate/deallocate
```

## Key classes

### vsg::Object
The base class for (almost) all VSG types, providing intrusive reference counting and a meta-data store (core/Object.h:59).
- `static ref_ptr<Object> create()` — factory returning an owning `ref_ptr`; never use `new` directly (core/Object.h:67).
- `void ref()` / `void unref()` — increment/decrement the atomic reference count; `unref()` deletes the object when the count hits zero (core/Object.h:122-126). Normally driven for you by `ref_ptr`.
- `unsigned int referenceCount() const` — current strong reference count (core/Object.h:128).
- `template<class T> T* cast()` — safe down-cast that returns `nullptr` on type mismatch (core/Object.h:89-99).
- `const char* className() const` / `const std::type_info& type_info() const` — runtime type info (core/Object.h:82-85).
- `virtual void accept(Visitor&)` / `accept(ConstVisitor&) const` — entry point for visitor double-dispatch (core/Object.h:109-112).
- `void setValue(const std::string& key, const T& value)` / `bool getValue(const std::string& key, T& value) const` — attach/read typed meta-data by string key (core/Object.h:132-140).
- `void setObject(key, ref_ptr<Object>)` / `Object* getObject(key)` / `ref_ptr<Object> getRefObject(key)` — attach/retrieve arbitrary objects by key (core/Object.h:143-160).
- `virtual ref_ptr<Object> clone(const CopyOp& = {}) const` — clone support (core/Object.h:104).

### vsg::Inherit
CRTP base (`Inherit<ParentClass, Subclass>`) that gives a subclass its `create()`/`create_if()` factories, `accept()` overrides, RTTI methods and a default `compare()` (core/Inherit.h:26-27). Used internally by VSG classes; you derive from it when authoring your own scene-graph node or visitor-visitable type.
- `static ref_ptr<Subclass> create(Args&&... args)` — perfect-forwards to the subclass constructor and returns an owning `ref_ptr` (core/Inherit.h:35-38).
- `static ref_ptr<Subclass> create_if(bool flag, Args&&...)` — creates only when `flag` is true, else returns an empty `ref_ptr` (core/Inherit.h:41-45).
- Auto-generated `sizeofObject()`, `className()`, `type_info()`, `is_compatible()` (core/Inherit.h:47-50).
- Auto-generated `accept(Visitor&)` / `accept(ConstVisitor&)` that call `visitor.apply(static_cast<Subclass&>(*this))` (core/Inherit.h:70-71).

### vsg::ref_ptr
A strong, owning intrusive smart pointer; broadly like `std::shared_ptr` but half the size and faster because the count lives in the `Object` itself (core/ref_ptr.h:18-21). Holding a `ref_ptr` keeps the object alive; the last one destroyed deletes it.
- Constructs/copies/moves and increments the count automatically (core/ref_ptr.h:29-61).
- `T* operator->()`, `T& operator*()`, `T* get()` — access the pointee (core/ref_ptr.h:169-173).
- `bool valid()` / `explicit operator bool()` — test for non-null (core/ref_ptr.h:160-162).
- `void reset()` — release the held reference and become null (core/ref_ptr.h:72-76).
- `template<class R> ref_ptr<R> cast() const` — type-safe cast to a related type (core/ref_ptr.h:192-193).

### vsg::observer_ptr
A weak, non-owning smart pointer that cooperates with `ref_ptr` and the object's `Auxiliary`, similar to `std::weak_ptr` (core/observer_ptr.h:20-23). It does NOT keep the object alive; use it to break ownership cycles or hold a back-reference.
- Construct from a `ref_ptr<R>` or raw pointer (core/observer_ptr.h:38-56).
- `bool valid()` — true only while the observed object is still alive (core/observer_ptr.h:168).
- `vsg::ref_ptr<T> ref_ptr() const` and `operator vsg::ref_ptr<R>()` — promote to a strong `ref_ptr`, locking the object so it can be safely accessed (returns empty if it was already destroyed) (core/observer_ptr.h:172-186).

### vsg::Data
Abstract base for all CPU-side data blocks — values, vertices, indices, images — that may be uploaded to Vulkan (core/Data.h:112-114). Its `Properties` struct carries the Vulkan format, stride, mip levels and a `DataVariance` hint.
- `Properties properties` — public layout descriptor: `VkFormat format`, `uint32_t stride`, `DataVariance dataVariance`, etc. (core/Data.h:120-169).
- `enum DataVariance { STATIC_DATA, STATIC_DATA_UNREF_AFTER_TRANSFER, DYNAMIC_DATA, DYNAMIC_DATA_TRANSFER_AFTER_RECORD }` — tells VSG how/when the data changes so it can manage GPU transfers (core/Data.h:59-65).
- `void dirty()` — mark the data modified so it is re-transferred to the GPU; call after editing a `DYNAMIC_DATA` buffer (core/Data.h:206).
- `void* dataPointer()`, `size_t dataSize()`, `size_t valueCount()`, `uint32_t width()/height()/depth()` — raw access and dimensions (core/Data.h:179-191).
- `using DataList = std::vector<ref_ptr<Data>>` — common container of data blocks (core/Data.h:239).

### vsg::Value
`Value<T>` wraps a single value of type `T` as a `Data` object, used for uniforms and meta-data (core/Value.h:38-39). Prefer the predefined typedefs.
- `static ref_ptr<Value> create(Args&&...)` — factory; e.g. `vsg::floatValue::create(1.0f)` (core/Value.h:55-59).
- `value_type& value()` / `void set(const value_type&)` — read/write the wrapped value (core/Value.h:159-162).
- Implicit conversion to `value_type&` (core/Value.h:156-157).
- Predefined typedefs via the `VSG_value` macro: `stringValue`, `boolValue`, `intValue`, `uintValue`, `floatValue`, `doubleValue`, `vec2Value`/`vec3Value`/`vec4Value`, `dvec*Value`, `mat4Value`, `quatValue`, etc. (core/Value.h:206-264).
- Free function `vsg::value<T>(defaultValue, key, objects...)` — read the first matching meta-data value with a fallback (core/Value.h:198-204).

### vsg::Array
`Array<T>` is a contiguous (or strided) 1-D array of `T` backing vertex/index/uniform buffers (core/Array.h:34-35). Prefer the predefined typedefs.
- `static ref_ptr<Array> create(Args&&...)` and `create(std::initializer_list<value_type>)` — factories; the initializer-list form is the usual way to build vertex data (core/Array.h:115-133).
- `value_type& operator[](size_t)`, `value_type& at(size_t)`, `void set(size_t, const value_type&)` — element access (core/Array.h:329-335).
- `size_t size()`, `value_type* data()`, `begin()`/`end()` (stride-aware iterators) (core/Array.h:214, 323, 340-344).
- `void assign(uint32_t numElements, value_type* data, ...)` — adopt an existing buffer (core/Array.h:250-263).
- Predefined typedefs via the `VSG_array` macro: `floatArray`, `uintArray`, `ushortArray`, `vec2Array`/`vec3Array`/`vec4Array`, `ubvec4Array`, `mat4Array`, etc. (core/Array.h:386-437).

### vsg::Visitor
Base class for read/write traversals of a scene graph; double-dispatch via overloadable `apply(Type&)` methods (core/Visitor.h:178-179). Subclass it and override the `apply` overloads for the types you care about.
- `Mask traversalMask = MASK_ALL;` / `Mask overrideMask = MASK_OFF;` — control which masked nodes are visited (core/Visitor.h:188-189).
- `virtual void apply(Object&)` plus type-specific overloads for nodes, values, arrays, commands and events (core/Visitor.h:193-481). Override `apply(SomeType&)` and call the object's `t.traverse(*this)` to descend.

### vsg::ConstVisitor
Identical in shape to `Visitor` but takes objects by `const&`, for read-only traversals (core/ConstVisitor.h:178). Override `apply(const Type&)` overloads (core/ConstVisitor.h:193).
- Same `Mask traversalMask` / `overrideMask` fields (core/ConstVisitor.h:188-189).

### vsg::Allocator
Singleton, extensible CPU allocator that backs `Object`/`Data`/node memory; most apps never touch it directly because `new`/`delete` on VSG objects route through it automatically (core/Allocator.h:40-41).
- `static std::unique_ptr<Allocator>& instance()` — access/replace the global allocator (core/Allocator.h:51).
- `enum AllocatorAffinity { ALLOCATOR_AFFINITY_OBJECTS, ALLOCATOR_AFFINITY_DATA, ALLOCATOR_AFFINITY_NODES, ALLOCATOR_AFFINITY_PHYSICS }` — pools by usage (core/Allocator.h:31-38).
- `enum AllocatorType { ALLOCATOR_TYPE_NO_DELETE, ALLOCATOR_TYPE_NEW_DELETE, ALLOCATOR_TYPE_MALLOC_FREE, ALLOCATOR_TYPE_VSG_ALLOCATOR }` — per-`Data` allocation policy stored in `Data::Properties::allocatorType` (core/Allocator.h:23-29, core/Data.h:136).
- Free helpers `vsg::allocate(size, affinity)` / `vsg::deallocate(ptr, size)` (core/Allocator.h:87-90).

## Idioms

```cpp
// Always construct via create(); the result is an owning ref_ptr.
auto value = vsg::floatValue::create(1.0f);          // core/Value.h:55, core/Value.h:213
value->set(2.0f);                                    // core/Value.h:162
float v = value->value();                            // core/Value.h:160
```

```cpp
// Build a vertex array from an initializer list (typical GPU upload data).
auto vertices = vsg::vec3Array::create({             // core/Array.h:130, core/Array.h:403
    {-0.5f, 0.0f, -0.5f},
    { 0.5f, 0.0f, -0.5f},
    { 0.0f, 0.0f,  0.5f}});
vertices->at(0) = vsg::vec3(0.0f, 0.0f, 0.0f);       // core/Array.h:332

// For data that changes each frame, hint it and dirty() after editing.
vertices->properties.dataVariance = vsg::DYNAMIC_DATA; // core/Data.h:135, core/Data.h:63
vertices->dirty();                                     // core/Data.h:206
```

```cpp
// Strong vs weak references.
vsg::ref_ptr<vsg::Object> owner = vsg::Object::create(); // owns it  (core/Object.h:67)
vsg::observer_ptr<vsg::Object> watcher(owner);           // does not own (core/observer_ptr.h:52)
if (auto locked = watcher.ref_ptr()) {                   // promote safely (core/observer_ptr.h:173)
    locked->setValue("name", std::string("hello"));      // meta-data (core/Object.h:132)
}
```

```cpp
// A read-only visitor that counts vec3Arrays.
struct CountArrays : public vsg::Inherit<vsg::ConstVisitor, CountArrays> { // core/Inherit.h:27
    int count = 0;
    void apply(const vsg::Object& o) override { o.traverse(*this); } // descend (core/ConstVisitor.h:193)
    void apply(const vsg::vec3Array&) override { ++count; }
};
auto counter = vsg::visit<CountArrays>(scene);  // visit.h helper
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `vsg::Group* g = new vsg::Group;` | `auto g = vsg::Group::create();` | VSG objects are intrusively ref-counted; the `create()` factory from `Inherit` returns a `ref_ptr` and routes through the allocator (core/Inherit.h:35-38). |
| Storing a parent back-pointer as `ref_ptr` | Store it as `observer_ptr` | Mutual `ref_ptr`s form a cycle that never reaches refcount 0 and leaks; `observer_ptr` does not own (core/observer_ptr.h:20-23). |
| Editing a `DYNAMIC_DATA` array and expecting the GPU to update | Call `data->dirty()` after editing | `dirty()` bumps the modified count so VSG re-transfers the buffer (core/Data.h:206). |
| `T* raw = ptr.get();` then letting `ptr` go out of scope and using `raw` | Keep a `ref_ptr` for as long as you use the object | Dropping the last `ref_ptr` deletes the object, dangling any raw pointer (core/ref_ptr.h:67-70). |
| `static_cast<Derived*>(obj)` on a base pointer | `obj->cast<Derived>()` or `vsg::cast<Derived>(obj)` | `cast<>()` is type-checked and returns `nullptr` on mismatch (core/Object.h:89-99). |

## Things to never invent

- There is no `make_ref` / `std::make_shared`-style free helper; construction is always `Class::create(...)` (core/Inherit.h:35).
- `ref_ptr` has no `use_count()`; the count lives on the object as `Object::referenceCount()` (core/Object.h:128).
- `observer_ptr` has no `lock()` method (that is `std::weak_ptr`); promote via `ref_ptr()` or the `operator ref_ptr<R>()` conversion (core/observer_ptr.h:172-186).
- `Data` has no public `resize()`; rebuild or use `assign(...)` on `Array` instead (core/Array.h:250-263).
- `Visitor`/`ConstVisitor` dispatch is via overloaded `apply(...)`, not a single templated `visit()` method on the visitor (core/Visitor.h:193).

## Source references

- include/vsg/core/Object.h
- include/vsg/core/Inherit.h
- include/vsg/core/ref_ptr.h
- include/vsg/core/observer_ptr.h
- include/vsg/core/Data.h
- include/vsg/core/Value.h
- include/vsg/core/Array.h
- include/vsg/core/Visitor.h
- include/vsg/core/ConstVisitor.h
- include/vsg/core/visit.h
- include/vsg/core/Allocator.h
