#pragma once

#include <QObject>
#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>
#include <OpenFlipper/BasePlugin/LoadSaveInterface.hh>
#include <ObjectTypes/TriangleMesh/TriangleMeshTypes.hh>
#include <ObjectTypes/PolyhedralMesh/PolyhedralMesh.hh>
#include <OpenMesh/Core/Utils/PropertyManager.hh>

class IsoContouringToolbar;

using ScalarVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<double>>;

class IsoContouringPlugin : public QObject,
    BaseInterface,
    ToolboxInterface,
    LoggingInterface,
    LoadSaveInterface
{
    Q_OBJECT
    Q_INTERFACES(BaseInterface)
    Q_INTERFACES(ToolboxInterface)
    Q_INTERFACES(LoggingInterface)
    Q_INTERFACES(LoadSaveInterface)
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex03-IsoContouring")
public:
    QString name() override { return (QString("Ex03-IsoContouring")); };
    QString description( ) override { return (QString("UniBE 3D Geometry Processing Exercise 03")); };

signals:
    // LoggingInterface:
    void log(Logtype _type, QString _message) override;
    void log(QString _message) override;

    // ToolboxInterface:
    void addToolbox(QString _name, QWidget* _widget) override;

    // BaseInterface:
    void updatedObject(int _identifier, const UpdateType& _type) override;

    // LoadSaveInterface:
    void addEmptyObject( DataType _type, int& _id) override;

public slots:
    // BaseInterface:
    void initializePlugin() override;
    void slotObjectUpdated(int _identifier, const UpdateType& _type) override;

    // our own slots:
    void slotUpdateIsoContours();

private:
    PolyhedralMeshObject &get_output_mesh_object();
    void sliceMesh(TriMesh& mesh);
    void extractIsoContour(PolyhedralMeshObject &output, TriMesh& input,
            ScalarVProp const &func, double value, OpenMesh::Vec4f const &color);

private:
    IsoContouringToolbar *tool_;
    int output_pmo_id_ = -1; // visualisation object id (PolyhedralMeshObject)
};
