// VSG compile-probe — the validation gate for the `vsg` skill.
//
// This file exercises the public API surface documented in references/. Building it
// against the installed vsg::vsg proves the documented types, signatures and symbols
// are REAL — the C++ equivalent of a design-system typecheck, but stronger: it checks
// signatures and linkage, not just name existence.
//
// It is NOT meant to run: every call is gated behind `if (argc < 0)` (never true), so
// the compiler + linker process every referenced symbol while no window/device is ever
// created. Build with probe/run-probe.sh.
//
// Grounding note: citations in references/ point at the repo headers (master, v1.1.15);
// this probe links the locally INSTALLED vsg::vsg (currently 1.1.14). The documented
// surface is the stable core, identical across that patch delta. A genuine master-only
// symbol that fails here is surfaced as [VERIFY], never silently shipped.

#include <vsg/all.h>

// --- core: object model, smart pointers, data, visitor double-dispatch ---
struct ProbeVisitor : public vsg::Inherit<vsg::Visitor, ProbeVisitor>
{
    void apply(vsg::Object& o) override { o.traverse(*this); }
    void apply(vsg::Group& g) override { g.traverse(*this); }
};

static void probe_core()
{
    auto group = vsg::Group::create();                 // create() -> ref_ptr (Inherit)
    vsg::ref_ptr<vsg::Node> node = group;              // owning smart pointer, upcast
    group->addChild(vsg::Group::create());             // Group::children via addChild
    vsg::observer_ptr<vsg::Group> weak(group);         // non-owning back-reference
    vsg::ref_ptr<vsg::Group> back = weak.ref_ptr();    // re-acquire strong ref

    group->setValue("name", std::string("scene"));     // typed metadata store
    std::string name;
    group->getValue("name", name);

    auto f = vsg::floatValue::create(1.0f);             // Value<T> as Data
    f->value() = 2.0f;
    auto verts = vsg::vec3Array::create(3);             // Array<T> backing a buffer
    (*verts)[0] = vsg::vec3(0.0f, 0.0f, 0.0f);
    verts->dirty();                                     // mark for GPU re-transfer
    (void)verts->size();

    auto visitor = ProbeVisitor::create();              // custom Inherit<Visitor,...>
    group->accept(*visitor);                            // accept -> apply -> traverse
}

// --- maths: header-only vectors/matrices/transforms (compile-only) ---
static void probe_maths()
{
    vsg::vec3 a(1.0f, 2.0f, 3.0f);
    vsg::dvec3 d(1.0, 2.0, 3.0);
    vsg::vec4 c(0.0f, 0.0f, 0.0f, 1.0f);
    vsg::mat4 m;
    vsg::dmat4 dm = vsg::translate(d) * vsg::rotate(1.0, vsg::dvec3(0.0, 0.0, 1.0)) * vsg::scale(2.0, 2.0, 2.0);
    vsg::dmat4 view = vsg::lookAt(vsg::dvec3(0, -4, 2), vsg::dvec3(0, 0, 0), vsg::dvec3(0, 0, 1));
    vsg::dmat4 proj = vsg::perspective(60.0, 1.6, 0.1, 100.0);
    vsg::quat q;
    vsg::dbox box;
    vsg::dsphere sphere;
    (void)a; (void)c; (void)m; (void)dm; (void)view; (void)proj; (void)q; (void)box; (void)sphere;
}

// --- scene-graph: nodes, transforms, state, geometry leaves ---
static void probe_scene_graph()
{
    auto root = vsg::Group::create();

    auto transform = vsg::MatrixTransform::create();
    transform->matrix = vsg::translate(vsg::dvec3(1.0, 0.0, 0.0));
    root->addChild(transform);

    auto stateGroup = vsg::StateGroup::create();
    transform->addChild(stateGroup);

    auto sw = vsg::Switch::create();
    sw->addChild(true, vsg::Group::create());
    root->addChild(sw);

    auto lod = vsg::LOD::create();
    root->addChild(lod);

    auto cull = vsg::CullGroup::create();
    root->addChild(cull);

    vsg::ref_ptr<vsg::PagedLOD> paged = vsg::PagedLOD::create();
    vsg::ref_ptr<vsg::Geometry> geometry = vsg::Geometry::create();
    vsg::ref_ptr<vsg::VertexIndexDraw> vid = vsg::VertexIndexDraw::create();
    stateGroup->addChild(vid);
    (void)paged; (void)geometry;
}

// --- app: window, viewer, camera, render loop (gated; never executed) ---
static void probe_app(int argc, char** argv)
{
    auto scene = vsg::Group::create();

    auto traits = vsg::WindowTraits::create();
    traits->windowTitle = "probe";
    traits->width = 800;
    traits->height = 600;

    auto window = vsg::Window::create(traits);
    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    auto perspective = vsg::Perspective::create(60.0, 1.6, 0.1, 100.0);
    auto lookAt = vsg::LookAt::create(vsg::dvec3(0, -4, 2), vsg::dvec3(0, 0, 0), vsg::dvec3(0, 0, 1));
    auto viewport = vsg::ViewportState::create(window->extent2D());
    auto camera = vsg::Camera::create(perspective, lookAt, viewport);

    auto view = vsg::View::create(camera, scene);
    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    viewer->addEventHandler(vsg::Trackball::create(camera));
    viewer->setupThreading();   // enable multi-threaded recording
    viewer->compile();

    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }
    (void)view; (void)argc; (void)argv;
}

// --- io: load/save, options, logging ---
static void probe_io()
{
    auto options = vsg::Options::create();
    auto loaded = vsg::read_cast<vsg::Node>("model.vsgt", options);
    if (loaded) vsg::write(loaded, "out.vsgt", options);
    vsg::info("probe ", 42);
    vsg::warn("warning ", 1);
}

// --- commands: recordable command objects ---
static void probe_commands()
{
    auto commands = vsg::Commands::create();
    commands->addChild(vsg::Draw::create(3, 1, 0, 0));
    vsg::ref_ptr<vsg::DrawIndexed> di = vsg::DrawIndexed::create(3, 1, 0, 0, 0);
    vsg::ref_ptr<vsg::BindVertexBuffers> bvb = vsg::BindVertexBuffers::create();
    vsg::ref_ptr<vsg::BindIndexBuffer> bib;
    (void)di; (void)bvb; (void)bib;
}

// --- utils: builder, compute bounds, command line ---
static void probe_utils(int argc, char** argv)
{
    auto builder = vsg::Builder::create();
    vsg::GeometryInfo geomInfo;
    vsg::StateInfo stateInfo;
    auto box = builder->createBox(geomInfo, stateInfo);

    auto computeBounds = vsg::ComputeBounds::create();
    box->accept(*computeBounds);
    vsg::dbox bounds = computeBounds->bounds;

    vsg::CommandLine arguments(&argc, argv);
    (void)bounds; (void)arguments;
}

// --- state: pipelines/descriptors/samplers (existence + the easy ctors) ---
static void probe_state()
{
    auto sampler = vsg::Sampler::create();
    vsg::ref_ptr<vsg::GraphicsPipeline> graphicsPipeline;
    vsg::ref_ptr<vsg::ComputePipeline> computePipeline;
    vsg::ref_ptr<vsg::ShaderStage> shaderStage;
    vsg::ref_ptr<vsg::ShaderModule> shaderModule;
    vsg::ref_ptr<vsg::PipelineLayout> pipelineLayout;
    vsg::ref_ptr<vsg::DescriptorSetLayout> descriptorSetLayout;
    vsg::ref_ptr<vsg::DescriptorSet> descriptorSet;
    vsg::ref_ptr<vsg::DescriptorImage> descriptorImage;
    vsg::ref_ptr<vsg::Image> image;
    vsg::ref_ptr<vsg::ImageView> imageView;
    (void)sampler; (void)graphicsPipeline; (void)computePipeline; (void)shaderStage;
    (void)shaderModule; (void)pipelineLayout; (void)descriptorSetLayout;
    (void)descriptorSet; (void)descriptorImage; (void)image; (void)imageView;
}

// --- text / lighting / animation / threading: headline types + easy ctors ---
static void probe_text()
{
    auto text = vsg::Text::create();
    auto textGroup = vsg::TextGroup::create();
    auto layout = vsg::StandardLayout::create();
    vsg::ref_ptr<vsg::Font> font;
    (void)text; (void)textGroup; (void)layout; (void)font;
}

static void probe_lighting()
{
    auto ambient = vsg::AmbientLight::create();
    auto directional = vsg::DirectionalLight::create();
    auto point = vsg::PointLight::create();
    auto spot = vsg::SpotLight::create();
    directional->color = vsg::vec3(1.0f, 1.0f, 1.0f);
    directional->intensity = 1.0f;
    vsg::ref_ptr<vsg::ShadowSettings> shadow;
    (void)ambient; (void)point; (void)spot; (void)shadow;
}

static void probe_animation()
{
    auto animation = vsg::Animation::create();
    vsg::ref_ptr<vsg::AnimationManager> manager;
    vsg::ref_ptr<vsg::TransformSampler> transformSampler = vsg::TransformSampler::create();
    vsg::ref_ptr<vsg::CameraSampler> cameraSampler;
    (void)animation; (void)manager; (void)transformSampler; (void)cameraSampler;
}

static void probe_threading()
{
    vsg::ref_ptr<vsg::OperationThreads> threads;
    vsg::ref_ptr<vsg::OperationQueue> queue;
    vsg::ref_ptr<vsg::Barrier> barrier;
    vsg::ref_ptr<vsg::Latch> latch;
    (void)threads; (void)queue; (void)barrier; (void)latch;
}

int main(int argc, char** argv)
{
    // Never true at runtime (argc >= 1), so no window/device is created — but the
    // compiler and linker still resolve every symbol referenced below.
    if (argc < 0)
    {
        probe_core();
        probe_maths();
        probe_scene_graph();
        probe_app(argc, argv);
        probe_io();
        probe_commands();
        probe_utils(argc, argv);
        probe_state();
        probe_text();
        probe_lighting();
        probe_animation();
        probe_threading();
    }
    return 0;
}
