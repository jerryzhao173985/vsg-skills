// custom_geometry — the HAND-BUILT rendering path (no vsg::Builder).
//
// Own vertex arrays + a ShaderSet + GraphicsPipelineConfigurator + a material
// descriptor -> StateGroup -> VertexIndexDraw leaf. This is the path you take when
// Builder's primitives aren't enough (custom meshes, your own shaders/materials).
//
// Build: part of examples/CMakeLists.txt. Run needs a display; build does not.
#include <vsg/all.h>

#include <iostream>

int main(int argc, char** argv)
{
    auto scene = vsg::Group::create();

    // lights — the Phong shader set needs light to shade the surface
    auto directional = vsg::DirectionalLight::create();
    directional->color = vsg::vec3(1.0f, 1.0f, 1.0f);
    directional->intensity = 0.9f;
    directional->direction = vsg::dvec3(0.2, 1.0, -1.0);
    scene->addChild(directional);
    auto ambient = vsg::AmbientLight::create();
    ambient->intensity = 0.1f;
    scene->addChild(ambient);

    // --- custom pipeline: ShaderSet -> GraphicsPipelineConfigurator ---
    auto shaderSet = vsg::createPhongShaderSet();                        // utils/ShaderSet.h:210
    auto gpConf = vsg::GraphicsPipelineConfigurator::create(shaderSet);  // utils/GraphicsPipelineConfigurator.h:100

    // geometry: a quad in the XY plane
    auto vertices = vsg::vec3Array::create({
        {-0.5f, -0.5f, 0.0f}, {0.5f, -0.5f, 0.0f}, {0.5f, 0.5f, 0.0f}, {-0.5f, 0.5f, 0.0f}});
    auto normals = vsg::vec3Array::create(4, vsg::vec3(0.0f, 0.0f, 1.0f));
    auto texcoords = vsg::vec2Array::create({{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}});
    auto colors = vsg::vec4Value::create(vsg::vec4(1.0f, 1.0f, 1.0f, 1.0f));   // per-instance
    auto indices = vsg::ushortArray::create({0, 1, 2, 2, 3, 0});

    // a material descriptor — set/binding resolved by name (before init)
    auto material = vsg::PhongMaterialValue::create();                  // state/material.h:90
    material->value().diffuse = vsg::vec4(0.8f, 0.2f, 0.2f, 1.0f);
    gpConf->assignDescriptor("material", material);                     // GraphicsPipelineConfigurator.h:119

    // assign vertex arrays by their standard shader names (order preserved)
    vsg::DataList vertexArrays;
    gpConf->assignArray(vertexArrays, "vsg_Vertex",    VK_VERTEX_INPUT_RATE_VERTEX,   vertices);   // .h:118
    gpConf->assignArray(vertexArrays, "vsg_Normal",    VK_VERTEX_INPUT_RATE_VERTEX,   normals);
    gpConf->assignArray(vertexArrays, "vsg_TexCoord0", VK_VERTEX_INPUT_RATE_VERTEX,   texcoords);
    gpConf->assignArray(vertexArrays, "vsg_Color",     VK_VERTEX_INPUT_RATE_INSTANCE, colors);

    gpConf->init();                                                     // .h:139 — after all assigns

    auto stateGroup = vsg::StateGroup::create();                        // nodes/StateGroup.h:31
    gpConf->copyTo(stateGroup);                                         // .h:148 (returns bool)

    auto vid = vsg::VertexIndexDraw::create();                          // nodes/VertexIndexDraw.h:24
    vid->assignArrays(vertexArrays);                                    // .h:42 — pass the SAME DataList
    vid->assignIndices(indices);                                        // .h:43
    vid->indexCount = static_cast<uint32_t>(indices->size());
    vid->instanceCount = 1;                                             // MUST be >= 1, defaults to 0
    stateGroup->addChild(vid);
    scene->addChild(stateGroup);

    // --- standard viewer / camera / loop (see hello-vsg.md) ---
    auto traits = vsg::WindowTraits::create("custom_geometry");
    auto window = vsg::Window::create(traits);
    if (!window) { std::cerr << "Could not create window.\n"; return 1; }
    auto viewer = vsg::Viewer::create();
    viewer->addWindow(window);

    double aspect = static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height);
    auto perspective = vsg::Perspective::create(60.0, aspect, 0.1, 100.0);
    auto lookAt = vsg::LookAt::create(vsg::dvec3(0.0, -3.0, 2.0), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 0.0, 1.0));
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
