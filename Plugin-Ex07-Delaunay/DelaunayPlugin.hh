#pragma once

#include <QObject>
#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoadSaveInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>
#include <OpenFlipper/BasePlugin/MouseInterface.hh>
#include <OpenFlipper/BasePlugin/PickingInterface.hh>
#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>
#include <ACG/Scenegraph/GlutPrimitiveNode.hh>
#include <ACG/Utils/HaltonColors.hh>

class DelaunayToolbar;

class DelaunayPlugin : public QObject
                     , public BaseInterface
                     , public ToolboxInterface
                     , public LoggingInterface
                     , public LoadSaveInterface
                     , public MouseInterface
                     , public PickingInterface
{
    Q_OBJECT
    Q_INTERFACES(BaseInterface)
    Q_INTERFACES(ToolboxInterface)
    Q_INTERFACES(LoggingInterface)
    Q_INTERFACES(LoadSaveInterface)
    Q_INTERFACES(MouseInterface)
    Q_INTERFACES(PickingInterface)
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex07-Delaunay")
public:
    QString name() override { return (QString("Ex07-Delaunay")); };
    QString description( ) override { return (QString("Geometry Processing Exercise 07")); };

signals:
    // BaseInterface:
    void updatedObject(int _identifier, const UpdateType& _type) override;
    void updateView() override;
    // LoggingInterface:
    void log(Logtype _type, QString _message) override;
    void log(QString _message) override;

    // ToolboxInterface:
    void addToolbox(QString _name, QWidget* _widget) override;

    // PickingInterface
    void addPickMode(const std::string &_mode) override;

    // LoadSaveInterface:
    void addEmptyObject( DataType _type, int& _id) override;
    void deleteObject( int _id ) override;

private slots:
    // BaseInterface:
    void initializePlugin() override;
    void pluginsInitialized() override;

    // MouseInterface:
    void slotMouseEvent(QMouseEvent* _event) override;

private slots:
    void slot_set_2DView();
    void slot_create_initial_mesh();
    void slot_reset_mesh();

private:
    TriMeshObject& getOrCreateMeshObject();
    void add_cone(TriMeshObject *_tri_obj, ACG::Vec3d const&_point);

    ACG::SceneGraph::GlutPrimitiveNode* getOrCreateGlutPrimitiveNode(TriMeshObject* _tri_obj);


private:
    int mesh_obj_id_ = -1;
    DelaunayToolbar *tool_;
    ACG::HaltonColors hcolors_;
};
