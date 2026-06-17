# Threading

VSG's threading module provides the low-level concurrency primitives that back multi-threaded command recording and asynchronous work in a VSG application. Most app authors never touch these directly: the one switch you call is `Viewer::setupThreading()`, which spins up per-task recording threads. You reach for the primitives below — `OperationThreads`/`OperationQueue` for a worker-thread pool, `Barrier`/`Latch`/`FrameBlock` for cross-thread synchronization, `ActivityStatus` for cooperative shutdown, and `Affinity` for pinning threads to CPUs — when you build your own background workers (e.g. asynchronous database paging or off-thread compilation) or customize the viewer's frame loop.

## Public includes

```cpp
#include <vsg/all.h> // pulls in everything below

// or target the specific headers:
#include <vsg/threading/OperationThreads.h> // also brings in OperationQueue.h
#include <vsg/threading/OperationQueue.h>   // Operation, OperationQueue, ThreadSafeQueue, InsertionPosition
#include <vsg/threading/Barrier.h>
#include <vsg/threading/Latch.h>
#include <vsg/threading/FrameBlock.h>
#include <vsg/threading/ActivityStatus.h>
#include <vsg/threading/Affinity.h>

#include <vsg/app/Viewer.h> // setupThreading() lives here
```

`OperationThreads.h` includes `OperationQueue.h` (threading/OperationThreads.h:15), and `OperationQueue.h` includes `ActivityStatus.h` and `Latch.h` (threading/OperationQueue.h:16-17), so including the worker-pool header transitively gives you the queue, operation base class, and activity status.

## Primary recipe: enable multi-threaded recording

The high-level entry point is `Viewer::setupThreading()` (app/Viewer.h:132); pair it with `Viewer::stopThreading()` (app/Viewer.h:133). The viewer owns an `ActivityStatus` (app/Viewer.h:129) and a `std::list<std::thread>` (app/Viewer.h:130) that these calls start and stop. Call `setupThreading()` once after the viewer's record-and-submit tasks have been assigned, and the per-task recording is distributed across threads.

```cpp
auto viewer = vsg::Viewer::create();
// ... add windows, assign command graphs ...
viewer->assignRecordAndSubmitTaskAndPresentation(commandGraphs);
viewer->compile();

viewer->setupThreading();   // distribute recording across threads (app/Viewer.h:132)

while (viewer->advanceToNextFrame())
{
    viewer->handleEvents();
    viewer->update();
    viewer->recordAndSubmit();
    viewer->present();
}

viewer->stopThreading();    // join the recording threads (app/Viewer.h:133)
```

## Key classes

### vsg::OperationThreads
A pool of `std::thread`s that share one `OperationQueue`; each thread pulls a `vsg::Operation` off the queue and calls its `run()` (threading/OperationThreads.h:22-25).

- `OperationThreads::create(uint32_t numThreads, ref_ptr<ActivityStatus> in_status = {})` — construct a pool of `numThreads` worker threads, optionally sharing an existing `ActivityStatus` (threading/OperationThreads.h:28).
- `add(ref_ptr<Operation> operation, InsertionPosition = INSERT_BACK)` — enqueue one operation; forwards to the shared queue (threading/OperationThreads.h:32-35).
- `add(Iterator begin, Iterator end, InsertionPosition = INSERT_BACK)` — enqueue a range of operations (threading/OperationThreads.h:37-41).
- `run()` — use the calling thread to also consume and run operations until the queue is empty, in parallel with the pool (threading/OperationThreads.h:43-45).
- `stop()` — stop the worker threads (threading/OperationThreads.h:48).
- `ref_ptr<OperationQueue> queue;` — the shared queue (threading/OperationThreads.h:52).
- `ref_ptr<ActivityStatus> status;` — the activity flag the threads watch (threading/OperationThreads.h:53).

### vsg::OperationQueue
A thread-safe queue of `ref_ptr<Operation>`; it is the alias `ThreadSafeQueue<ref_ptr<Operation>>` (threading/OperationQueue.h:152-153). `Operation` is the work item: a struct with a pure-virtual `run()` you override (threading/OperationQueue.h:145-149).

- `OperationQueue::create(ref_ptr<ActivityStatus> status)` — construct, watching the given status (threading/OperationQueue.h:37-40).
- `add(value_type operation, InsertionPosition = INSERT_BACK)` — push to back or front (threading/OperationQueue.h:46-54).
- `take()` — pop the head without blocking; returns an empty `ref_ptr` if none available (threading/OperationQueue.h:96-107).
- `take_when_available()` — pop the head, blocking until one is available or the status is canceled (threading/OperationQueue.h:110-133).
- `take_all()` — swap out and return the whole queue as a `std::list` (threading/OperationQueue.h:85-93).
- `empty()` — true if no operations are queued (threading/OperationQueue.h:78-82).
- `enum InsertionPosition { INSERT_FRONT, INSERT_BACK }` controls where `add` inserts (threading/OperationQueue.h:23-27).

### vsg::Barrier
Synchronizes a fixed number of threads that all release together once that many have arrived (threading/Barrier.h:22-23). Reusable across phases.

- `Barrier::create(uint32_t num_thread)` — construct expecting `num_thread` participants (threading/Barrier.h:26-29).
- `arrive_and_wait()` — increment the arrived count; the last arrival releases all, others block until release (threading/Barrier.h:35-47).
- `arrive_and_drop()` — increment and release if complete, but return immediately without waiting (threading/Barrier.h:50-57).

### vsg::Latch
A countdown latch: threads wait until the count is decremented to zero (threading/Latch.h:23-24).

- `Latch::create(int num)` or `Latch::create(size_t num)` — construct with an initial count (threading/Latch.h:27-31).
- `count_down()` — decrement; returns true and releases waiters when it reaches zero (threading/Latch.h:51-62).
- `count_up()` — increment the count (threading/Latch.h:46-49).
- `wait()` — block until the count reaches zero (threading/Latch.h:69-76).
- `set(int num)` — reset the count; setting to 0 releases waiters (threading/Latch.h:33-44).
- `is_ready()` — true if the count is at or below zero (threading/Latch.h:64-67).
- `count()` — current count (threading/Latch.h:84).

### vsg::FrameBlock
Synchronizes threads waiting on the start of a new frame, carrying the current `FrameStamp` (threading/FrameBlock.h:23-24).

- `FrameBlock::create(ref_ptr<ActivityStatus> status)` — construct watching a status (threading/FrameBlock.h:29-31).
- `set(ref_ptr<FrameStamp> frameStamp)` — publish a new frame stamp and wake all waiters (threading/FrameBlock.h:36-41).
- `get()` — read the current frame stamp (threading/FrameBlock.h:43-47).
- `wait_for_change(ref_ptr<FrameStamp>& value)` — block until the stamp differs from `value` (or status goes inactive), then write the new stamp into `value`; returns whether the status is still active (threading/FrameBlock.h:57-67).
- `wake()` — wake all waiters without changing the value, e.g. to unblock them at shutdown (threading/FrameBlock.h:51-55).
- `active()` — true while the watched status is active (threading/FrameBlock.h:49).

### vsg::ActivityStatus
Atomic flag telling watching threads whether to keep working or exit cleanly (threading/ActivityStatus.h:20-21).

- `ActivityStatus::create(bool active = true)` — construct, active by default (threading/ActivityStatus.h:24-25).
- `set(bool flag)` — atomically change the flag (threading/ActivityStatus.h:27).
- `active()` — true if callers should continue (threading/ActivityStatus.h:30).
- `cancel()` — true if callers should stop; the inverse of `active()` (threading/ActivityStatus.h:33).

### vsg::Affinity
A plain struct holding a set of CPU ids to pin a thread to (threading/Affinity.h:23-24). Note: this is a struct, not a `vsg::Object`, so there is no `create()` — construct it directly.

- `Affinity()` — empty (no affinity) (threading/Affinity.h:26).
- `Affinity(uint32_t cpu, uint32_t num = 1)` — affinity to `num` CPUs starting at `cpu` (threading/Affinity.h:28-31).
- `std::set<uint32_t> cpus;` — the CPU ids (threading/Affinity.h:33).
- `operator bool()` — true if any CPU is set (threading/Affinity.h:35).
- `void vsg::setAffinity(std::thread& thread, const Affinity&)` — pin a given thread (threading/Affinity.h:39).
- `void vsg::setAffinity(const Affinity&)` — pin the current thread; under Linux child threads inherit it (threading/Affinity.h:42-43).

## Idioms

Worker pool driven by custom Operations:

```cpp
struct MyOp : public vsg::Inherit<vsg::Operation, MyOp>
{
    void run() override { /* do background work */ }
};

auto status  = vsg::ActivityStatus::create();                 // threading/ActivityStatus.h:24
auto threads = vsg::OperationThreads::create(4, status);       // threading/OperationThreads.h:28
threads->add(MyOp::create());                                  // threading/OperationThreads.h:32
// ... later, at shutdown:
status->set(false);                                            // threading/ActivityStatus.h:27
threads->stop();                                               // threading/OperationThreads.h:48
```

Wait for a batch of work to finish with a Latch:

```cpp
auto latch = vsg::Latch::create(static_cast<size_t>(items.size())); // threading/Latch.h:30
for (auto& item : items)
    threads->add(WorkOp::create(item, latch)); // each op calls latch->count_down() in run()
latch->wait();                                                  // threading/Latch.h:69
```

Block a thread until the next frame is published:

```cpp
auto frameBlock = vsg::FrameBlock::create(status);              // threading/FrameBlock.h:29
vsg::ref_ptr<vsg::FrameStamp> seen;
while (frameBlock->wait_for_change(seen))                       // threading/FrameBlock.h:57
{
    // 'seen' now holds the newest FrameStamp; do per-frame work
}
```

Release a set of threads together with a Barrier:

```cpp
auto barrier = vsg::Barrier::create(numWorkers + 1);           // threading/Barrier.h:26
// in each worker thread: barrier->arrive_and_wait();          // threading/Barrier.h:35
barrier->arrive_and_wait(); // main thread releases everyone at once
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `new vsg::OperationThreads(4)` | `vsg::OperationThreads::create(4, status)` | All `vsg::Object`-derived types are heap-managed via the `create()` factory returning a `ref_ptr` (threading/OperationThreads.h:28). |
| Calling `threads->stop()` while `status->active()` is still true | `status->set(false);` then `threads->stop();` | Threads block in `take_when_available()` until the watched `ActivityStatus` is canceled (threading/OperationQueue.h:117-127). |
| `auto a = vsg::Affinity::create(0, 4);` | `vsg::Affinity a(0, 4);` | `Affinity` is a plain struct, not a `vsg::Object`; it has no `create()` (threading/Affinity.h:24-31). |
| Reusing a `Latch` after it hits zero without resetting | call `latch->set(num)` to reload before reuse | `count_down()` only releases; the count must be re-established explicitly (threading/Latch.h:33-44, 51-62). |
| Managing recording threads by hand | `viewer->setupThreading()` / `viewer->stopThreading()` | The viewer owns and lifecycles the recording threads for you (app/Viewer.h:132-133). |

## Things to never invent

- `OperationThreads` has no `wait()` or `join()` method — use `stop()` (threading/OperationThreads.h:48) and cancel the `ActivityStatus`.
- `OperationQueue` has no `pop()`/`push()` — the API is `add`, `take`, `take_when_available`, and `take_all` (threading/OperationQueue.h:46-133).
- `Affinity` has no `create()`, no `ref_ptr`, and no `VSG_type_name` — it is a non-Object struct (threading/Affinity.h:24).
- `Barrier` exposes only `arrive_and_wait` and `arrive_and_drop`; there is no `reset()` (threading/Barrier.h:35-57).
- `setupThreading()` takes no arguments — do not pass a thread count (app/Viewer.h:132); `stopThreading()` is likewise argument-free and is invoked automatically by `Viewer::~Viewer()` (app/Viewer.h:133, src/vsg/app/Viewer.cpp:45).

## Source references

- include/vsg/threading/OperationThreads.h
- include/vsg/threading/OperationQueue.h
- include/vsg/threading/Barrier.h
- include/vsg/threading/Latch.h
- include/vsg/threading/FrameBlock.h
- include/vsg/threading/ActivityStatus.h
- include/vsg/threading/Affinity.h
- include/vsg/app/Viewer.h
