// view_model — load a 3D model from the command line and view it.
//
// Demonstrates: vsg::CommandLine, loading with vsg::read_cast<vsg::Node>, framing the
// camera from the scene's bounds via vsg::ComputeBounds, and a trackball viewer.
// Reading native .vsgt/.vsgb works out of the box; other formats require vsgXchange
// to be installed and linked (out of scope for this skill).
//
// Build: part of examples/CMakeLists.txt. Run: ./view_model model.vsgt
#include <vsg/all.h>

int main(int argc, char** argv)
{
    vsg::CommandLine arguments(&argc, argv);
    auto options = vsg::Options::create();

    if (argc < 2)
    {
        vsg::warn("usage: view_model <model.vsgt>");
        return 1;
    }

    auto scene = vsg::read_cast<vsg::Node>(argv[1], options);
    if (!scene)
    {
        vsg::fatal("Could not load model: ", argv[1]);
        return 1;
    }

    // frame the camera from the loaded scene's bounds
    auto computeBounds = vsg::ComputeBounds::create();
    scene->accept(*computeBounds);
    vsg::dbox bounds = computeBounds->bounds;
    vsg::dvec3 centre = (bounds.min + bounds.max) * 0.5;
    double radius = vsg::length(bounds.max - bounds.min) * 0.5;
    if (radius <= 0.0) radius = 1.0;

    auto traits = vsg::WindowTraits::create();
    traits->windowTitle = "view_model";
    auto window = vsg::Window::create(traits);
    if (!window)
    {
        vsg::fatal("Could not create window.");
        return 1;
    }

    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    double aspect = static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height);
    auto perspective = vsg::Perspective::create(30.0, aspect, radius * 0.001, radius * 10.0);
    auto lookAt = vsg::LookAt::create(centre + vsg::dvec3(0.0, -radius * 3.5, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));
    auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    viewer->addEventHandler(vsg::Trackball::create(camera));
    viewer->compile();

    while (viewer->advanceToNextFrame())
    {
        viewer->handleEvents();
        viewer->update();
        viewer->recordAndSubmit();
        viewer->present();
    }
    return 0;
}
