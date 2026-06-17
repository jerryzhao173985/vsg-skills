// builder_primitives — compose a scene of Builder primitives positioned with
// MatrixTransforms, lit by an ambient + directional light.
//
// Demonstrates: vsg::Builder (createBox/createSphere/createCapsule), positioning
// subgraphs with vsg::MatrixTransform + vsg::translate, and adding vsg::AmbientLight /
// vsg::DirectionalLight nodes to the scene graph.
//
// Build: part of examples/CMakeLists.txt. Run: ./builder_primitives
#include <vsg/all.h>

int main(int argc, char** argv)
{
    auto scene = vsg::Group::create();

    // lighting — lights are nodes added to the graph
    auto ambient = vsg::AmbientLight::create();
    ambient->color = vsg::vec3(1.0f, 1.0f, 1.0f);
    ambient->intensity = 0.2f;
    scene->addChild(ambient);

    auto directional = vsg::DirectionalLight::create();
    directional->color = vsg::vec3(1.0f, 1.0f, 1.0f);
    directional->intensity = 0.8f;
    directional->direction = vsg::vec3(0.5f, 1.0f, -1.0f);
    scene->addChild(directional);

    // primitives positioned with MatrixTransforms
    auto builder = vsg::Builder::create();
    vsg::StateInfo stateInfo;

    auto placeAt = [&](vsg::ref_ptr<vsg::Node> node, const vsg::dvec3& position) {
        auto transform = vsg::MatrixTransform::create();
        transform->matrix = vsg::translate(position);
        transform->addChild(node);
        scene->addChild(transform);
    };

    vsg::GeometryInfo box;
    placeAt(builder->createBox(box, stateInfo), vsg::dvec3(-2.0, 0.0, 0.0));

    vsg::GeometryInfo sphere;
    placeAt(builder->createSphere(sphere, stateInfo), vsg::dvec3(0.0, 0.0, 0.0));

    vsg::GeometryInfo capsule;
    placeAt(builder->createCapsule(capsule, stateInfo), vsg::dvec3(2.0, 0.0, 0.0));

    // viewer + camera
    auto traits = vsg::WindowTraits::create();
    traits->windowTitle = "builder_primitives";
    auto window = vsg::Window::create(traits);
    if (!window) { vsg::fatal("Could not create window."); return 1; }

    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    double aspect = static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height);
    auto perspective = vsg::Perspective::create(60.0, aspect, 0.1, 100.0);
    auto lookAt = vsg::LookAt::create(vsg::dvec3(0.0, -8.0, 3.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
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

    (void)argc; (void)argv;
    return 0;
}
