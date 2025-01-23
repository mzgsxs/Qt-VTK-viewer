#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include <vtkDataSetReader.h>
#include <vtkPLYReader.h>
#include <vtkOBJReader.h>
#include <vtkSTLReader.h>
#include <vtkNew.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::showAboutDialog()
{
    QMessageBox::information(
        this, "About",
        "Original implementation by Martijn Koopman, modified by Zhiguang Mu. "
        "\nSource code available under Apache License 2.0.");
}

void MainWindow::showOpenFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open file"), "",
                                                    tr("3D Model Files (*.vtk *.stl *.obj *.ply)")
                                                    );

    // Open file
    QFile file(fileName);
    file.open(QIODevice::ReadOnly);

#ifdef QT_DEBUG
    qDebug() << "Selected file for opening:" << fileName;
#endif

    // Return on Cancel
    if (!file.exists())
        return;

    openFile(fileName);
}

void MainWindow::showSaveFileDialog()
{
    // Open "Save As" dialog
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Image"),
        "",
        tr("JPEG Files (*.jpg *.jpeg);;PNG Files (*.png);;All Files (*)")
        );

#ifdef QT_DEBUG
    qDebug() << "Selected file path for saving:" << filePath;
#endif

    // Check if the user canceled the dialog
    if (filePath.isEmpty()) {
        return;
    }

    // Ensure the file has a .jpg extension if none is provided
    if (!filePath.endsWith(".jpg", Qt::CaseInsensitive) &&
        !filePath.endsWith(".jpeg", Qt::CaseInsensitive)) {
        filePath += ".jpg";
    }

#ifdef QT_DEBUG
    qDebug() << "Final file path after extension check:" << filePath;
#endif

    // Save the image and display success or error message
    if (ui->sceneWidget->exportSceneToJPG(filePath.toStdString().c_str())) {
        QMessageBox::information(this, tr("Success"), tr("Image saved successfully!"));
#ifdef QT_DEBUG
        qDebug() << "Image saved successfully to:" << filePath;
#endif
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save the image."));
#ifdef QT_DEBUG
        qCritical() << "Failed to save image to:" << filePath;
#endif
    }
}

void MainWindow::openFile(const QString& fileName)
{
#ifdef QT_DEBUG
    qDebug() << "Opening file:" << fileName;
#endif

    // Clear any previous data
    ui->sceneWidget->removeDataSet();

    // Get the file extension using QFileInfo
    QFileInfo fileInfo(fileName);
    QString extension = fileInfo.suffix().toLower(); // Extract and normalize the file suffix

#ifdef QT_DEBUG
    qDebug() << "File extension:" << extension;
#endif

    // Map of supported extensions to their corresponding VTK reader classes
    const QMap<QString, std::function<vtkSmartPointer<vtkAbstractPolyDataReader>()>> readerMapping = {
        {"obj", []() { return vtkSmartPointer<vtkOBJReader>::New(); }},
        {"ply", []() { return vtkSmartPointer<vtkPLYReader>::New(); }},
        {"stl", []() { return vtkSmartPointer<vtkSTLReader>::New(); }}
    };

    // Check if the extension is supported
    if (!readerMapping.contains(extension) && extension != "vtk") {
#ifdef QT_DEBUG
        qWarning() << "Unsupported file extension:" << extension;
#endif
        return; // Early exit for unsupported extensions
    }

    // Safely downcast the output to vtkDataSet or vtkPolyData
    vtkSmartPointer<vtkDataSet> dataSet = nullptr;
    if (extension == "vtk") {
        vtkNew<vtkDataSetReader> vtkReader; // Create reader
        vtkReader->SetFileName(fileName.toStdString().c_str());
        vtkReader->Update(); // Read the file
        dataSet = vtkDataSet::SafeDownCast(vtkReader->GetOutputDataObject(0));
#ifdef QT_DEBUG
        qDebug() << "Loaded VTK dataset from:" << fileName;
#endif
    } else {
        // Create the appropriate reader
        vtkSmartPointer<vtkAbstractPolyDataReader> reader = readerMapping[extension]();
        reader->SetFileName(fileName.toStdString().c_str());
        reader->Update();
        dataSet = vtkPolyData::SafeDownCast(reader->GetOutputDataObject(0));
#ifdef QT_DEBUG
        qDebug() << "Loaded PolyData from:" << fileName;
#endif
    }

    // Add dataset to 3D view if valid
    if (dataSet) {
        ui->sceneWidget->addDataSet(dataSet);
#ifdef QT_DEBUG
        qDebug() << "Successfully loaded dataset from:" << fileName;
#endif
    } else {
#ifdef QT_DEBUG
        qCritical() << "Failed to load dataset from:" << fileName;
#endif
    }
}
