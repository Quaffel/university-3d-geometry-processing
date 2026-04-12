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

#ifdef ENABLE_SPLATCLOUD_SUPPORT
  #include <ObjectTypes/SplatCloud/SplatCloud.hh>
#endif

#include "PoissonReconstructionPlugin.hh"

#include <OpenFlipper/BasePlugin/PluginFunctions.hh>
#include <OpenFlipper/common/GlobalOptions.hh>
#include <OpenFlipper/BasePlugin/WhatsThisGenerator.hh>
#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>
#include <ObjectTypes/PolyMesh/PolyMesh.hh>
#include <ACG/Utils/ColorCoder.hh>

#include "PoissonReconstructionT.hh"


void PoissonPlugin::initializePlugin(){

  if ( ! OpenFlipper::Options::gui())
      return;
 
  tool_ = new PoissonToolBox();
  
  connect(tool_->reconstructButton, &QPushButton::clicked, this, &PoissonPlugin::slotPoissonReconstruct);

  toolIcon_ = new QIcon(OpenFlipper::Options::iconDirStr()+OpenFlipper::Options::dirSeparator()+"PoissonReconstruction.png");
  emit addToolbox( tr("GP Ex06: Poisson Reconstruction") , tool_, toolIcon_);

  QString info =
  "This plugin is based on the code published by Michael Kazhdan and Matthew Bolitho<br>   "
  "<br>                                                                                    "
  "The following license applies to their code: <br>                                       "
  "Copyright (c) 2006, Michael Kazhdan and Matthew Bolitho <br>                            "
  "All rights reserved. <br>                                                               "
  "<br>                                                                                    "
  "Redistribution and use in source and binary forms, with or without modification,        "
  "are permitted provided that the following conditions are met: <br>                      "
  "<br>                                                                                    "
  "Redistributions of source code must retain the above copyright notice, this list of     "
  "conditions and the following disclaimer. Redistributions in binary form must reproduce  "
  "the above copyright notice, this list of conditions and the following disclaimer        "
  "in the documentation and/or other materials provided with the distribution. <br>        "
  "<br>                                                                                    "
  "Neither the name of the Johns Hopkins University nor the names of its contributors      "
  "may be used to endorse or promote products derived from this software without specific  "
  "prior written permission.  <br>                                                         "
  "<br>                                                                                    "
  "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY   "
  "EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE IMPLIED WARRANTIES     "
  "OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT     "
  "SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,           "
  "INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED    "
  "TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR     "
  "BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN        "
  "CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN      "
  "ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH     "
  "DAMAGE.                                                                                 ";

  emit addAboutInfo(info,"Poisson Reconstruction Plugin");

#if 0
  WhatsThisGenerator whatGen("PoissonReconstruction");
  tool_->reconstructButton->setWhatsThis(tool_->reconstructButton->toolTip()+whatGen.generateLink());
  tool_->depthBox->setWhatsThis(tool_->depthBox->toolTip()+whatGen.generateLink("octree"));
  tool_->label->setWhatsThis(tool_->label->toolTip()+whatGen.generateLink("octree"));
#endif
}


void PoissonPlugin::pluginsInitialized()
{
  emit setSlotDescription("poissonReconstruct(int,int,bool)",tr("Reconstruct a triangle mesh from the given object. Returns the id of the new object or -1 if it failed."),
      QStringList(tr("ObjectId;depth;use_ssd").split(';')),QStringList(tr("ObjectId of the object;octree depth;use SSD reconstruction").split(';')));
  emit setSlotDescription("poissonReconstruct(IdList,int)",tr("Reconstruct one triangle mesh from the given objects. Returns the id of the new object or -1 if it failed."),
      QStringList(tr("IdList;depth;use_ssd").split(';')),QStringList(tr("Id of the objects;octree depth;use SSD reconstruction").split(';')));

  emit setSlotDescription("poissonReconstruct(int)",tr("Reconstruct a triangle mesh from the given object. (Octree depth defaults to 7). Returns the id of the new object or -1 if it failed."),
      QStringList(tr("ObjectId")),QStringList(tr("ObjectId of the object")));
  emit setSlotDescription("poissonReconstruct(IdList)",tr("Reconstruct one triangle mesh from the given objects. (Octree depth defaults to 7). Returns the id of the new object or -1 if it failed."),
      QStringList(tr("IdList")),QStringList(tr("Id of the objects")));
}

int PoissonPlugin::poissonReconstruct(int _id, int _depth, bool _use_ssd)
{
  IdList list(1,_id);
  return poissonReconstruct(list, _depth, _use_ssd);
}

int PoissonPlugin::poissonReconstruct(IdList _ids, int _depth, bool _use_ssd)
{
  PoissonReconParameter params;
  params.max_octree_depth = _depth;
  params.use_ssd = _use_ssd;
  return poissonReconstruct(_ids, params);
}

int PoissonPlugin::poissonReconstruct(IdList _ids, PoissonReconParameter const &params)
{

  IdList generatedMeshes;

  unsigned int n_points = 0;

  // Data container for the algorithm
  // holds two 3D vectors in 6 columns, first the position, followed by the normal of the point
  std::vector<double> pt_data;

  //get data from objects
  for (IdList::iterator idIter = _ids.begin(); idIter != _ids.end(); ++idIter)
  {
    BaseObjectData* obj = 0;
    PluginFunctions::getObject(*idIter,obj);
    if ( obj == 0 ) {
      emit log(LOGERR , QString("Unable to get Object width id %1").arg(*idIter));
      continue;
    }

    //Triangle mesh
    if ( obj->dataType() == DATA_TRIANGLE_MESH) {
      TriMesh* mesh = PluginFunctions::triMesh(obj);

      n_points += mesh->n_vertices();

      emit log(LOGINFO,QString("Adding %1 points from Object %2").arg(mesh->n_vertices()).arg(*idIter) );

      pt_data.reserve( n_points * 6 );
      TriMesh::VertexIter vit = mesh->vertices_begin();
      for ( ; vit != mesh->vertices_end(); ++vit )
      {
        pt_data.push_back( mesh->point( *vit )[0] );
        pt_data.push_back( mesh->point( *vit )[1] );
        pt_data.push_back( mesh->point( *vit )[2] );
        pt_data.push_back( mesh->normal( *vit )[0] );
        pt_data.push_back( mesh->normal( *vit )[1] );
        pt_data.push_back( mesh->normal( *vit )[2] );
      }
    }
    //Poly mesh
    else if ( obj->dataType() == DATA_POLY_MESH) {
      PolyMesh* mesh = PluginFunctions::polyMesh(obj);

      n_points += mesh->n_vertices();

      emit log(LOGINFO,QString("Adding %1 points from Object %2").arg(mesh->n_vertices()).arg(*idIter) );

      pt_data.reserve(n_points * 6);
      PolyMesh::VertexIter vit = mesh->vertices_begin();
      for ( ; vit != mesh->vertices_end(); ++vit )
      {
        pt_data.push_back( mesh->point( *vit )[0] );
        pt_data.push_back( mesh->point( *vit )[1] );
        pt_data.push_back( mesh->point( *vit )[2] );
        pt_data.push_back( mesh->normal( *vit )[0] );
        pt_data.push_back( mesh->normal( *vit )[1] );
        pt_data.push_back( mesh->normal( *vit )[2] );
      }
    }
    //Splat cloud
   #ifdef ENABLE_SPLATCLOUD_SUPPORT
    else if( obj->dataType() == DATA_SPLATCLOUD)
    {
      SplatCloud* cloud = PluginFunctions::splatCloud(obj);

      if ( ! cloud->hasNormals() ) {
        emit log(LOGERR,"Splat cloud has no normals. Skipping it");
        continue;
      }

      n_points += cloud->numSplats();

      emit log(LOGINFO,QString("Adding %1 points from Object %2").arg(cloud->numSplats()).arg(*idIter) );

      pt_data.reserve(n_points * 6 );
      for (unsigned i = 0 ; i < cloud->numSplats(); ++i )
      {
        pt_data.push_back( cloud->positions( i )[0] );
        pt_data.push_back( cloud->positions( i )[1] );
        pt_data.push_back( cloud->positions( i )[2] );
        pt_data.push_back( cloud->normals( i )[0] );
        pt_data.push_back( cloud->normals( i )[1] );
        pt_data.push_back( cloud->normals( i )[2] );
      }
    }
#endif
    else
      emit log(LOGERR,QString("ObjectType of Object with id %1 is unsupported").arg(*idIter));
  }


  int meshId = -1;

  if (pt_data.empty() ) {
      emit log(LOGWARN, "Poisson Reconstruction: input is empty.");
      return -1;
  }

  emit addEmptyObject ( DATA_TRIANGLE_MESH, meshId );

  TriMeshObject* output_tmo = PluginFunctions::triMeshObject(meshId);
  TriMesh* output_mesh = output_tmo->mesh();

  emit log(LOGINFO,"Starting Poisson reconstruction");

  if (reconstruct(pt_data, *output_mesh, params) && output_mesh->n_vertices() > 0) {
      emit log(LOGINFO, "Poisson Reconstruction success");
      output_mesh->update_face_normals(); // for flat shading. per-vertex normals come from PR gradient.
      QString out_name = QString(params.use_ssd ? "SSD" : "Poisson")
          +  "Rec., d=" + QString::number(params.max_octree_depth)
          +  ", w=" + QString::number(params.point_weight)
          ;
      output_tmo->setName(out_name);
      emit updatedObject(meshId, UPDATE_ALL);
      return meshId;
  } else {
      emit log(LOGERR,"Reconstruction failed");
      emit deleteObject( meshId );
      return -1;
  }
}


void PoissonPlugin::slotPoissonReconstruct(){

  if (!OpenFlipper::Options::gui())
    return;

  IdList ids;

  DataType restriction = (DATA_TRIANGLE_MESH | DATA_POLY_MESH);

  #ifdef ENABLE_SPLATCLOUD_SUPPORT
    restriction |= DATA_SPLATCLOUD;
  #endif

  for ( PluginFunctions::ObjectIterator o_it(PluginFunctions::TARGET_OBJECTS, restriction ); o_it != PluginFunctions::objectsEnd(); ++o_it)
  {
    ids.push_back(o_it->id());
  }

  PoissonReconParameter params;
  params.max_octree_depth = tool_->depthBox->value();
  params.use_ssd = tool_->rb_ssd->isChecked();
  params.point_weight = tool_->dsb_point_weight->value();

  int output_id = poissonReconstruct(ids, params);
  if (output_id == -1) return;
  TriMeshObject *tmo = PluginFunctions::triMeshObject(output_id);
  if (!tmo) return;
  auto &mesh = *tmo->mesh();
  auto prop = OpenMesh::getOrMakeProperty<OpenMesh::VertexHandle, double>(mesh, "weight");
  double prop_min = mesh.vertices().min(prop);
  double prop_max = mesh.vertices().max(prop);

  const auto coco = ACG::ColorCoder(prop_min, prop_max);
  for (const auto vh: mesh.vertices()) {
      mesh.set_color(vh, coco(prop[vh]));
  }
  tmo->setObjectDrawMode(
          //ACG::SceneGraph::DrawModes::SOLID_POINTS_COLORED // no shading, displayed color does not depend on orientation or lighting and is simply interpolated vertex color:
          ACG::SceneGraph::DrawModes::SOLID_POINTS_COLORED_SHADED // prettier, but lighting can be misleading
          | ACG::SceneGraph::DrawModes::WIREFRAME
          );
  for (const auto id: ids) {
      BaseObjectData *obj = nullptr;
      PluginFunctions::getObject(id, obj);
      if (obj) {
          obj->visible(false);
          emit updatedObject(id, UPDATE_VISIBILITY);
      }
  }
}
