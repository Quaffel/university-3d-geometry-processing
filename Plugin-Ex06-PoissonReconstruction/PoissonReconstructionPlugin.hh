#pragma once
/*===========================================================================*\
*                                                                            *
*                              OpenFlipper                                   *
*      Copyright (C) 2001-2014 by Computer Graphics Group, RWTH Aachen       *
*                           www.openflipper.org                              *
*                                                                            *
*--------------------------------------------------------------------------- *
*  This file is part of OpenFlipper.                                         *
*                                                                            *
*  OpenFlipper is free software: you can redistribute it and/or modify       *
*  it under the terms of the GNU Lesser General Public License as            *
*  published by the Free Software Foundation, either version 3 of            *
*  the License, or (at your option) any later version with the               *
*  following exceptions:                                                     *
*                                                                            *
*  If other files instantiate templates or use macros                        *
*  or inline functions from this file, or you compile this file and          *
*  link it with other files to produce an executable, this file does         *
*  not by itself cause the resulting executable to be covered by the         *
*  GNU Lesser General Public License. This exception does not however        *
*  invalidate any other reasons why the executable file might be             *
*  covered by the GNU Lesser General Public License.                         *
*                                                                            *
*  OpenFlipper is distributed in the hope that it will be useful,            *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*  GNU Lesser General Public License for more details.                       *
*                                                                            *
*  You should have received a copy of the GNU LesserGeneral Public           *
*  License along with OpenFlipper. If not,                                   *
*  see <http://www.gnu.org/licenses/>.                                       *
*                                                                            *
\*===========================================================================*/


#include <OpenFlipper/BasePlugin/BaseInterface.hh>
#include <OpenFlipper/BasePlugin/AboutInfoInterface.hh>
#include <OpenFlipper/BasePlugin/ToolboxInterface.hh>
#include <OpenFlipper/BasePlugin/LoggingInterface.hh>
#include <OpenFlipper/BasePlugin/LoadSaveInterface.hh>


#include <QObject>
#include <QtGui>

#include "PoissonToolbox.hh"
#include "PoissonReconstructionT.hh"

class PoissonPlugin : public QObject, BaseInterface, ToolboxInterface, LoadSaveInterface, LoggingInterface, AboutInfoInterface
{
    Q_OBJECT
    Q_INTERFACES(BaseInterface)
    Q_INTERFACES(ToolboxInterface)
    Q_INTERFACES(LoggingInterface)
    Q_INTERFACES(LoadSaveInterface)
    Q_INTERFACES(AboutInfoInterface)
    Q_PLUGIN_METADATA(IID "ch.unibe.cgg.gp26.Ex06-PoissonReconstruction")

public:
  QString name() override { return (QString("Poisson Reconstruction Plugin")); };
  QString description( ) override { return (QString("Poisson reconstruction based on the Code by Michael Kazhdan and Matthew Bolitho")); };
  QString version() override { return QString("2.0"); };

signals:

  //BaseInterface
  void updateView() override;
  void updatedObject(int _identifier, const UpdateType& _type) override;
  void setSlotDescription(QString     _slotName,   QString     _slotDescription,
                          QStringList _parameters, QStringList _descriptions) override;

  //LoggingInterface:
  void log( Logtype _type, QString _message ) override;
  void log( QString _message ) override;

  // Load/Save Interface
  void addEmptyObject (DataType _type, int& _id) override;
  void deleteObject( int _id ) override;

  // ToolboxInterface
  void addToolbox( QString _name  , QWidget* _widget, QIcon* _icon ) override;

  //AboutInfoInterface
  void addAboutInfo(QString _text, QString _tabName ) override;

private slots:

  // BaseInterface
  void initializePlugin() override;
  void pluginsInitialized() override;

private slots:

  /// Button slot iterating over all targets and passing them to the correct functions
  void slotPoissonReconstruct();

  // Tell system that this plugin runs without ui
  void noguiSupported() override {}

public slots:

  int poissonReconstruct(int _id, int _depth = 7, bool _use_ssd = false);
  int poissonReconstruct(IdList _ids, int _depth = 7, bool _use_ssd = false);
  int poissonReconstruct(IdList _ids, PoissonReconParameter const &params);

private:
  PoissonToolBox* tool_ = nullptr;
  QIcon* toolIcon_ = nullptr;
};

