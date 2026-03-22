#pragma once

#include <QObject>
#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>
#include <OpenFlipper/BasePlugin/LoadSaveInterface.hh>

#include <ObjectTypes/PolyLine/PolyLineTypes.hh>
#include <ObjectTypes/PolyLineCollection/PolyLineCollectionObject.hh>
#include <ObjectTypes/PolyLine/PolyLineObject.hh>
#include <ObjectTypes/PolyMesh/PolyMesh.hh>
#include <ObjectTypes/PolyMesh/PolyMeshTypes.hh>
#include <ObjectTypes/PolyhedralMesh/PolyhedralMesh.hh>

class CurveSmoothingToolbar;

enum class CurveSmoothingMethod {
    UniformLaplacian,
    OsculatingCircle
};
using SimplePolyLine = std::vector<ACG::Vec3d>;

class CurveSmoothingPlugin : public QObject, 
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
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex04-CurveSmoothing")
public:
    QString name() override { return (QString("Ex04-CurveSmoothing")); };
    QString description( ) override { return (QString("Geometry Processing Exercise 04")); };

signals:
    // LoggingInterface:
    void log(Logtype _type, QString _message) override;
    void log(QString _message) override;

    // ToolboxInterface:
    void addToolbox(QString _name, QWidget* _widget) override;

    // BaseInterface:
    void updatedObject(int _objectId, const UpdateType&) override;

    // LoadSaveInterface:
    void addEmptyObject( DataType _type, int& _id) override;

public slots:
    // BaseInterface:
    void initializePlugin() override;

    // our own slots:
    void slotCreateRandomCurve();
    void slotSmooth();
    void slotImportOVM();

private:
    PolyLineObject& getOrCreateCurveObject(int &id, QString const& name, ACG::Vec4f const &color);
    PolyLineObject& getOrCreateInputCurveObject();
    PolyhedralMeshObject& getOrCreateOutputMeshObject();
    PolyhedralMeshObject& getOrCreateVisualizationMeshObject();

    void smoothCurve(PolyLine input, PolyhedralMesh *out_mesh, PolyhedralMesh *vis_mesh = nullptr);

    void rescaleCurve(SimplePolyLine &curve, bool closed, double target_length);
    void smoothLaplacian(SimplePolyLine const& input, SimplePolyLine &output, bool closed, double eps);
    void smoothCurvatureFlow(SimplePolyLine const& input, SimplePolyLine &output, bool closed, double eps);

private:
    CurveSmoothingToolbar *tool_;
    int input_curve_obj_id_ = BaseObject::NOOBJECT;
    int output_mesh_obj_id_ = BaseObject::NOOBJECT;
    int vis_mesh_obj_id_ = BaseObject::NOOBJECT;
};
