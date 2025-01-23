#ifndef SCENEWIDGET_H
#define SCENEWIDGET_H

#include <QVTKOpenGLNativeWidget.h>
#include <vtkDataSet.h>
#include <vtkAlgorithmOutput.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>


#include <vtkPolyDataNormals.h>

class SceneWidget : public QVTKOpenGLNativeWidget {
    Q_OBJECT
public:
    explicit SceneWidget(QWidget* parent = nullptr);

    //! Add a data set to the scene
    /*!
    \param[in] dataSet The data set to add
    */
    void addDataSet(vtkSmartPointer<vtkDataSet>);
    bool exportSceneToJPG(std::string const &);

    //! Remove the data set from the scene
    void removeDataSet();

public slots:
    //! Zoom to the extent of the data set in the scene
    void zoomToExtent();

private:
    // Data holder
    vtkSmartPointer<vtkDataSet> currentDataSet;
    vtkSmartPointer<vtkPolyData> currentNormals;
    // Renderers
    vtkSmartPointer<vtkRenderer> createRenderer(vtkSmartPointer<vtkCamera>, const std::array<double, 4> &);
    void initRenderer(vtkSmartPointer<vtkRenderer>, std::string const & displayMode = "");
    // Viewports
    vtkSmartPointer<vtkRenderer> topLeftRenderer;
    vtkSmartPointer<vtkRenderer> topRightRenderer;
    vtkSmartPointer<vtkRenderer> bottomLeftRenderer;
    vtkSmartPointer<vtkRenderer> bottomRightRenderer;
};

#endif // SCENEWIDGET_H
