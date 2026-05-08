#include "MeshSmoothingPlugin.hh"
#include "Eigen/Core"
#include "OpenMesh/Core/Utils/Predicates.hh"
#include <ObjectTypes/TriangleMesh/PluginFunctionsTriangleMesh.hh>
#include <OpenMesh/Core/Utils/PropertyManager.hh>
#include <OpenFlipper/common/perObjectData.hh>
#include <ACG/Scenegraph/DrawModes.hh>

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <cmath>

#include <OpenMesh/Core/Mesh/Handles.hh>
#include <OpenMesh/Core/Geometry/VectorT.hh>
#include <OpenMesh/Core/Mesh/DefaultTriMesh.hh>

#define ASSIGNMENT_08_SOLUTION 1

using VH = OpenMesh::VertexHandle;
using EH = OpenMesh::EdgeHandle;
using FH = OpenMesh::FaceHandle;
using HEH = OpenMesh::HalfedgeHandle;
using OpenMesh::Vec3d;
using OpenMesh::TriMesh;
using Point = TriMesh::Point;
using ScalarHProp = OpenMesh::PropertyManager<OpenMesh::HPropHandleT<double>>;
using ScalarVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<double>>;
using SpMat = Eigen::SparseMatrix<double, Eigen::RowMajor>;


inline Eigen::Vector3d to_eigen(Vec3d const& v)
{
    return {v[0], v[1], v[2]};
}

inline Vec3d from_eigen(Eigen::Vector3d const& v)
{
    return {v[0], v[1], v[2]};
}

/// Compute cotan of triangle corner that heh points to (i.e. heh.to() in heh.face())
inline double compute_cotan(OpenMesh::TriMesh const& _mesh,
        Eigen::MatrixXd const &_positions,
        OpenMesh::HalfedgeHandle _heh)
{
    auto heh = _mesh.make_smart(_heh);
    const Eigen::Vector3d &p0 = _positions.row(heh.from().idx());
    const Eigen::Vector3d &p1 = _positions.row(heh.to().idx()); // this is the corner whose cotan we want
    const Eigen::Vector3d &p2 = _positions.row(heh.next().to().idx());

    const Eigen::Vector3d d0 = p2 - p1;
    const Eigen::Vector3d d1 = p0 - p1;
    // the_sin and the_cos are both scaled with ||d0||*||d1||, but this cancels out
    const double the_sin = d0.cross(d1).norm();
    const double the_cos = d0.dot(d1);
    const double cot = the_cos / the_sin;
    if (!std::isfinite(cot)) {
        std::cerr << "Invalid cotan weight on halfedge " << heh << std::endl;
        return 0.;
    }
    return cot;
}

/// Compute per-vertex areas for lumped mass matrix
inline Eigen::VectorXd computeVertexAreas(AreaFormulation _kind,
        OpenMesh::TriMesh const& _mesh,
        Eigen::MatrixXd const &_positions)
{
    Eigen::VectorXd vertex_area = Eigen::VectorXd::Zero(_mesh.n_vertices());

    for (const auto fh: _mesh.faces()) {
        auto heh0 = fh.halfedge();
        auto heh1 = heh0.next();
        auto heh2 = heh1.next();
        const auto v0 = heh0.to();
        const auto v1 = heh1.to();
        const auto v2 = heh2.to();
        const Eigen::Vector3d p0 = _positions.row(v0.idx());
        const Eigen::Vector3d p1 = _positions.row(v1.idx());
        const Eigen::Vector3d p2 = _positions.row(v2.idx());
        const Eigen::Vector3d e0 = p1 - p0;
        const Eigen::Vector3d e1 = p2 - p0;
        const double face_area = e0.cross(e1).norm() / 2;

        if (_kind == AreaFormulation::Barycentric) {
            vertex_area[v0.idx()] += face_area / 3.0;
            vertex_area[v1.idx()] += face_area / 3.0;
            vertex_area[v2.idx()] += face_area / 3.0;
        } else if (_kind == AreaFormulation::MixedVoronoi) {
            double l01 = (p1 - p0).squaredNorm();
            double l12 = (p2 - p1).squaredNorm();
            double l20 = (p0 - p2).squaredNorm();
            double cot0 = compute_cotan(_mesh, _positions, heh0); // cotan at heh0.to()
            double cot1 = compute_cotan(_mesh, _positions, heh1);
            double cot2 = compute_cotan(_mesh, _positions, heh2);
            if (cot0 < 0.0 || cot1 < 0.0 || cot2 < 0.0) {
                if (cot0 < 0.0) {
                    vertex_area[v0.idx()] += face_area / 2.0;
                    vertex_area[v1.idx()] += face_area / 4.0;
                    vertex_area[v2.idx()] += face_area / 4.0;
                } else if (cot1 < 0.0) {
                    vertex_area[v0.idx()] += face_area / 4.0;
                    vertex_area[v1.idx()] += face_area / 2.0;
                    vertex_area[v2.idx()] += face_area / 4.0;
                } else {
                    vertex_area[v0.idx()] += face_area / 4.0;
                    vertex_area[v1.idx()] += face_area / 4.0;
                    vertex_area[v2.idx()] += face_area / 2.0;
                }
            }else {
                vertex_area[v0.idx()] += (l01 * cot2 + l20 * cot1) / 8.0;
                vertex_area[v1.idx()] += (l12 * cot0 + l01 * cot2) / 8.0;
                vertex_area[v2.idx()] += (l20 * cot1 + l12 * cot0) / 8.0;
            }
        }
    }
    return vertex_area;
}



inline double compute_cotan_weight(OpenMesh::TriMesh const& _mesh,
        OpenMesh::EdgeHandle _eh,
        Eigen::MatrixXd const &_positions)
{
    auto eh = _mesh.make_smart(_eh);
    double w = 0;
    for (int subidx: {0, 1}) {
        auto heh = eh.h(subidx);
        if (heh.is_boundary()) continue;
        // heh.next() points at the vertex opposite of heh in heh.face
        w += compute_cotan(_mesh, _positions, heh.next());
    }
    return 0.5 * w;
}

inline Eigen::DiagonalMatrix<double, -1> setup_laplacian_diagonal(LaplacianWeights _kind,
        OpenMesh::TriMesh const& _mesh,
        Eigen::MatrixXd const &_positions) {
    Eigen::VectorXd vertex_weights;
    if (_kind == LaplacianWeights::Cotan) {
        Eigen::VectorXd vertex_areas = computeVertexAreas(AreaFormulation::MixedVoronoi, _mesh, _positions);
        vertex_weights = (2.0 * vertex_areas).cwiseInverse();
    } else {
        assert(_kind == LaplacianWeights::Uniform);
        vertex_weights.resize(_mesh.n_vertices());
        for (const auto vh : _mesh.vertices()) {
            vertex_weights[vh.idx()] = 1.0 / _mesh.valence(vh);
        }
    }

    return vertex_weights.asDiagonal();
}

inline SpMat setup_laplacian_coefficients(LaplacianWeights _kind,
        OpenMesh::TriMesh const& _mesh,
        Eigen::MatrixXd const &_positions) {
    SpMat L {static_cast<Eigen::Index>(_mesh.n_vertices()), static_cast<Eigen::Index>(_mesh.n_vertices())};
    std::vector<Eigen::Triplet<double>> triplets;

    // reserve: pre-allocate the right amount of memory to avoid re-allocation when adding elements to the vector
    triplets.reserve(_mesh.n_edges() * 2 + _mesh.n_vertices());

    for (OpenMesh::SmartVertexHandle current_vertex : _mesh.vertices()) {
        int current_vertex_idx = current_vertex.idx();
        
        double current_weight_sum = 0;
        for (OpenMesh::SmartHalfedgeHandle current_halfedge : current_vertex.outgoing_halfedges()) {
            OpenMesh::SmartVertexHandle current_neighbor = current_halfedge.to();

            double current_neighbor_weight;
            if (_kind == LaplacianWeights::Cotan) {
                current_neighbor_weight = compute_cotan_weight(_mesh, current_halfedge.edge(), _positions);
            } else {
                assert(_kind == LaplacianWeights::Uniform);
                current_neighbor_weight = 1;
            }

            double current_neighbor_value = current_neighbor_weight;

            triplets.push_back({current_vertex_idx, current_neighbor.idx(), current_neighbor_value});
            current_weight_sum += current_neighbor_value;
        }

        triplets.push_back({current_vertex_idx, current_vertex_idx, -current_weight_sum});
    }

    L.setFromTriplets(triplets.begin(), triplets.end());
    return L;
}

inline SpMat setup_laplacian(
        LaplacianWeights _kind,
        OpenMesh::TriMesh const& _mesh,
        Eigen::MatrixXd const &_positions)
{
    /*
        Constructs Laplacian matrix L for which it holds that
            L = DM
        with
            D       = diag(..., 1 / (2 * A_i))
        and
                        cotan weights for halfedge that connects i and j    if  i != j and j is neighbor of i
            M_ij    =   negative sum of all cotan weights                   if  i = j
                        0                                                   otherwise
    */
    SpMat L = setup_laplacian_coefficients(_kind, _mesh, _positions);
    Eigen::DiagonalMatrix<double, -1> D = setup_laplacian_diagonal(_kind, _mesh, _positions);

    return D * L;
}

/// Get mesh vertex positions as n x 3 matrix
Eigen::MatrixXd mesh_to_eigen(TriMesh const& mesh)
{
    Eigen::MatrixXd positions(mesh.n_vertices(), 3);
    for (const auto vh: mesh.vertices()) {
        positions.row(vh.idx()) = to_eigen(mesh.point(vh));
    }
    return positions;
}

/// Set mesh vertex positions from n x 3 matrix
void eigen_to_mesh(TriMesh& mesh, Eigen::MatrixXd const& positions)
{
    if (static_cast<Eigen::Index>(positions.rows()) != static_cast<Eigen::Index>(mesh.n_vertices())) {
        assert(false); throw std::runtime_error("Position matrix row count mismatch.");
    }
    for (const auto vh: mesh.vertices()) {
        mesh.point(vh) = from_eigen(positions.row(vh.idx()));
    }
}


class MeshSmoothingObjectData : public PerObjectData
{
public:
    /// Results of every iteration: Entry 0 is the original positions
    std::vector<Eigen::MatrixXd> position_history;

    MeshSmoothingObjectData(TriMesh& mesh) {
        reset(mesh);
    }
    void reset(TriMesh const &mesh) {
        position_history.resize(1);
        position_history[0] = mesh_to_eigen(mesh);
    }


    void invalidatePositionHistory() {
        position_history.resize(1);
    }
};

namespace PodNames {
    static const char* const smoothing_data = "Plugin-Ex08-Smoothing/SmoothingData";
}

MeshSmoothingObjectData* get_smoothing_data(TriMeshObject& object)
{
    return dynamic_cast<MeshSmoothingObjectData*>(object.objectData(PodNames::smoothing_data));
}

MeshSmoothingObjectData& get_or_create_smoothing_data(TriMeshObject& object)
{
    if (auto* data = get_smoothing_data(object)) {
        return *data;
    }
    auto* data = new MeshSmoothingObjectData(*object.mesh());
    object.setObjectData(PodNames::smoothing_data, data);
    return *data;
}

void normalize_laplacian_rows(SpMat& L)
{
    const Eigen::VectorXd diag = L.diagonal();
    Eigen::VectorXd row_scale_factors(diag.size());
    for (Eigen::Index i = 0; i < diag.size(); ++i) {
        const double d = diag[i];
        row_scale_factors[i] = (std::abs(d) > 1e-16) ? (-1.0 / d) : 1.0;
    }
    L = row_scale_factors.asDiagonal() * L;
}

template <typename Fn>
void for_each_target_trimesh(Fn&& fn)
{
    for (BaseObjectData * obj: PluginFunctions::objects(
                PluginFunctions::TARGET_OBJECTS,
                DATA_TRIANGLE_MESH))
    {
        TriMeshObject *tmo = PluginFunctions::triMeshObject(obj);
        if (!tmo) continue;
        fn(*tmo);
    }
}

void MeshSmoothingPlugin::initializePlugin()
{
    tool_ = new MeshSmoothingToolbar();

    connect(tool_, &MeshSmoothingToolbar::majorSettingsChanged, this, &MeshSmoothingPlugin::slotInvalidateHistory);
    connect(tool_, &MeshSmoothingToolbar::minorSettingsChanged, this, &MeshSmoothingPlugin::slotSettingsChanged);

    connect(tool_->btn_apply, &QPushButton::clicked, this, &MeshSmoothingPlugin::slotApply);
    connect(tool_->cb_preview, &QCheckBox::toggled, this, &MeshSmoothingPlugin::slotUpdatePreviewVisibility);

    emit addToolbox( tr("GP Ex08: Mesh Smoothing") , tool_ );
}

void MeshSmoothingPlugin::slotInvalidateHistory()
{
    for_each_target_trimesh([&](TriMeshObject& obj) {
        auto &pod = get_or_create_smoothing_data(obj);
        pod.invalidatePositionHistory();
    });
}
void MeshSmoothingPlugin::slotSettingsChanged()
{
    if (!tool_->show_preview()) return;
    const SmoothingSettings settings = tool_->getSettings();
    for_each_target_trimesh([&](TriMeshObject& obj) {
        smooth(obj, settings);
        emit updatedObject(obj.id(), UPDATE_GEOMETRY);
    });
}

void MeshSmoothingPlugin::slotUpdatePreviewVisibility()
{
    if (tool_->show_preview()) {
        slotSettingsChanged();
        return;
    } else {
        // User disabled preview, so we copy the saved original mesh positions into the mesh
        for_each_target_trimesh([&](TriMeshObject& tmo) {
            auto* pod = get_smoothing_data(tmo);
            if (!pod) return;
            eigen_to_mesh(*tmo.mesh(), pod->position_history[0]);
            tmo.mesh()->update_normals();
            emit updatedObject(tmo.id(), UPDATE_GEOMETRY);
        });
    }
}
void MeshSmoothingPlugin::slotApply()
{
    const SmoothingSettings settings = tool_->getSettings();
    for_each_target_trimesh([&](TriMeshObject& tmo) {
        smooth(tmo, settings); // ensure all required iterations are computed and applied to mesh
        auto* pod = get_smoothing_data(tmo);
        pod->reset(*tmo.mesh());
        emit updatedObject(tmo.id(), UPDATE_GEOMETRY);
    });
}

/// Compute mesh volume, but use (n x 3) positions matrix for vertex
/// positions instead of the native mesh vertex positions.
static double compute_mesh_volume(TriMesh const& mesh, Eigen::MatrixXd const& positions)
{
    double total_volume = 0;

    for (OpenMesh::SmartFaceHandle face : mesh.faces()) {
        Eigen::MatrixXd tetrahedron_volume_matrix(3, 3);

        int vertex_idx = 0;
        for (OpenMesh::SmartVertexHandle current_face_vertex : face.vertices()) {
            auto current_vertex_position = positions.row(current_face_vertex.idx());

            // We use the origin as the reference point, which means that the coordinates alone
            // already describe the edges from the triangle to the reference point (to form the tetrahedron).
            tetrahedron_volume_matrix.row(vertex_idx) = current_vertex_position;

            vertex_idx++;
        }

        double face_tetrahedron_volume = tetrahedron_volume_matrix.determinant();
        total_volume += face_tetrahedron_volume;
    }

    return 1.0 / 6.0 * total_volume;
}

static Eigen::RowVector3d average_position(TriMesh const& mesh, Eigen::MatrixXd const &_positions)
{
    return _positions.colwise().sum() / _positions.rows();
#if 0
    // Alternative implementation:
    Eigen::RowVector3d sum = Eigen::RowVector3d::Zero();
    for (auto vh: mesh.vertices()) {
        sum += _positions.row(vh.idx());
    }
    return sum / mesh.n_vertices();
#endif
}

/// Parameter positions is of shape n x 3, where n is the number of vertices in mesh
/// Adjust `positions` to match the given volume.
static void apply_volume_preservation(TriMesh const& mesh,
                                      Eigen::MatrixXd& positions,
                                      double vol_before,
                                      Eigen::RowVector3d center_before)
{
    double volume_current = compute_mesh_volume(mesh, positions);
    double scaling_factor = vol_before / volume_current;
    double adjusted_scaling_factor = std::cbrt(scaling_factor);

    // Equivalent to the following per-vertex update when using the origin as the reference point:
    //  vertex_position = scaling-factor * (vertex_position - reference_point) + reference_point
    positions *= adjusted_scaling_factor;

    Eigen::RowVector3d new_center = average_position(mesh, positions);
    auto position_comp = center_before - new_center;

    positions.rowwise() += position_comp;
}

bool has_boundary(TriMesh const &mesh) {
    bool has_boundary = false;
    for (const auto heh: mesh.halfedges()) {
        if (mesh.is_boundary(heh)) return true;
    }
    return false;
}

/// extrapolate smoothing to exaggerate high frequencies
/// alpha = 0: keep smoothed positions
/// alpha = 1: original mesh positions
/// alpha > 1: extrapolate
static Eigen::MatrixXd apply_feature_enhancement(
                                      Eigen::MatrixXd const& original_positions,
                                      Eigen::MatrixXd const& smoothed_positions,
                                      double alpha)
{
    // 'original_positions - smoothened_positions' represents high-frequency features
    return smoothed_positions + alpha * (original_positions - smoothed_positions);
}

void MeshSmoothingPlugin::smooth(
    TriMeshObject& obj,
    SmoothingSettings const &settings)
{
    auto& pod = get_or_create_smoothing_data(obj);
    TriMesh& mesh = *obj.mesh();

    if (settings.method == SmoothingMethod::Explicit) {
        smooth_explicit(obj, settings);
    } else {
        bool success = smooth_implicit(obj, settings);
        if (!success) return;
    }

    const auto &orig_pos = pod.position_history.at(0);
    auto new_pos = pod.position_history.at(settings.iterations);

    auto mesh_volume = compute_mesh_volume(mesh, orig_pos);
    std::cout << "volume: " << mesh_volume << std::endl;

    if (settings.preserve_volume && !has_boundary(mesh)) {
        // volume is not well-defined on an open mesh,
        // so we silently ignore volume preservation if there is a boundary.
        auto vol_before = compute_mesh_volume(mesh, orig_pos);
        Eigen::Vector3d center_before = average_position(mesh, orig_pos);
        apply_volume_preservation(mesh, new_pos, vol_before, center_before);

        auto vol_after = compute_mesh_volume(mesh, new_pos);
        std::cout << "volume    before: " << vol_before << "    after: " << vol_after << "  diff:" << (vol_after - vol_before) << std::endl;
    }

    if (settings.feature_enhance_alpha != 0) {
        Eigen::MatrixXd enhanced = apply_feature_enhancement(orig_pos, new_pos, settings.feature_enhance_alpha);
        eigen_to_mesh(mesh, enhanced);
    } else {
        eigen_to_mesh(mesh, new_pos);
    }


    mesh.update_normals();
}

void MeshSmoothingPlugin::smooth_explicit(
    TriMeshObject& obj,
    SmoothingSettings const & settings)
{
    auto& pod = get_or_create_smoothing_data(obj);
    TriMesh& mesh = *obj.mesh();

    const std::size_t base_iter = pod.position_history.size() - 1;

    if (base_iter >= settings.iterations) {
        // nothing to do, all required iterations are precomputed.
        return;
    }

    pod.position_history.reserve(settings.iterations + 1);

    for (std::size_t iter = base_iter; iter < settings.iterations; ++iter) {
        Eigen::MatrixXd cur_pos = pod.position_history.back();
        SpMat L = setup_laplacian(settings.laplacian, mesh, cur_pos);
        if (tool_->cb_normalized_lap->isChecked()) {
            normalize_laplacian_rows(L);
        }

        cur_pos = cur_pos + settings.timestep * (L * cur_pos);
        pod.position_history.push_back(cur_pos);
    }
}

bool MeshSmoothingPlugin::smooth_implicit(
    TriMeshObject& obj,
    SmoothingSettings const & settings)
{
    auto& pod = get_or_create_smoothing_data(obj);

    const std::size_t base_iter = pod.position_history.size() - 1;

    if (base_iter >= settings.iterations) {
        // nothing to do, all required iterations are precomputed.
        return true;
    }

    TriMesh& mesh = *obj.mesh();

    const Eigen::MatrixXd orig_vpos = pod.position_history.at(0);


    pod.position_history.reserve(settings.iterations + 1);
    for (std::size_t iter = base_iter; iter < settings.iterations; ++iter) {
        // We set up a new system in every iteration on purpose.
        // Usually you will use one a single iteration (or only few) for implicit smoothing,
        // but this is useful for experimentation.
        Eigen::MatrixXd const &cur_pos = pod.position_history.at(iter);
        Eigen::MatrixXd new_positions;

        Eigen::DiagonalMatrix<double, -1> laplacian_diagonal = setup_laplacian_diagonal(settings.laplacian, mesh, cur_pos);
        std::cout << "calculating inverse diagonal now" << std::endl;
        
        auto diag_vector = laplacian_diagonal.diagonal().cwiseInverse();
        
        SpMat laplacian_coefficients = setup_laplacian_coefficients(settings.laplacian, mesh, cur_pos);

        SpMat system = -(settings.timestep * laplacian_coefficients);
        system += diag_vector.asDiagonal();

        std::cout << "calculating b now" << std::endl;
        Eigen::MatrixXd b = diag_vector.asDiagonal() * cur_pos;

        Eigen::SimplicialLLT decomposition(system);
        std::cout << "solving now" << std::endl;
        Eigen::MatrixXd result = decomposition.solve(b);
        Eigen::ComputationInfo t = decomposition.info();
        
        std::cout <<"finished now!" << std::endl;
        // TODO:
        //  - Set up symmetric(!) system matrix A (symmetry is important for efficient decomposition/solve)
        //     - Use cur_pos to compute the laplacian matrix.
        //  - Compute the right hand side b
        //  - Solve Ax = b using a suitable sparse linear solver from Eigen's offerings
        //      - You may want to try different solvers and see which one performs best for your test inputs
        //  - Store the new x in new_positions
        //
        new_positions = result;

        pod.position_history.push_back(new_positions);
    }
    return true;
}

void MeshSmoothingPlugin::slotObjectUpdated(int _identifier, const UpdateType& _type)
{
    BaseObject *obj = nullptr;
    // if the object's topology was updated, we have to discard all our intermediate results
    if (!_type.contains(UPDATE_TOPOLOGY)) return;
    PluginFunctions::getObject(_identifier, obj);
    obj->clearObjectData(PodNames::smoothing_data);
}
