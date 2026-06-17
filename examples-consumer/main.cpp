// vsg_consumer_test — a realistic minimal VulkanSceneGraph app, written by following
// ONLY the .claude/skills/vsg guidance (SKILL.md + references/patterns.md + components).
// It is a dogfood test: every API form below is used exactly as the skill documents it.
//
// Exercises: CommandLine, viewer/window setup, camera setup, command graph creation,
// scene-graph structure (Group hierarchy), transforms (translate/rotate/scale),
// state + Builder usage (GeometryInfo/StateInfo), and model loading from a local asset.
//
// Build: cmake + find_package(vsg) (see CMakeLists.txt). Run needs a display; build does not.
#include <vsg/all.h>

#include <iostream>

int main(int argc, char** argv)
{
    // --- command line (utils/CommandLine.h via patterns.md "Loading models"/utils idioms) ---
    vsg::CommandLine arguments(&argc, argv);
    auto numFrames = arguments.value<int>(-1, "--frames");
    bool fullscreen = arguments.read({"--fullscreen", "-f"});
    auto modelFile = arguments.value<vsg::Path>("teapot.vsgt", "--model");  // pass --model /path/to/model.vsgt
    if (arguments.errors()) return arguments.writeErrorMessages(std::cerr);

    // --- window + viewer (patterns.md "Minimal application skeleton") ---
    auto traits = vsg::WindowTraits::create("VSG Consumer Test");
    traits->width = 1280;
    traits->height = 720;
    traits->fullscreen = fullscreen;

    auto viewer = vsg::Viewer::create();
    auto window = vsg::Window::create(traits);
    if (!window)
    {
        std::cerr << "Could not create window.\n";
        return 1;
    }
    viewer->addWindow(window);

    // --- scene graph structure: a Group hierarchy (patterns.md "Building a scene graph") ---
    auto scene = vsg::Group::create();
    auto primitives = vsg::Group::create();
    scene->addChild(primitives);

    // --- state + Builder usage (utils/Builder.h; GeometryInfo/StateInfo per components/utils.md) ---
    auto builder = vsg::Builder::create();
    vsg::StateInfo state;
    state.lighting = true;

    auto addPrimitive = [&](vsg::ref_ptr<vsg::Node> node, double x, const vsg::vec4& /*color*/) {
        auto transform = vsg::MatrixTransform::create();   // transforms: translate (patterns.md)
        transform->matrix = vsg::translate(x, 0.0, 0.0);
        transform->addChild(node);
        primitives->addChild(transform);
    };

    {
        vsg::GeometryInfo geom;
        geom.position = vsg::vec3(0.0f, 0.0f, 0.0f);
        geom.color = vsg::vec4(1.0f, 0.2f, 0.2f, 1.0f);
        addPrimitive(builder->createBox(geom, state), -2.5, geom.color);
    }
    {
        vsg::GeometryInfo geom;
        geom.color = vsg::vec4(0.2f, 1.0f, 0.2f, 1.0f);
        addPrimitive(builder->createSphere(geom, state), 0.0, geom.color);
    }
    {
        vsg::GeometryInfo geom;
        geom.color = vsg::vec4(0.2f, 0.4f, 1.0f, 1.0f);
        addPrimitive(builder->createCapsule(geom, state), 2.5, geom.color);
    }

    // --- model loading from a local asset (patterns.md "Loading models") ---
    auto options = vsg::Options::create();
    if (auto model = vsg::read_cast<vsg::Node>(modelFile, options))
    {
        auto transform = vsg::MatrixTransform::create();
        transform->matrix = vsg::translate(0.0, 0.0, 2.5) * vsg::scale(0.5, 0.5, 0.5);
        transform->addChild(model);
        scene->addChild(transform);
        std::cout << "Loaded model: " << modelFile << "\n";
    }
    else
    {
        std::cout << "No model loaded (missing or wrong type): " << modelFile << "\n";
    }

    // --- frame the camera on the scene bounds (components/utils.md ComputeBounds idiom) ---
    auto computeBounds = vsg::ComputeBounds::create();
    scene->accept(*computeBounds);
    vsg::dvec3 centre = (computeBounds->bounds.min + computeBounds->bounds.max) * 0.5;
    double radius = vsg::length(computeBounds->bounds.max - computeBounds->bounds.min) * 0.5;
    if (radius <= 0.0) radius = 1.0;

    // --- camera setup (patterns.md "Camera & interaction": 2-arg Camera::create) ---
    double aspect = static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height);
    auto perspective = vsg::Perspective::create(30.0, aspect, radius * 0.001, radius * 10.0);
    auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -radius * 3.5, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
    auto camera = vsg::Camera::create(perspective, lookAt);

    // --- command graph creation + viewer wiring (patterns.md skeleton) ---
    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    viewer->addEventHandler(vsg::Trackball::create(camera));
    viewer->compile();

    // --- render loop ---
    int frame = 0;
    while (viewer->advanceToNextFrame() && (numFrames < 0 || frame++ < numFrames))
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }
    return 0;
}
