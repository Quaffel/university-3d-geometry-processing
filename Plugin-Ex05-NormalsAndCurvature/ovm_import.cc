#include "NormalsAndCurvaturePlugin.hh"
#include <ObjectTypes/PolyhedralMesh/PluginFunctionsPolyhedralMesh.hh>
#include <ObjectTypes/TriangleMesh/PluginFunctionsTriangleMesh.hh>

void NormalsAndCurvaturePlugin::slotImportOVMSurfaces()
{
    // You can ignore this helper function, it's not part of the exercise and
    // only exists so you can import your curve smoothing trajectories.
    using OpenVolumeMesh::HalfFaceHandle;
    using OpenVolumeMesh::VertexHandle;

    int count = 0;
    for (BaseObjectData *obj : PluginFunctions::objects(
                PluginFunctions::TARGET_OBJECTS,
                DATA_POLYHEDRAL_MESH))
    {
        auto *pmo = PluginFunctions::polyhedralMeshObject(obj);
        if (!pmo) continue;

        PolyhedralMesh &mesh = *pmo->mesh();
        mesh.enable_edge_bottom_up_incidences();
        mesh.enable_face_bottom_up_incidences();

        auto visited_face = mesh.create_private_face_property("visited_face", false);
        std::vector<HalfFaceHandle> stack;
        std::vector<HalfFaceHandle> component;

        for (const auto hfh : mesh.halffaces()) {
            if (!mesh.is_boundary(hfh)) continue;
            const auto fh = mesh.face_handle(hfh);
            if (!fh.is_valid() || visited_face[fh]) continue;

            stack.clear();
            component.clear();
            stack.push_back(hfh);

            while (!stack.empty()) {
                auto cur = stack.back();
                stack.pop_back();
                const auto cur_fh = mesh.face_handle(cur);
                if (!cur_fh.is_valid() || visited_face[cur_fh]) continue;
                visited_face[cur_fh] = true;
                component.push_back(cur);

                for (OpenVolumeMesh::BoundaryHalfFaceHalfFaceIter it(cur, &mesh);
                     it.valid(); ++it)
                {
                    const auto neigh_fh = mesh.face_handle(*it);
                    if (!neigh_fh.is_valid() || visited_face[neigh_fh]) continue;
                    stack.push_back(*it);
                }
            }

            int obj_id = BaseObject::NOOBJECT;
            emit addEmptyObject(DATA_TRIANGLE_MESH, obj_id);
            auto *tmo = PluginFunctions::triMeshObject(obj_id);
            tmo->setName(QString("Imported surface %1").arg(++count));
            tmo->target(true);

            TriMesh &tri = *tmo->mesh();

            auto vmap = mesh.create_private_vertex_property<TriMesh::VertexHandle>("", TriMesh::InvalidVertexHandle);

            auto get_or_add_vertex = [&](VertexHandle vh) -> TriMesh::VertexHandle {
                TriMesh::VertexHandle &tri_vh = vmap[vh];
                if (!tri_vh.is_valid()) {
                    tri_vh = tri.add_vertex(mesh.vertex(vh));
                }
                return tri_vh;
            };

            std::vector<TriMesh::VertexHandle> face_vertices;
            for (const auto comp_hfh : component) {
                face_vertices.clear();
                for (const auto vh : mesh.halfface_vertices(comp_hfh)) {
                    face_vertices.push_back(get_or_add_vertex(vh));
                }
                tri.add_face(face_vertices);
            }

            tri.update_normals();
            emit updatedObject(tmo->id(), UPDATE_ALL);
        }
    }

    if (count == 0) {
        emit log(LOGINFO, "No surface components found in target OVM meshes.");
    }
}

