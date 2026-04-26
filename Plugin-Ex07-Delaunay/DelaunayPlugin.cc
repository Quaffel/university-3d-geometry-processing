#include "DelaunayPlugin.hh"
#include <Eigen/Sparse>
#include <OpenFlipper/BasePlugin/PluginFunctions.hh>
#include <ObjectTypes/TriangleMesh/PluginFunctionsTriangleMesh.hh>
#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>
#include <ACG/Scenegraph/GlutPrimitiveNode.hh>
#include <ACG/Utils/HaltonColors.hh>

#include <queue>

#include "ui_DelaunayToolbarBase.h"

#define ASSIGNMENT_07_SOLUTION 1
using Vec3d = ACG::Vec3d;
using Vec2d = ACG::Vec2d;

bool is_delaunay(TriMesh const& _mesh, TriMesh::EdgeHandle _eh) {
    OpenMesh::SmartEdgeHandle eh = _mesh.make_smart(_eh);

    /* 
        We assume that the provided edge '_eh' (here depicted through its halfedge 'h0') is incident to two triangles.
        We establish the following naming convention:

                       (d) = h0.next.to
                      /   \
                     /     \
                    /       \
        h0.from = (a)--h0-->(c) = h0.to
                    \       /
                     \     /
                      \   /
                       (b) = h1.next.to
    */
    assert(eh.h1().next().is_valid());
    assert(eh.h0().next().is_valid());

    Vec3d const& a = _mesh.point(eh.h0().from());
    Vec3d const& b = _mesh.point(eh.h1().next().to());
    Vec3d const& c = _mesh.point(eh.h0().to());
    Vec3d const& d = _mesh.point(eh.h0().next().to());

    // We assume that this is a 2-D mesh with all z coordinates = 0
    assert(a[2] == 0);
    assert(b[2] == 0);
    assert(c[2] == 0);
    assert(d[2] == 0);

    const auto square_distance_to_origin = [](const Vec3d& p) {
        return p[0] * p[0] + p[1] * p[1];
    };

    // ACG matrices are initialized through a column-major array
    const double open_mesh_data[16] = {
        a[0], b[0], c[0], d[0],
        a[1], b[1], c[1], d[1],
        square_distance_to_origin(a), square_distance_to_origin(b), 
            square_distance_to_origin(c), square_distance_to_origin(d),
        1.0,  1.0,  1.0,  1.0
    };
    ACG::Matrix4x4d incircle_test_matrix {open_mesh_data};

    return incircle_test_matrix.determinant() <= 0;
}

/// Insert point into planar (z=0) delaunay mesh and re-establish delaunay
/// property. Return the number of flips that were performed.
size_t insert_point(TriMesh & _mesh, const TriMesh::FaceHandle& _fh,
        const TriMesh::EdgeHandle& _eh, const TriMesh::Point& _p)
{
    // add vertex, assign random color to it
    auto p = TriMesh::Point(_p[0], _p[1], 0.0);
    auto vh = _mesh.add_vertex(p);

    /*
        The newly inserted vertex 'v' is inserted into an already existing triangle.
        
             (d)
            / | \
           / (v) \
          / /   \ \
        (a)-------(c)

        Note that if 'v' falls onto an edge, we perform a 2:4 split instead.
    */
    if (_eh.is_valid()) {
        _mesh.split(_eh, vh);
    } else if (_fh.is_valid()) {
        _mesh.split(_fh, vh);
    } else {
        return 0;
    }
    
    size_t n_flips = 0;
    std::queue<TriMesh::EdgeHandle> edges_to_check;

    /* 
        To ensure that the mesh remains delaunay even after the insertion, we first need to check
        the delaunay property for all edges opposite to the newly inserted vertex.
        These edges are ac, cd, and ad.
        Note that there are 4 opposite edges in case of a 2:4 split.
    */
    for (OpenMesh::SmartHalfedgeHandle edge : vh.outgoing_halfedges()) {
        OpenMesh::EdgeHandle opposite_edge = edge.next().edge();
        edges_to_check.push(opposite_edge);
    }

    /* Iteratively check for Delaunay property for all edges, and flip them if property is currently not satisfied.
        Edges that can be flipped are incident to two triangles.
        We establish the following naming convention ('current_edge' is depicted through halfedge 'h0'):

                       (d) = h0.next.to
                      /   \
                     /     \
                    /       \
        h0.from = (a)--h0-->(c) = h0.to
                    \       /
                     \     /
                      \   /
                       (b) = h1.next.to
    */
    for (; !edges_to_check.empty(); edges_to_check.pop()) {
        OpenMesh::EdgeHandle current_edge = edges_to_check.front();
        if (is_delaunay(_mesh, current_edge)) {
            continue;
        }

        if (!_mesh.is_flip_ok(current_edge)) {
            continue;
        }

        _mesh.flip(current_edge);
        n_flips++;

        OpenMesh::SmartHalfedgeHandle current_halfedge_h0 = _mesh.make_smart(current_edge).h0();
        OpenMesh::SmartHalfedgeHandle current_halfedge_h1 = _mesh.make_smart(current_edge).h1();

        edges_to_check.push(current_halfedge_h0.next().edge());
        edges_to_check.push(current_halfedge_h0.next().next().edge());

        edges_to_check.push(current_halfedge_h1.next().edge());
        edges_to_check.push(current_halfedge_h1.next().next().edge());
    }

    _mesh.garbage_collection();
    return n_flips;
}

class DelaunayToolbar : public QWidget, public Ui::DelaunayToolbarBase
{
    public:
    DelaunayToolbar(QWidget * parent = nullptr)
        : QWidget(parent)
    {
        setupUi(this);
    }
};

void DelaunayPlugin::initializePlugin()
{
    tool_ = new DelaunayToolbar();
    connect(tool_->create_test_mesh, &QPushButton::clicked, this, &DelaunayPlugin::slot_create_initial_mesh);
    connect(tool_->pb_reset_mesh, &QPushButton::clicked, this, &DelaunayPlugin::slot_reset_mesh);
    connect(tool_->pb_set_2d_view, &QPushButton::clicked, this, &DelaunayPlugin::slot_set_2DView);
    emit addToolbox( tr("GP Ex07: Delaunay") , tool_ );
}
static char pickmode_name[] = "DelaunayDemoAddVertex";
void DelaunayPlugin::pluginsInitialized() {
    emit addPickMode("Separator");
    emit addPickMode(pickmode_name);
}
void DelaunayPlugin::slotMouseEvent(QMouseEvent* _event)
{
    // control modifier is reserved for selecting target
    if (_event->modifiers() & (Qt::ControlModifier)) {
        return;
    }
    if (_event->type() != QEvent::MouseButtonPress) return;
    if (PluginFunctions::actionMode() != Viewer::PickingMode) return;
    if (PluginFunctions::pickMode() != (pickmode_name)) return;
    if (_event->button() != Qt::LeftButton) return;

    size_t node_idx, target_idx;
    ACG::Vec3d hit_point;

    // first, check if the user picked an edge
    if (PluginFunctions::scenegraphPick(ACG::SceneGraph::PICK_EDGE, _event->pos(),
                node_idx, target_idx, &hit_point)) {
        BaseObjectData *obj;
        if (!PluginFunctions::getPickedObject(node_idx, obj)) return;
        // is picked object a triangle mesh?
        auto tri_obj = PluginFunctions::triMeshObject(obj);
        if (!tri_obj) return;
        auto eh = tri_obj->mesh()->edge_handle(target_idx);
        if (!tri_obj->mesh()->is_valid_handle(eh)) return;

        emit log(LOGINFO, "Adding point on edge");
        // insert the respective point into the triangulation
        add_cone(tri_obj, hit_point);
        auto n_flips = insert_point(*tri_obj->mesh(), TriMesh::InvalidFaceHandle, eh, hit_point);
        emit log(LOGINFO, "Restored Delaunay property using " + QString::number(n_flips) + " edge flips.");
        emit updatedObject(tri_obj->id(), UPDATE_ALL);
        return;
    }

    // no edge picked, maybe a face?
    if (PluginFunctions::scenegraphPick(ACG::SceneGraph::PICK_FACE, _event->pos(),
                node_idx, target_idx, &hit_point)) {
        BaseObjectData *obj;
        if (!PluginFunctions::getPickedObject(node_idx, obj)) return;
        // is picked object a triangle mesh?
        TriMeshObject *tri_obj = PluginFunctions::triMeshObject(obj);
        if (!tri_obj) return;
        auto fh = tri_obj->mesh()->face_handle(target_idx);
        if (!tri_obj->mesh()->is_valid_handle(fh)) return;
        // insert the respective point into the triangulation
        emit log(LOGINFO, "Adding point in face");
        add_cone(tri_obj, hit_point);
        auto n_flips = insert_point(*tri_obj->mesh(), fh, TriMesh::InvalidEdgeHandle, hit_point);
        emit log(LOGINFO, "Restored Delaunay property using " + QString::number(n_flips) + " edge flips.");
        emit updatedObject(tri_obj->id(), UPDATE_ALL);
        return;
    }
}
void DelaunayPlugin::slot_create_initial_mesh() {
    getOrCreateMeshObject();
    slot_set_2DView();
}

void DelaunayPlugin::slot_reset_mesh() {
    emit deleteObject(mesh_obj_id_);
    mesh_obj_id_ = -1;
    slot_create_initial_mesh();
}

TriMeshObject& DelaunayPlugin::getOrCreateMeshObject()
{
    if (auto *tmo = PluginFunctions::triMeshObject(mesh_obj_id_)) {
        tmo->show();
        emit updatedObject(tmo->id(), UPDATE_VISIBILITY);
        return *tmo;
    } else {
        emit addEmptyObject(DATA_TRIANGLE_MESH, mesh_obj_id_);
        tmo = PluginFunctions::triMeshObject(mesh_obj_id_);
        tmo->target(true);
        tmo->show();
        tmo->setName("DelaunayMesh");
        //tmo->materialNode()->set_specular_color(ACG::Vec4f(0.0, 0.0, 0.0, 1.0));
        tmo->materialNode()->set_line_width(2);
        tmo->materialNode()->set_point_size(10);
        tmo->materialNode()->set_color(ACG::Vec4f(.0f, .0f, .0f, 1.f));
        tmo->setObjectDrawMode(ACG::SceneGraph::DrawModes::WIREFRAME);

        // create initial mesh:
        auto mesh = tmo->mesh();

        std::array<TriMesh::VertexHandle, 4> vhs = {
            mesh->add_vertex({-.5, -.5, 0.}),
            mesh->add_vertex({ .5, -.5, 0.}),
            mesh->add_vertex({ .5,  .5, 0.}),
            mesh->add_vertex({-.5,  .5, 0.})};

        for (const auto vh: mesh->vertices()) {
            add_cone(tmo, mesh->point(vh));
        }
        mesh->add_face(vhs[0], vhs[1], vhs[2]);
        mesh->add_face(vhs[0], vhs[2], vhs[3]);

        emit updatedObject(tmo->id(), UPDATE_ALL);
        return *tmo;
    }
}

void DelaunayPlugin::slot_set_2DView() {
    PluginFunctions::orthographicProjection();
    // set viewing direction along negative z axis
    ACG::Vec3d eye    {0.0, 0.0, 1.0};
    ACG::Vec3d center {0.0, 0.0, 0.0};
    ACG::Vec3d up     {0.0, 1.0, 0.0};
    PluginFunctions::lookAt(eye, center, up);
    // Set appropriate (quasi-)zoom level:
    PluginFunctions::viewerProperties().orthoWidth(1.0);

    PluginFunctions::pickMode(pickmode_name);
    PluginFunctions::actionMode(Viewer::ActionMode::PickingMode);

}




void DelaunayPlugin::add_cone(TriMeshObject *_tri_obj, ACG::Vec3d const&_point)
{
    // add cones
    auto node = getOrCreateGlutPrimitiveNode(_tri_obj);
    auto axis = ACG::Vec3d(0,0,1);
    // add an offset to the cones position, since the position is not located at the apex
    // but at the intersection of axis and base plate
    auto offset = ACG::Vec3d(0,0,-1);

    node->add_primitive(ACG::SceneGraph::GlutPrimitiveNode::CONE, _point+offset, axis, hcolors_.get_next_color());
}

ACG::SceneGraph::GlutPrimitiveNode*
DelaunayPlugin::getOrCreateGlutPrimitiveNode(TriMeshObject* _tri_obj) {
    // get or add material node for primitive node
    ACG::SceneGraph::MaterialNode * material_node = 0;
    if(!_tri_obj->hasAdditionalNode(name(), "PrimitiveMaterialNode")) {
        material_node = new ACG::SceneGraph::MaterialNode(
                _tri_obj->manipulatorNode(), "DGP Delaunay2D: Cone Material");

        if(!_tri_obj->addAdditionalNode(
                material_node, QString(name()), "PrimitiveMaterialNode")
                ) {
            emit(LOGERR, "DelaunayPlugin::getOrCreateGlutPrimitiveNode(): could not add an primitive material node");
            return nullptr;
        }
    } else {
        _tri_obj->getAdditionalNode(material_node,  name(), "PrimitiveMaterialNode" );
    }

    material_node->set_specular_color(ACG::Vec4f(0.0, 0.0, 0.0, 1.0));

    ACG::SceneGraph::GlutPrimitiveNode * glut_node = 0;
    if(!_tri_obj->hasAdditionalNode(name(), "GlutPrimitiveNode")) {
        glut_node = new ACG::SceneGraph::GlutPrimitiveNode(material_node, "Voronoi diagram cones");
        if(!_tri_obj->addAdditionalNode(glut_node, name(), "GlutPrimitiveNode")) {
            emit(LOGERR, "DelaunayPlugin::getOrCreateGlutPrimitiveNode(): could not add glut primitive node\n");
            return nullptr;
        }
        glut_node->clear();
        //glut_node->drawMode(ACG::SceneGraph::DrawModes::SOLID_FACES_COLORED);
        glut_node->drawMode(ACG::SceneGraph::DrawModes::SOLID_FACES_COLORED_SMOOTH_SHADED);
        glut_node->show();
    } else {
        _tri_obj->getAdditionalNode(glut_node, name(), "GlutPrimitiveNode" );
    }

    return glut_node;
}
