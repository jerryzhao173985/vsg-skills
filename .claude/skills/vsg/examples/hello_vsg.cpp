// hello_vsg — the smallest complete VulkanSceneGraph application.
//
// Creates a window, builds a one-box scene with vsg::Builder, frames it with a
// camera + trackball, compiles once, and runs the frame loop. Demonstrates the
// three core idioms: create()/ref_ptr ownership, Group/addChild composition, and
// the compile-then-loop rendering model.
//
// Build: part of examples/CMakeLists.txt (find_package(vsg)). Run: ./hello_vsg
#include <vsg/all.h>

int main(int argc, char** argv)
{
    // 1. scene graph: a single box created by the geometry Builder
    auto scene = vsg::Group::create();

    auto builder = vsg::Builder::create();
    vsg::GeometryInfo geomInfo;
    vsg::StateInfo stateInfo;
    scene->addChild(builder->createBox(geomInfo, stateInfo));

    // 2. window + viewer
    auto traits = vsg::WindowTraits::create();
    traits->windowTitle = "hello_vsg";
    auto window = vsg::Window::create(traits);
    if (!window)
    {
        vsg::fatal("Could not create window.");
        return 1;
    }

    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    // 3. camera framing the scene
    double aspect = static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height);
    auto perspective = vsg::Perspective::create(60.0, aspect, 0.1, 100.0);
    auto lookAt = vsg::LookAt::create(vsg::dvec3(0.0, -4.0, 2.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
    auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    // 4. wire the scene into a command graph and assign it to the viewer
    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    // 5. interaction + compile (allocate GPU resources) before the loop
    viewer->addEventHandler(vsg::Trackball::create(camera));
    viewer->compile();

    // 6. the canonical render loop
    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }

    (void)argc; (void)argv;
    return 0;
}
