#pragma once

#include <QObject>
#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>
#include <OpenFlipper/BasePlugin/BackupInterface.hh>
#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>

class RemeshingToolbar;

enum class VertexType {
    Rest,
    Fixed,
    Handle,
};
class Remeshing;

class RemeshingPlugin : public QObject
                          , public BaseInterface
                          , public ToolboxInterface
                          , public LoggingInterface
                          , public BackupInterface
{
    Q_OBJECT
    Q_INTERFACES(BaseInterface)
    Q_INTERFACES(ToolboxInterface)
    Q_INTERFACES(LoggingInterface)
    Q_INTERFACES(BackupInterface)
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex09-Remeshing")
public:
    QString name() override { return (QString("Ex09-Remeshing")); };
    QString description( ) override { return (QString("Geometry Processing Exercise 09")); };

signals:
    // BaseInterface:
    void updateView() override;
    void updatedObject(int _identifier, const UpdateType& _type) override;
    // BackupInterface
    void createBackup( int _objectid, QString _name, UpdateType _type = UPDATE_ALL) override;
    // LoggingInterface:
    void log(Logtype _type, QString _message) override;
    void log(QString _message) override;

    // ToolboxInterface:
    void addToolbox(QString _name, QWidget* _widget) override;


public slots:
    // BaseInterface:
    void initializePlugin() override;

    // ours:
    void slot_init();
    void slot_remesh();
    void slot_smooth();
    void slot_flip();
    void slot_collapse();
    void slot_split();
private:
    Remeshing& get_or_create_remesh_pod(TriMeshObject& object);
    template <typename Fn>
    void for_each_target_trimesh_remesh(Fn&& fn);
    RemeshingToolbar *tool_;
};
