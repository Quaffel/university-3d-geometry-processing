#pragma once

#include <QObject>
#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>
#include <ObjectTypes/TriangleMesh/TriangleMeshTypes.hh>
#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>
#include <OpenMesh/Core/Utils/PropertyManager.hh>
#include <MeshSmoothingToolbar.hh>

class MeshSmoothingPlugin : public QObject,
    BaseInterface,
    ToolboxInterface,
    LoggingInterface
{
    Q_OBJECT
    Q_INTERFACES(BaseInterface)
    Q_INTERFACES(ToolboxInterface)
    Q_INTERFACES(LoggingInterface)
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex08-MeshSmoothingPlugin")
public:
    QString name() override { return (QString("Ex08-MeshSmoothingPlugin")); };
    QString description( ) override { return (QString("Geometry Processing Exercise 08")); };

signals:
    // LoggingInterface:
    void log(Logtype _type, QString _message) override;
    void log(QString _message) override;

    // ToolboxInterface:
    void addToolbox(QString _name, QWidget* _widget) override;

    // BaseInterface:
    //void updatedObject(int _objectId) override;
    void updatedObject(int _identifier, const UpdateType& _type) override;

public slots:
    // BaseInterface:
    void initializePlugin() override;
    void slotObjectUpdated(int _identifier, const UpdateType& _type) override;

    // our own slots:
    void slotInvalidateHistory();
    void slotSettingsChanged();
    void slotUpdatePreviewVisibility();
    void slotApply();

private:
    void smooth(TriMeshObject& obj, SmoothingSettings const &);
    void smooth_explicit(TriMeshObject& obj, SmoothingSettings const &);
    bool smooth_implicit(TriMeshObject& obj, SmoothingSettings const &);
    TriMeshObject& getOrCreatePreviewObject(TriMeshObject& input_object);

private:
    MeshSmoothingToolbar *tool_;
};
