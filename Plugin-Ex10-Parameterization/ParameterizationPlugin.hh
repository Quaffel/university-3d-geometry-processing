#pragma once

#include <QObject>
#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>
#include <OpenFlipper/BasePlugin/PickingInterface.hh>
#include <OpenFlipper/BasePlugin/MouseInterface.hh>
#include <OpenFlipper/BasePlugin/LoadSaveInterface.hh>
#include <OpenFlipper/BasePlugin/TextureInterface.hh>
#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>

class ParameterizationToolbar;

enum class RelaxationType {
    Implicit, Explicit
};

class ParameterizationPlugin : public QObject
                             , public BaseInterface
                             , public ToolboxInterface
                             , public LoggingInterface
                             , public PickingInterface
                             , public MouseInterface
                             , public LoadSaveInterface
                             , public TextureInterface
{
    Q_OBJECT
    Q_INTERFACES(BaseInterface)
    Q_INTERFACES(ToolboxInterface)
    Q_INTERFACES(LoggingInterface)
    Q_INTERFACES(PickingInterface)
    Q_INTERFACES(MouseInterface)
    Q_INTERFACES(LoadSaveInterface)
    Q_INTERFACES(TextureInterface)
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex10-Parameterization")
public:
    QString name() override { return (QString("Ex10-Parameterization")); };
    QString description() override { return (QString("Geometry Processing Exercise 10")); };

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
    void addPickMode( const std::string& _mode ) override;
    void addHiddenPickMode( const std::string& _mode ) override;
    void setPickModeMouseTracking (const std::string& _mode, bool _mouseTracking) override;
    void setPickModeToolbar (const std::string& _mode, QToolBar * _toolbar) override;

    // LoadSaveInterface:
    void addEmptyObject( DataType _type, int& _id) override;
    void deleteObject( int _id ) override;

    // Texture Interface
    void addTexture(QString _textureName, QString _filename, uint dimension) override;
    //void updatedTextures(QString , int) override;
    void setTextureMode(QString _textureName, QString _mode) override;
    void switchTexture( QString _textureName , int _id  ) override;

public slots:
    // BaseInterface:
    void initializePlugin() override;
    void pluginsInitialized() override;
    void slotConstraintButton(int);
    //void slotPickVertex(int which);
    // MouseInterface
    void slotMouseEvent(QMouseEvent* _event) override;

    void slotParamInit();
    void slotParamRelax(RelaxationType);

private:
    void computeHarmonicScalarFunction();
    void ensure_vis_object(TriMeshObject &orig);
    ParameterizationToolbar *tool_;
};
