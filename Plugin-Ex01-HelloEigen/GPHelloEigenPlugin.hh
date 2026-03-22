#pragma once

#include <QObject>
#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>

class GPHelloEigenToolbar;

class GPHelloEigenPlugin : public QObject, BaseInterface, ToolboxInterface, LoggingInterface
{
    Q_OBJECT
    Q_INTERFACES(BaseInterface)
    Q_INTERFACES(ToolboxInterface)
    Q_INTERFACES(LoggingInterface)
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex01-HelloEigen")
public:
    QString name() override { return (QString("Ex01-HelloEigen")); };
    QString description( ) override { return (QString("Geometry Processing Exercise 01")); };

signals:
    // LoggingInterface:
    void log(Logtype _type, QString _message) override;
    void log(QString _message) override;

    // ToolboxInterface:
    void addToolbox(QString _name, QWidget* _widget) override;

public slots:
    // BaseInterface:
    void initializePlugin() override;
    void slotHelloEigen();

private:
    GPHelloEigenToolbar *tool_;
};
