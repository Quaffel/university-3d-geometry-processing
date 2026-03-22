#include "IsoContouringPlugin.hh"
#include "IsoContouringToolbar.hh"

#include <OpenFlipper/BasePlugin/PluginFunctions.hh>
#include <ObjectTypes/TriangleMesh/PluginFunctionsTriangleMesh.hh>
#include <ObjectTypes/PolyhedralMesh/PluginFunctionsPolyhedralMesh.hh>
#include <ACG/Utils/ColorCoder.hh>

#define ASSIGNMENT_03_SOLUTION 1

using SVH = OpenMesh::SmartVertexHandle;
using VH = OpenMesh::VertexHandle;
using EH = OpenMesh::EdgeHandle;
using FH = OpenMesh::FaceHandle;
using Point = TriMesh::Point;

void IsoContouringPlugin::initializePlugin()
{
    tool_ = new IsoContouringToolbar();
    connect(tool_, &IsoContouringToolbar::valueChanged, this, &IsoContouringPlugin::slotUpdateIsoContours);
    emit addToolbox(tr("GP Ex03: Iso-Contouring") , tool_ );
}

void IsoContouringPlugin::extractIsoContour(PolyhedralMeshObject &output, TriMesh& input,
        ScalarVProp const &func, double value, OpenMesh::Vec4f const &color)
{
    PolyhedralMesh &contour_mesh = *output.mesh();

    // This "property" allows us to save an _output_ OVM vertex handle per _input_ edge
    auto contour_vertex_by_edge = OpenMesh::makeTemporaryProperty<EH, OpenVolumeMesh::VH>(input);

    // Initially, no edge contains any output vertex, mark them all invalid:
    contour_vertex_by_edge.set_range(input.edges(), PolyhedralMesh::InvalidVertexHandle);

    std::set<FH> faces_adjacent_to_intersecting_edges;
    for (const auto edge_handle: input.edges()) {
        SVH v0 = edge_handle.v0();
        SVH v1 = edge_handle.v1();

        double v0_value = func[v0];
        double v1_value = func[v1];

        bool v0_inside_shape = v0_value < value;
        bool v1_inside_shape = v1_value < value;
        bool edge_intersects_with_contour = v0_inside_shape ^ v1_inside_shape;

        if (edge_intersects_with_contour) {
            const Point &v0_coordinates = input.point(v0);
            const Point &v1_coordinates = input.point(v1);

            double t = (value - v0_value) / (v1_value - v0_value);
            const auto intersection_point = v0_coordinates + t * (v1_coordinates - v0_coordinates);

            contour_vertex_by_edge[edge_handle] = contour_mesh.add_vertex(intersection_point);
            for (const auto adjacent_face: edge_handle.faces()) {
                faces_adjacent_to_intersecting_edges.insert(adjacent_face);
            }
        }
    }

    for (const auto _fh: faces_adjacent_to_intersecting_edges) {
        auto adjacent_face = input.make_smart(_fh);

        std::optional<OpenVolumeMesh::VertexHandle> first_contour_vertex;
        for (const auto intersecting_edge : adjacent_face.edges()) {
            OpenVolumeMesh::VertexHandle contour_vertex = contour_vertex_by_edge[intersecting_edge];
            if (!contour_vertex.is_valid()) {
                continue;
            }
            
            if (!first_contour_vertex.has_value()) {
                first_contour_vertex = contour_vertex;
                continue;
            }
            
            auto contour_edge = contour_mesh.add_edge(*first_contour_vertex, contour_vertex);
            output.colors()[contour_edge] = color;
        }        
    }
}

void IsoContouringPlugin::sliceMesh(TriMesh &mesh)
{
    // A property saves data per entity (in this case, a `double` per vertex)
    auto prop = OpenMesh::getOrMakeProperty<OpenMesh::VertexHandle, double>(mesh, "func");

    /// Compute per-vertex function value:
    auto eval_f = [&](VH vh) -> double {
        const auto pos = mesh.point(vh);
        return pos[2];
        // Note: This is a good place to try more interesting functions than just
        //       the z coordinate of the vertex positions.
        //       This is optional :)
        // These functions in the x-y plane could be a start:
        //    f(x,y) = \sqrt{x^2+y^2}-1
        //    f(x,y) = y^2 - \sin{(x^2)}
        //    f(x,y) = \sin{(2x+2y)} - \cos{(4xy)} +1
        //    f(x,y) = (3x^2 - y^2)^2y^2-(x^2+y^2)^4
    };

    for (const auto vh: mesh.vertices()) {
        prop[vh] = eval_f(vh);
    }

    /// We use a OpenVolumeMesh PolyhedralMesh for visualisation purposes:
    PolyhedralMeshObject &output_mesh_object = get_output_mesh_object();
    output_mesh_object.mesh()->clear(); // we re-use this mesh, so get rid of the old stuff

    // We want to distribute iso-contours uniformly among the entire function range:
    const double min_value = mesh.vertices().min(prop);
    const double max_value = mesh.vertices().max(prop);

    const int n_slices = tool_->n_slices();
    const double offset = tool_->offset(); // this is in the range [0..1]
    emit log(LOGINFO, "Countouring into " + QString::number(n_slices)
            + " slices, offset " + QString::number(offset));

    const auto coco = ACG::ColorCoder(min_value, max_value);

    for (int i = 0; i < n_slices; ++i) {
        double t = (i + offset) / n_slices;
        double value = (1-t) * min_value + t * max_value;
        auto color = coco.color_float4_raw(value);
        extractIsoContour(output_mesh_object, mesh, prop, value, color);
    }
    // We need to let OpenFlipper know that we modified the object,
    // so it will re-upload it to the GPU (and let other plugins know that something changed)
    emit updatedObject(output_mesh_object.id(), UPDATE_ALL);
}

PolyhedralMeshObject& IsoContouringPlugin::get_output_mesh_object()
{
    if (auto *pmo = PluginFunctions::polyhedralMeshObject(output_pmo_id_)) {
        return *pmo;
    } else {
        emit addEmptyObject(DATA_POLYHEDRAL_MESH, output_pmo_id_);
        pmo = PluginFunctions::polyhedralMeshObject(output_pmo_id_);
        pmo->setName("IsoContouring result");
        pmo->setObjectDrawMode(
                  ACG::SceneGraph::DrawModes::getDrawMode("Edges (colored per edge)")
                  // You may want to comment out the next line if the vertices annoy you:
                | ACG::SceneGraph::DrawModes::getDrawMode("Vertices")
                , true);
        pmo->materialNode()->set_point_size(3); // you can tweak this for debug visualisation purposes
        pmo->materialNode()->set_line_width(3);
        return *pmo;
    }
}

void IsoContouringPlugin::slotUpdateIsoContours()
{
    // Open the "Data Control" toolbox (in the plugin list on the right hand side)
    // to see and adjust which objects are marked as "target"
    for (BaseObjectData * obj: PluginFunctions::objects(
                PluginFunctions::TARGET_OBJECTS,
                DATA_TRIANGLE_MESH))
    {
        TriMeshObject *tmo = PluginFunctions::triMeshObject(obj);
        sliceMesh(*tmo->mesh());
    }
}


void IsoContouringPlugin::slotObjectUpdated(int _identifier, const UpdateType& _type)
{
    const bool geometry_changed =
        _type.contains(UPDATE_ALL)
        || _type.contains(UPDATE_GEOMETRY)
        || _type.contains(UPDATE_TOPOLOGY);
    if (!geometry_changed) return;
    TriMeshObject *tmo = PluginFunctions::triMeshObject(_identifier);
    if (tmo == nullptr) { // not a trimesh
        return;
    }
    sliceMesh(*tmo->mesh());
}
