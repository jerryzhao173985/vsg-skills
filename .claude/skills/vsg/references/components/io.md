# I/O & Serialization

The `vsg/io` module is how an app author loads assets into the scene graph and saves them back out. The core entry points are the free functions `vsg::read` / `vsg::read_cast<T>` (loading objects from a file, stream, or memory block) and `vsg::write` (saving an object). Behavior is configured through an `vsg::Options` object that carries search paths, format hints, and the list of `vsg::ReaderWriter` plugins that actually parse each format. VSG ships a native ReaderWriter (`vsg::VSG`) for its own `.vsgt` (ascii) and `.vsgb` (binary) formats, backed by the `vsg::ObjectFactory` singleton that maps serialized type names to constructors. The module also provides the `vsg::Path` filename abstraction, filesystem helper functions, and the thread-safe `vsg::Logger` used throughout the library.

## Public includes

```cpp
#include <vsg/all.h> // umbrella header, pulls in everything below

// or the specific headers:
#include <vsg/io/read.h>         // vsg::read, vsg::read_cast<T>
#include <vsg/io/write.h>        // vsg::write
#include <vsg/io/Options.h>      // vsg::Options
#include <vsg/io/ReaderWriter.h> // vsg::ReaderWriter, vsg::CompositeReaderWriter
#include <vsg/io/ObjectFactory.h>// vsg::ObjectFactory
#include <vsg/io/Logger.h>       // vsg::info/warn/error/fatal/debug/log, vsg::Logger
#include <vsg/io/Path.h>         // vsg::Path, vsg::Paths, path helpers
#include <vsg/io/FileSystem.h>   // vsg::findFile, vsg::fileExists, vsg::makeDirectory, ...
#include <vsg/io/VSG.h>          // vsg::VSG native .vsgt/.vsgb ReaderWriter
```

## Key classes

### vsg::read / vsg::read_cast

Convenience free functions for reading objects from a file, stream, or memory block (io/read.h:23).

- `ref_ptr<Object> read(const Path& filename, ref_ptr<const Options> options = {})` reads a single object from a file (io/read.h:24).
- `PathObjects read(const Paths& filenames, ref_ptr<const Options> options = {})` reads multiple files, returning a `std::map<Path, ref_ptr<Object>>` (io/read.h:27, io/Path.h:215).
- `ref_ptr<Object> read(std::istream& fin, ref_ptr<const Options> options = {})` reads from a stream (io/read.h:30).
- `ref_ptr<Object> read(const uint8_t* ptr, size_t size, ref_ptr<const Options> options = {})` reads from a memory block (io/read.h:33).
- `template<class T> ref_ptr<T> read_cast(const Path& filename, ref_ptr<const Options> options = {})` reads and `dynamic_cast`s to `T`, returning a null `ref_ptr<T>` if the type does not match (io/read.h:36-41). Stream and memory overloads also exist (io/read.h:44-57).

### vsg::write

Convenience free function for writing one object to a file (io/write.h:22).

- `bool write(ref_ptr<Object> object, const Path& filename, ref_ptr<const Options> options = {})` returns true on success; the file extension (e.g. `.vsgt` / `.vsgb`) selects the ReaderWriter (io/write.h:23).

### vsg::Options

Carries IO configuration that is passed into every `vsg::read`/`vsg::write` call (io/Options.h:35-36). Construct with `vsg::Options::create(...)`.

- Variadic constructor: `Options::create(arg1, arg2, ...)` calls `add(arg)` for each argument, so you can pass `ReaderWriter` instances directly (io/Options.h:42-46, 53).
- `void add(ref_ptr<ReaderWriter> rw = {})` / `void add(const ReaderWriters& rws)` register ReaderWriter plugins (io/Options.h:53-54).
- `ReaderWriters readerWriters` is the list of plugins consulted when reading/writing (io/Options.h:57).
- `Paths paths` is the search path list used by `vsg::findFile` and loaders (io/Options.h:69).
- `FindFileHint checkFilenameHint` controls whether the original filename is checked before/after/instead of `paths` (io/Options.h:61-67).
- `Path fileCache` location for a file cache; `Path extensionHint` forces a format when reading streams/memory (io/Options.h:74, 76).
- `ref_ptr<SharedObjects> sharedObjects` enables sharing of loaded objects to avoid duplication (io/Options.h:56).
- `CoordinateConvention sceneCoordinateConvention` defaults to `Z_UP` (io/Options.h:80).
- `std::map<std::string, ref_ptr<ShaderSet>> shaderSets` overrides the standard `"pbr"`/`"phong"`/`"flat"`/`"text"` shader sets used by loaders (io/Options.h:85-91).
- `virtual bool readOptions(CommandLine& arguments)` populates the Options from command-line arguments (io/Options.h:51).

### vsg::ReaderWriter

Abstract base class for format/protocol plugins; subclass and register on `Options` to add new formats (io/ReaderWriter.h:33-34). Create concrete ones via their `::create()`.

- `virtual ref_ptr<Object> read(const Path& filename, ref_ptr<const Options> = {}) const` returns the object, a null `ref_ptr` if the format is unsupported, or a `vsg::ReadError` on failure (io/ReaderWriter.h:64-65, 22-24).
- Stream and memory `read` overloads have the same contract (io/ReaderWriter.h:67-71).
- `virtual bool write(const Object* object, const Path& filename, ref_ptr<const Options> = {}) const` returns true on success (io/ReaderWriter.h:73-74); a stream overload also exists (io/ReaderWriter.h:77).
- `template<class T> ref_ptr<T> read_cast(...)` member convenience for casting (io/ReaderWriter.h:41-46).
- `enum FeatureMask { READ_FILENAME, READ_ISTREAM, READ_MEMORY, WRITE_FILENAME, WRITE_OSTREAM }` and `virtual bool getFeatures(Features&) const` describe what a ReaderWriter supports (io/ReaderWriter.h:82-99).
- `vsg::CompositeReaderWriter` aggregates several ReaderWriters, trying each in turn (io/ReaderWriter.h:103-104).

### vsg::ObjectFactory

Singleton that creates VSG object instances from a `namespace::class` type-name string; used by the native VSG ReaderWriter during deserialization (io/ObjectFactory.h:24-26).

- `static ref_ptr<ObjectFactory>& instance()` returns the singleton (io/ObjectFactory.h:46).
- `virtual ref_ptr<Object> create(const std::string& className)` constructs an object by name (io/ObjectFactory.h:31).
- `template<class T> void add()` registers `T` keyed by `type_name<T>()`, calling `T::create()` (io/ObjectFactory.h:39-43).
- `RegisterWithObjectFactoryProxy<T>` is a helper struct that registers `T` on construction (io/ObjectFactory.h:55-62).

### vsg::Logger

Thread-safe singleton logger; the free functions are the everyday entry points (io/Logger.h:25-26). Most code calls the namespace-level helpers rather than the class directly.

- `template<typename... Args> void vsg::debug/info/warn/error/fatal(Args&&...)` stream-format and log to the current logger, e.g. `vsg::info("size = ", n)` (io/Logger.h:255-312).
- `template<typename... Args> void vsg::log(Logger::Level, Args&&...)` logs at a chosen level (io/Logger.h:321-325).
- `enum Logger::Level { LOGGER_ALL, LOGGER_DEBUG, LOGGER_INFO, LOGGER_WARN, LOGGER_ERROR, LOGGER_FATAL, LOGGER_OFF }` and `Level level` set the threshold (io/Logger.h:33-44).
- `static ref_ptr<Logger>& instance()` defaults to `vsg::StdLogger`; assign a different logger to change behavior (io/Logger.h:46-47).
- `vsg::StdLogger`, `vsg::ThreadLogger`, and `vsg::NullLogger` are ready-made implementations (io/Logger.h:334, 359, 391).

### vsg::Path

Filename/path abstraction with wide and narrow string support, used everywhere a filename is needed (io/Path.h:31-33). Implicitly constructible from `const char*`, `std::string`, `std::wstring`.

- `Path() / Path(const std::string&) / Path(const char*) / ...` constructors (io/Path.h:63-68).
- `std::string string() const` and `const value_type* c_str() const` convert out (io/Path.h:110-125).
- `Path& operator/=(const Path&)` and `operator/` join with a separator; `concat`/`operator+` join without one (io/Path.h:160-163, 143-157, 201-211).
- `explicit operator bool()` / `bool empty()` test for non-empty (io/Path.h:105-106).
- `FileType type() const` reports `FILE_NOT_FOUND` / `REGULAR_FILE` / `DIRECTORY` (io/Path.h:191, 24-29).
- Helpers: `filePath`, `fileExtension`, `lowerCaseFileExtension`, `simpleFilename`, `removeExtension` (io/Path.h:218-234). `using Paths = std::vector<Path>` (io/Path.h:214).

### FileSystem helpers (vsg::findFile, etc.)

Free functions for filesystem queries (io/FileSystem.h:22-67).

- `Path findFile(const Path& filename, const Paths& paths)` / `findFile(const Path&, const Options*)` resolve a filename against search paths (io/FileSystem.h:51-55).
- `bool fileExists(const Path& path)` and `FileType fileType(const Path& path)` test existence/type (io/FileSystem.h:45-48).
- `bool makeDirectory(const Path& path)` creates a directory tree (io/FileSystem.h:58).
- `Paths getDirectoryContents(const Path&)`, `Path executableFilePath()`, `FILE* fopen(const Path&, const char*)` (io/FileSystem.h:61-67).
- `std::string getEnv(const char*)` / `Paths getEnvPaths(const char*)` read environment variables, the latter splitting on the platform delimiter (io/FileSystem.h:28-32).

### vsg::VSG (native .vsgt/.vsgb ReaderWriter)

The ReaderWriter for VSG's own ascii (`.vsgt`) and binary (`.vsgb`) formats (io/VSG.h:22-23). Usually invoked implicitly by `vsg::read`/`vsg::write` via the file extension; create with `vsg::VSG::create()`.

- Implements the `read`/`write` file, stream, and memory overrides (io/VSG.h:28-33).
- `enum FormatType { BINARY, ASCII, NOT_RECOGNIZED }` and `FormatInfo readHeader(std::istream&)` identify a stream's format (io/VSG.h:40-49).
- `ObjectFactory* getObjectFactory()` exposes the factory used for deserialization (io/VSG.h:37-38).

## Idioms

Load a scene graph node, casting to the expected type:

```cpp
#include <vsg/all.h>

auto options = vsg::Options::create();
options->paths = vsg::getEnvPaths("VSG_FILE_PATH"); // search paths from env

if (auto scene = vsg::read_cast<vsg::Node>("models/teapot.vsgt", options))
{
    vsg::info("Loaded scene with ", scene->getNumChildren(), " children");
}
else
{
    vsg::warn("Could not load models/teapot.vsgt");
}
```

Write a scene graph out in native binary format:

```cpp
#include <vsg/all.h>

vsg::ref_ptr<vsg::Node> scene = buildScene();
if (!vsg::write(scene, "out.vsgb"))
{
    vsg::error("Failed to write out.vsgb");
}
```

Resolve a filename against search paths before using it:

```cpp
#include <vsg/all.h>

auto options = vsg::Options::create();
options->paths.push_back("/usr/share/myapp/assets");

vsg::Path found = vsg::findFile("texture.png", options);
if (found) vsg::info("Resolved to ", found.string());
```

## Common mistakes

| Bad | Good | Why |
| --- | --- | --- |
| `auto n = vsg::read("scene.vsgt"); n->traverse(...);` assuming a `vsg::Node` | `auto n = vsg::read_cast<vsg::Node>("scene.vsgt")` | `read` returns `ref_ptr<Object>`; `read_cast<T>` does the `dynamic_cast` and yields null on type mismatch (io/read.h:36-41). |
| `if (vsg::read("x.vsgt")) { /* assume success */ }` without null-check on the result you store | Check the returned `ref_ptr` for null before dereferencing | `read` returns a null `ref_ptr` when the format is unsupported or the file is missing (io/ReaderWriter.h:64-65). |
| `std::cout << "loaded\n";` for library diagnostics | `vsg::info("loaded")` | Routes through the thread-safe singleton Logger and respects the configured level (io/Logger.h:269-273, 44). |
| `vsg::read(stream)` for a raw `.vsgb` byte stream with no hint | Set `options->extensionHint = ".vsgb"` then read | Stream/memory reads have no filename to infer the format from; `extensionHint` selects the ReaderWriter (io/Options.h:76). |

## Things to never invent

- There is no `vsg::load` or `vsg::save`; the functions are `vsg::read` and `vsg::write` (io/read.h:24, io/write.h:23).
- `vsg::write` does not take a `std::ostream` overload at the free-function level — only `(object, Path, options)` exists (io/write.h:23). Stream writing is on `ReaderWriter::write` (io/ReaderWriter.h:77).
- `Options` has no public setter methods for `paths`/`readerWriters`; assign the public members directly or pass ReaderWriters to the variadic constructor (io/Options.h:57, 69, 42-46).
- Do not construct any of these with `new`; use the static `::create(...)` factory from `vsg::Inherit` (e.g. `Options::create()`, `VSG::create()`) (io/Options.h:36, io/VSG.h:23).

## Source references

- include/vsg/io/read.h
- include/vsg/io/write.h
- include/vsg/io/Options.h
- include/vsg/io/ReaderWriter.h
- include/vsg/io/ObjectFactory.h
- include/vsg/io/Logger.h
- include/vsg/io/Path.h
- include/vsg/io/FileSystem.h
- include/vsg/io/VSG.h
