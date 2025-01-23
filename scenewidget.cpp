#include "scenewidget.h"

#include <string>
#include <array>

#include <vtkCamera.h>

#include <vtkDataSetMapper.h>
#include <vtkPolyDataMapper.h>

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkProperty.h>
#include <vtkRenderWindowInteractor.h>

#include <vtkRendererCollection.h>


// Shading
#include <vtkLight.h>
#include <vtkLightKit.h>
#include <vtkGeometryFilter.h>

// Exporting
#include <vtkWindowToImageFilter.h>
#include <vtkJPEGWriter.h>



SceneWidget::SceneWidget(QWidget* parent)
    : QVTKOpenGLNativeWidget(parent)
{
    vtkNew<vtkGenericOpenGLRenderWindow> window;
    setRenderWindow(window.Get());

    // Set up a shared camera
    vtkNew<vtkCamera> camera;
    camera->SetViewUp(0, 1, 0);
    camera->SetPosition(0, 0, 10);
    camera->SetFocalPoint(0, 0, 0);

    // Create renderers with shared camera and add them to the render window
    topLeftRenderer = createRenderer(camera, {0.0, 0.5, 0.5, 1.0}); // Top-left
    topRightRenderer = createRenderer(camera, {0.5, 0.5, 1.0, 1.0}); // Top-right
    bottomLeftRenderer = createRenderer(camera, {0.0, 0.0, 0.5, 0.5}); // Bottom-left
    bottomRightRenderer = createRenderer(camera, {0.5, 0.0, 1.0, 0.5}); // Bottom-right

}


vtkSmartPointer<vtkRenderer>
SceneWidget::createRenderer(vtkSmartPointer<vtkCamera> camera, std::array<double, 4> const & viewport)
{
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetActiveCamera(camera);
    renderer->SetBackground(0.3, 0.3, 0.3); // Gray background
    renderer->SetViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    renderWindow()->AddRenderer(renderer);
    return renderer;
}


void SceneWidget::addDataSet(vtkSmartPointer<vtkDataSet> dataSet)
{
    currentDataSet = dataSet;

    // Convert vtkDataSet to vtkPolyData if necessary
    vtkSmartPointer<vtkPolyData> polyData = vtkPolyData::SafeDownCast(currentDataSet);
    if (!polyData) {
        vtkSmartPointer<vtkGeometryFilter> geometryFilter = vtkSmartPointer<vtkGeometryFilter>::New();
        geometryFilter->SetInputData(currentDataSet);
        geometryFilter->Update();
        polyData = geometryFilter->GetOutput();
    }

    // Compute normals
    vtkSmartPointer<vtkPolyDataNormals> normalsFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
    normalsFilter->SetInputData(polyData);
    normalsFilter->SplittingOn();
    normalsFilter->ComputePointNormalsOn(); // Enable point normals
    normalsFilter->ComputeCellNormalsOn();  // Enable cell normals
    normalsFilter->Update();
    currentNormals = normalsFilter->GetOutput();

    // Setup renderer for each viewport
    initRenderer(topLeftRenderer, "wireframe");
    initRenderer(topRightRenderer, "Flat shading");
    initRenderer(bottomLeftRenderer, "Gouraud shading");
    initRenderer(bottomRightRenderer, "Phong shading");

    // Render
    renderWindow()->Render();
}


void configureActorProperties(vtkSmartPointer<vtkProperty> prop) {
    prop->SetColor(0, 1, 0);          // Green color
    prop->SetAmbient(0.3);           // Ambient light coefficient
    prop->SetDiffuse(0.3);           // Diffuse light coefficient
    prop->SetSpecular(0.8);          // Specular light coefficient
    prop->SetSpecularPower(40.0);    // Specular highlight size
    prop->ShadingOn();               // Enable shading
}


void configureLight(vtkSmartPointer<vtkLight> light) {
    light->SetLightTypeToSceneLight();
    light->SetAmbientColor(1, 1, 1);   // White ambient light
    light->SetDiffuseColor(1, 1, 1);   // White diffuse light
    light->SetSpecularColor(1, 1, 1);  // White specular light
    light->SetPosition(-100, 100, 25); // Light position in the scene
    light->SetFocalPoint(0, 0, 0);     // Light focal point
    light->SetIntensity(0.8);          // Light intensity
}


bool setShadingMode(vtkSmartPointer<vtkProperty> prop, const std::string& displayMode) {
    if (displayMode == "Flat shading") {
        prop->SetInterpolationToFlat();
    } else if (displayMode == "Gouraud shading") {
        prop->SetInterpolationToGouraud();
    } else if (displayMode == "Phong shading") {
        prop->SetInterpolationToPhong();
    } else {
        return false; // Unsupported shading mode
    }
    return true; // Shading mode successfully set
}


void applyShading(vtkSmartPointer<vtkActor> actor, vtkSmartPointer<vtkLight> light, const std::string& displayMode) {
    // Configure actor properties
    vtkSmartPointer<vtkProperty> prop = actor->GetProperty();
    configureActorProperties(prop);

    // Set shading mode
    if (!setShadingMode(prop, displayMode)) {
        std::cerr << "Error: Unsupported shading mode '" << displayMode << "'." << std::endl;
        return;
    }

    // Configure lighting
    configureLight(light);
}


void SceneWidget::initRenderer(vtkSmartPointer<vtkRenderer> renderer, std::string const & displayMode)
{
    // Each viewport with it's own Actor & Mapper
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    vtkSmartPointer<vtkDataSetMapper> mapper = vtkSmartPointer<vtkDataSetMapper>::New();

    mapper->SetInputData(currentNormals);
    actor->SetMapper(mapper);

    // Display Mode
    if (displayMode == "wireframe"){
        actor->GetProperty()->SetRepresentationToWireframe();
    }
    else if (
            displayMode == "Flat shading" ||
            displayMode == "Gouraud shading" ||
            displayMode == "Phong shading"
        ) {
        vtkSmartPointer<vtkLight> light = vtkSmartPointer<vtkLight>::New();
        applyShading(actor, light, displayMode);
        renderer->AddLight(light);
    }
    else {
        ;
    }

    renderer->AddActor(actor);
    renderer->ResetCamera(currentDataSet->GetBounds());
}


void SceneWidget::removeDataSet()
{
    // Clean up all renderers
    auto renderers = renderWindow()->GetRenderers();
    renderers->InitTraversal();

    while (auto renderer = vtkRenderer::SafeDownCast(renderers->GetNextItemAsObject())) {
        vtkSmartPointer<vtkActor> actor = renderer->GetActors()->GetLastActor();
        if (actor != nullptr) {
            renderer->RemoveActor(actor);
        }
    }

    renderWindow()->Render();
}


void SceneWidget::zoomToExtent()
{
    // Get all renderers
    auto renderers = renderWindow()->GetRenderers();
    renderers->InitTraversal();

    while (auto renderer = vtkRenderer::SafeDownCast(renderers->GetNextItemAsObject())) {
        // Zoom to extent of last added actor
        vtkSmartPointer<vtkActor> actor = renderer->GetActors()->GetLastActor();
        if (actor != nullptr) {
            renderer->ResetCamera(actor->GetBounds());
        }
    }

    renderWindow()->Render();
}


bool SceneWidget::exportSceneToJPG(std::string const & filename) {
    try {
        // Validate filename
        if (filename.empty()) {
            qWarning() << "Filename is empty.";
            return false;
        }

        // Capture the render window content
        vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
        windowToImageFilter->SetInput(renderWindow());
        windowToImageFilter->SetInputBufferTypeToRGB(); // Capture RGB data
        windowToImageFilter->ReadFrontBufferOff();      // Read from back buffer
        windowToImageFilter->Update();

        // Write the image to a JPG file
        vtkSmartPointer<vtkJPEGWriter> jpegWriter = vtkSmartPointer<vtkJPEGWriter>::New();
        jpegWriter->SetFileName(filename.c_str());
        jpegWriter->SetInputConnection(windowToImageFilter->GetOutputPort());
        jpegWriter->Write();

        qDebug() << "Scene successfully exported to:" << QString::fromStdString(filename);
        return true;
    } catch (std::exception const & e) {
        qCritical() << "Exception occurred while exporting scene to JPG:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception occurred while exporting scene to JPG.";
    }

    return false; // Return false if any exception occurs
}
