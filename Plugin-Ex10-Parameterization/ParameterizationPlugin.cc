#include "ParameterizationPlugin.hh"
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <cmath>
#include <cassert>
#include <limits>

#include "ui_ParameterizationToolbarBase.h"

#define ASSIGNMENT_10_SOLUTION 1

using Vec2d = ACG::Vec2d;
using Vec2f = ACG::Vec2f;
using Vec3d = ACG::Vec3d;
using Vec4f = ACG::Vec4f;
/* ===== BEGIN: This part is mostly copied from Plugin-Ex08-Smoothing ===== */

using VH = OpenMesh::VertexHandle;
using EH = OpenMesh::EdgeHandle;
using FH = OpenMesh::FaceHandle;
using HEH = OpenMesh::HalfedgeHandle;
using OpenMesh::Vec3d;
using OpenMesh::TriMesh;
using Point = TriMesh::Point;
using ScalarEProp = OpenMesh::PropertyManager<OpenMesh::EPropHandleT<double>>;
using ScalarVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<double>>;
using BooleanVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<bool>>;
using IndexVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<Eigen::Index>>;
using SpMat = Eigen::SparseMatrix<double, Eigen::RowMajor>;

/// A property holding an N-dimensional eigen row vector per vertex
template<int N>
using VectorVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<Eigen::RowVector<double, N>>>;


enum class LaplacianWeights {
    Uniform,
    Cotan,
};


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

/// Get mesh vertex positions as n x 3 matrix
Eigen::MatrixXd mesh_to_eigen(TriMesh const& mesh)
{
    Eigen::MatrixXd positions(mesh.n_vertices(), 3);
    for (const auto vh: mesh.vertices()) {
        positions.row(vh.idx()) = to_eigen(mesh.point(vh));
    }
    return positions;
}

/* ===== END: This part is copied from Plugin-Ex08-Smoothing ===== */


double g_radius = 1;
// Initialize vertex texture coordinates by mapping the (single!)
// boundary to the unit circle.
// Return true on success, false on failure (e.g. #boundaries != 1)
bool map_surface_boundary_to_circle(TriMesh &_mesh) {
    _mesh.request_vertex_status(); // ensure "selected" is available. (not necessary inside OF, but when used standalone)
    _mesh.request_vertex_texcoords2D();
    for(auto vh: _mesh.vertices()) {
        _mesh.set_texcoord2D(vh, Vec2f(0.f, 0.f));
        _mesh.status(vh).set_selected(false); // we use the `selected` status to denote fixed vertices
    }

    std::vector<std::pair<OpenMesh::SmartHalfedgeHandle, double>> boundary;
    double total_boundary_length = 0;

    for (OpenMesh::SmartHalfedgeHandle start_boundary_halfedge : _mesh.halfedges()) {
        if (!start_boundary_halfedge.is_boundary()) {
            continue;
        }

        if (_mesh.status(start_boundary_halfedge.to()).selected()) {
            continue;
        }

        if (!boundary.empty()) {
            std::cerr << "found more than one boundary" << std::endl;
            return false;
        }

        OpenMesh::SmartHalfedgeHandle current_boundary_halfedge = start_boundary_halfedge;
        do {
            _mesh.status(current_boundary_halfedge).set_selected(true);
            _mesh.status(current_boundary_halfedge.to()).set_selected(true);

            double halfedge_length = _mesh.calc_edge_length(current_boundary_halfedge);

            total_boundary_length += halfedge_length;
            boundary.emplace_back(current_boundary_halfedge, halfedge_length);

            current_boundary_halfedge = current_boundary_halfedge.next();
            assert(current_boundary_halfedge.is_boundary());
        } while (current_boundary_halfedge.idx() != start_boundary_halfedge.idx());
    }

    if (boundary.empty()) {
        std::cerr << "found no boundary" << std::endl;
        return false;
    }

    double radiant_length_ratio = 2.0 * M_PI / total_boundary_length;
    double current_angle = 0;
    for (auto [current_boundary_halfedge, current_boundary_halfedge_length] : boundary) {
        OpenMesh::SmartVertexHandle current_vertex = current_boundary_halfedge.to();
        _mesh.set_texcoord2D(
            current_vertex, 
            Vec2f(g_radius * std::sin(current_angle), g_radius * std::cos(current_angle))
        );

        current_angle += current_boundary_halfedge_length * radiant_length_ratio;
    }

    return true;
}

ScalarEProp compute_uniform_laplacian_weights(OpenMesh::TriMesh const& _mesh)
{
    ScalarEProp weight = OpenMesh::makeTemporaryProperty<EH, double>(_mesh);
    weight.set_range(_mesh.edges(), 1.);
    return weight;
}
ScalarEProp compute_cotan_laplacian_weights(OpenMesh::TriMesh const& _mesh)
{
    ScalarEProp weight = OpenMesh::makeTemporaryProperty<EH, double>(_mesh);
    const Eigen::MatrixXd pos = mesh_to_eigen(_mesh); // this copy is not very useful, but allows us to re-use more of the previous exercise code
    for (const auto eh: _mesh.edges()) {
        weight[eh] = compute_cotan_weight(_mesh, eh, pos);
    }
    return weight;
}

ScalarEProp compute_laplacian_weights(
        LaplacianWeights _kind,
        OpenMesh::TriMesh const& _mesh)
{
    if (_kind == LaplacianWeights::Uniform)
        return compute_uniform_laplacian_weights(_mesh);
    assert(_kind == LaplacianWeights::Cotan);
    return compute_cotan_laplacian_weights(_mesh);
}


inline SpMat setup_laplacian(
        OpenMesh::TriMesh const& _mesh,
        ScalarEProp const& _edge_weights,
        IndexVProp const& _sys_idx // map vertex handles to system indices
        )
{
    std::vector<Eigen::Triplet<double>> triplets;

    // reserve: pre-allocate the right amount of memory to avoid re-allocation when adding elements to the vector
    triplets.reserve(_mesh.n_edges() * 2 + _mesh.n_vertices());
    const size_t n = _mesh.n_vertices();
    auto L = SpMat(n, n);

    // ==== TODO: your code here: *permuted* Laplacian matrix assembly ===
    // fill in triplets to set up the Laplacian matrix using the given edge weights,
    // using explicitly specified system indices (what row/column a given vertex handle corresponds to)
    // NOTE: this is similar, but *not* identical to the smoothing exercise,
    // as we do not have a 1:1 mapping of vertex handles and system indices anymore.
    // (_sys_idx defines a permutation)

    L.setFromTriplets(triplets.begin(), triplets.end());
    return L;
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

static const std::string pickmode_name = "PoissonConstraint";

struct ParamConfig {
    LaplacianWeights lap_type = LaplacianWeights::Uniform;
};

class ParameterizationToolbar : public QWidget, public Ui::ParameterizationToolbarBase
{
    public:
    ParameterizationToolbar(QWidget * parent = nullptr)
        : QWidget(parent)
    {
        setupUi(this);
    }
    ParamConfig get_config() const {
        return ParamConfig {
            .lap_type = get_lap_type()
        };
    }
    LaplacianWeights get_lap_type() const {
        if (this->rb_lap_cotan->isChecked()) {
            return LaplacianWeights::Cotan;
        }
        if (this->rb_lap_uniform->isChecked()) {
            return LaplacianWeights::Uniform;
        }
        // optional TODO: you could implement floater's mean value coordinates:
        // https://cgvr.informatik.uni-bremen.de/teaching/cg_literatur/barycentric_floater.pdf
        assert(false); return LaplacianWeights::Uniform; // failsafe
    }
};

void ParameterizationPlugin::initializePlugin()
{
    tool_ = new ParameterizationToolbar();
    connect(tool_->pb_constrain_0, &QPushButton::clicked, [&]() { slotConstraintButton(0);});
    connect(tool_->pb_constrain_1, &QPushButton::clicked, [&]() { slotConstraintButton(1);});

    connect(tool_->pb_param_init, &QPushButton::clicked, this, &ParameterizationPlugin::slotParamInit);
    connect(tool_->pb_param_relax_explicit, &QPushButton::clicked, [this](){slotParamRelax(RelaxationType::Explicit);});
    connect(tool_->pb_param_relax_implicit, &QPushButton::clicked, [this](){slotParamRelax(RelaxationType::Implicit);});
    emit addToolbox( tr("GP Ex10: Parameterization") , tool_ );
}
void ParameterizationPlugin::pluginsInitialized()
{
    emit addPickMode(pickmode_name);
    emit addTexture( "HarmonicFunc" , "distance.png" , 2 );
    emit setTextureMode("HarmonicFunc","abs=false,clamp=false,repeat=false,center=false,scale=false");

    if (1) {
        // This texture was generated using https://uvchecker.atlux.one/ :
        emit addTexture( "HarmonicParam" , "CustomUVChecker_byValle_4K.png", 2 );
        g_radius = 1;
    } else {
        // more basic texture:
        emit addTexture( "HarmonicParam" , "quadTexture.png", 2 );
        g_radius = 10;
    }
    emit setTextureMode("HarmonicParam","abs=false,clamp=false,repeat=false,center=false,scale=false");
}

void ParameterizationPlugin::slotConstraintButton(int which)
{
    if (which == 0 && tool_->pb_constrain_0->isChecked()) {
        tool_->pb_constrain_1->setChecked(false);
    }
    if (which == 1 && tool_->pb_constrain_1->isChecked()) {
        tool_->pb_constrain_0->setChecked(false);
    }

    if ( tool_->pb_constrain_0->isChecked()
            || tool_->pb_constrain_1->isChecked())
    {
        PluginFunctions::pickMode(pickmode_name);
        PluginFunctions::actionMode(Viewer::ActionMode::PickingMode);
    } else {
        PluginFunctions::actionMode(Viewer::ActionMode::ExamineMode);
    }
}

struct ConstrainedPoissonSolver {
    ConstrainedPoissonSolver(TriMesh const&mesh,
            ScalarEProp const &edge_weights,
            BooleanVProp const& _is_constrained)
        : mesh_(mesh)
        , is_constrained_prop_{_is_constrained}
        , sys_idx_prop_{OpenMesh::makeTemporaryProperty<VH, Eigen::Index>(mesh)}
    {
        // === TODO: fill sys_idx_prop_ ====
        // Set up sys_idx_prop_ as a permutation of vertex handles, such that
        // the system variables [0.. n_variables_-1] correspond to the set of non-constrained vertices,
        // and [n_variables_...system_size_-1] are the constrained vertices.
        // This way we obtain a nice block structured matrix as shown in the lecture.

        SpMat full_system = setup_laplacian(mesh, edge_weights, sys_idx_prop_);

        // === TODO: extract sub-matrices ====
        // With the right permutation, we now have the following block structure in full_system:
        // [ A B ]    [u_var]   [b_var]
        // [ C D ]  * [u_fix] = [b_fix]
        //
        // full_system * full_u = full_rhs
        //
        //  To solve it, we handle the constrained variables in the following way:
        //  We implicitly (i.e., we pretend, but do not actually change the matrix)
        //  that C = 0 and D = I (identity matrix).
        //
        //  We want to end up with the following _reduced_ system:
        //
        //  A              * u_var     = b_var - B * u_fix
        //
        //  we use the following variables names in this code:
        //
        //  submatA_       * reduced_u = reduced_rhs
        //
        // Eigen offers some nice APIs to extract sub-matrices from SparseMatrix objects:
        // .block(row_start, col_start, n_rows, n_cols)
        // or .{top,bottom}{Left,Right}Corner(...)
        //
        // Use them to extract the sub-matrices
        //  submatA_ =      A (in the slides: \Delta_{nxn})
        //  submatB_ =      B (in the slides: \Delta_{nxm})
        //
        //  and use them to compute reduced_rhs.
        //
        // Note that if [A, B; C, D] is symmetric (or positive definite), so is A.

        solver_.compute(submatA_);
        if (solver_.info() != Eigen::Success) {
            std::cerr << "factorization failed" << std::endl;
        }
    }
    Eigen::ComputationInfo info() const {
        return solver_.info();
    }


    template<int N=1>
    VectorVProp<N> solve(VectorVProp<N> &rhs_prop) {

        Eigen::MatrixXd rhs_full = Eigen::MatrixXd::Zero(mesh_.n_vertices(), N);
        auto full_u = OpenMesh::makeTemporaryProperty<VH, Eigen::RowVector<double, N>>(mesh_);

        Eigen::MatrixXd reduced_rhs;
        // === TODO: your code here: compute reduced RHS using submatB_.
        // - First fill in rhs_full from the sparse constraints in rhs_prop by
        //   applying the sys_idx_prop_ permutation. Only copy values where
        //   vertices are constrained, leave the rest at zero.
        //
        // - Then compute reduced_rhs.
        //
        // Note: N can be > 1 (when we solve for multiple RHS at once), ensure
        // that your code can handle this.
        assert(reduced_rhs.rows() == submatA_.rows());

        // Note that even if we don't use it in this exercise, we could re-solve with
        // a different RHS, e.g. the same constrained vertices constraint to *different* values.

        const Eigen::MatrixXd reduced_u = solver_.solve(reduced_rhs);
        // these prints were useful for some debugging:
        //std::cerr << "solved." << std::endl;
        //std::cerr << "rhs min." << reduced_rhs.minCoeff() << std::endl;
        //std::cerr << "rhs max." << reduced_rhs.maxCoeff() << std::endl;
        //std::cerr << "min." << reduced_u.minCoeff() << std::endl;
        //std::cerr << "max." << reduced_u.maxCoeff() << std::endl;

        // === TODO: your code here: extend reduced solution and map to mesh ===
        // extend the reduced solution `reduced_u` to a full solution and store it in `full_u`.
        // Important: you need to undo the sys_idx_prop_ permutation!!
        // The full solution itself still is using system indices,
        // we need to save the values in full_u[vh] for the correct vertex handles.
        // (Tip: iterate over mesh vertices, not over system indices)
        return full_u;
    }

    TriMesh const& mesh_;
    BooleanVProp is_constrained_prop_;
    IndexVProp sys_idx_prop_;
    Eigen::Index system_size_ = 0;
    Eigen::Index n_variables_ = 0; // number of unconstrained vertices
    Eigen::Index n_constraints_ = 0; // number of constrained vertices
    SpMat submatA_;
    Eigen::SimplicialLDLT<SpMat> solver_;
    SpMat submatB_;
};


void ParameterizationPlugin::computeHarmonicScalarFunction()
{
    for_each_target_trimesh([&](TriMeshObject &tmo) {
        auto &mesh = *tmo.mesh();

        std::map<VH, double> fixed_vars;

        constexpr int N = 1;
        using RHSEntry = Eigen::RowVector<double, N>;
        auto rhs = OpenMesh::makeTemporaryProperty<VH, RHSEntry>(mesh);
        rhs.set_range(mesh.vertices(), RHSEntry::Zero());

        auto is_constrained = OpenMesh::makeTemporaryProperty<VH, bool>(mesh);
        is_constrained.set_range(mesh.vertices(), false);
        for (auto [mprop_name, value]: {
                std::make_pair("constraint_0", 0),
                std::make_pair("constraint_1", 1)})
        {
            auto vh = *OpenMesh::getOrMakeProperty<OpenMesh::MeshHandle, VH>(mesh, mprop_name);
            if (!mesh.is_valid_handle(vh)) continue;
            fixed_vars[vh] = value;
            is_constrained[vh] = true;
            rhs[vh] = RHSEntry(value); // turn scalar into 1-element vector
        }

        //const auto edge_weights = compute_laplacian_weights(LaplacianWeights::Cotan, mesh);
        const auto edge_weights = compute_laplacian_weights(tool_->get_lap_type(), mesh);
        auto solver = ConstrainedPoissonSolver(mesh, edge_weights, is_constrained);

        if (solver.info() != Eigen::Success) {
            std::cerr << "factorization failed, aborting." << std::endl;
            return;
        }

        auto u = solver.solve(rhs);

        mesh.request_vertex_texcoords2D();
        for (const auto vh: mesh.vertices()) {
            mesh.set_texcoord2D(vh, {float(u[vh][0]), 0.f});
        }
        tmo.setObjectDrawMode(
                ACG::SceneGraph::DrawModes::SOLID_TEXTURED_SHADED
                | ACG::SceneGraph::DrawModes::WIREFRAME, true);
        emit switchTexture("HarmonicFunc", tmo.id());
        emit updatedObject(tmo.id(), UPDATE_ALL);
    });
}

void ParameterizationPlugin::slotMouseEvent(QMouseEvent* _event)
{
    if (PluginFunctions::actionMode() != Viewer::PickingMode) return;
    if (PluginFunctions::pickMode() != pickmode_name) return;
    if (_event->button() != Qt::LeftButton) return;
    if (_event->type() == QMouseEvent::MouseButtonRelease) return; // we only handle down

    size_t node_idx, target_idx;
    ACG::Vec3d hit_point;
    // PICK_VERTEX would require the user to click very precisely. instead, we pick a face
    // and find the nearest vertex.
    if (!PluginFunctions::scenegraphPick(ACG::SceneGraph::PICK_FACE, _event->pos(),
                node_idx, target_idx, &hit_point)) {
        return;
    }

    BaseObjectData *obj;
    if (!PluginFunctions::getPickedObject(node_idx, obj)) {
        return;
    }
    // is picked object a triangle mesh?
    TriMeshObject *tri_obj = PluginFunctions::triMeshObject(obj);
    if (!tri_obj) return;

    auto &mesh = *tri_obj->mesh();
    auto fh = mesh.make_smart(OpenMesh::FaceHandle(target_idx));
    mesh.is_valid_handle(fh);
    if (!mesh.is_valid_handle(fh)) return;
    auto closest_vh = fh.vertices().argmin([&](OpenMesh::VertexHandle vh) {
            return (hit_point - tri_obj->mesh()->point(vh)).norm();
            });

    const char * mprop_name = nullptr;
    if (tool_->pb_constrain_0->isChecked()) {
        mprop_name = "constraint_0";
    } else if (tool_->pb_constrain_1->isChecked()) {
        mprop_name = "constraint_1";
    } else {
        return;
    }
    auto constraint_mprop = OpenMesh::getOrMakeProperty<OpenMesh::MeshHandle, VH>(mesh, mprop_name);
    *constraint_mprop = closest_vh;
    emit log ("Constraining vertex " + QString::number(closest_vh.idx()));
    computeHarmonicScalarFunction();
    emit updateView();
}
struct EdgeWeights {
    EdgeWeights(TriMesh& _mesh)
        : mesh_(_mesh)
        , edge_weights_uniform_(compute_uniform_laplacian_weights(_mesh))
        , edge_weights_cotan_(compute_cotan_laplacian_weights(_mesh))
    {
    }
    ScalarEProp const& get_weights(LaplacianWeights _type) const {
        if (_type == LaplacianWeights::Cotan)
            return edge_weights_cotan_;
        assert (_type == LaplacianWeights::Uniform);
        return edge_weights_uniform_;
    }

    TriMesh const &mesh_;
    ScalarEProp edge_weights_uniform_;
    ScalarEProp edge_weights_cotan_;
};

struct ParamPOD : public PerObjectData {
    ParamPOD(TriMesh& _mesh)
        : mesh_(&_mesh)
        , lap_weights_(_mesh)
    {
        auto total_area = mesh_->faces().sum([&](FH fh){return mesh_->calc_face_area(fh);});
        vis_scale_ = sqrt(total_area / M_PI) / g_radius;
    }
    bool init()
    {
        mesh_->request_vertex_texcoords2D();
        auto success = map_surface_boundary_to_circle(*mesh_);
        return success;
    }
    // Direct solve
    void relax_implicit()
    {
        constexpr int N = 2;
        using RHSEntry = Eigen::RowVector<double, N>;
        auto rhs = OpenMesh::makeTemporaryProperty<VH, RHSEntry>(*mesh_);

        const auto &edge_weights = lap_weights_.get_weights(config_.lap_type);
        auto is_constrained = OpenMesh::makeTemporaryProperty<VH, bool>(*mesh_);
        for (const auto vh: mesh_->vertices()) {
            is_constrained[vh] = vh.selected();
            if (vh.selected()) {
                const auto &uv = mesh_->texcoord2D(vh);
                rhs[vh](0) = uv[0];
                rhs[vh](1) = uv[1];
            }
        }
        auto solver = ConstrainedPoissonSolver(*mesh_, edge_weights, is_constrained);

        if (solver.info() != Eigen::Success) {
            std::cerr << "factorization failed, aborting." << std::endl;
            return;
        }

        auto u = solver.solve(rhs);

        for (const auto vh: mesh_->vertices()) {
            if (vh.selected()) continue;
            TriMesh::TexCoord2D uv;
            uv[0] = u[vh](0);
            uv[1] = u[vh](1);
            mesh_->set_texcoord2D(vh, uv);
        }
    }
    void set_config(ParamConfig const&_config) {
        config_ = _config;
    }
    void relax_explicit()
    {
        const auto &edge_weights = lap_weights_.get_weights(config_.lap_type);
        // ===== TODO: your code here ======
        // Implement a Gauss-Seidel style relaxation of vertex texture coordinates:
        // For all non-constrained vertices, move them to the weighted average
        // of their neighbor's texture coordinate positions, using already-updated
        // values where available. (This differs from how we implemented explicit smoothing:
        // there we were careful to only use the values of the previous iteration!)
    }
    void update_vis_obj() {
        for (const auto vh: mesh_->vertices()) {
            const auto &uv = mesh_->texcoord2D(vh);
            vis_mesh_->point(vh) = vis_origin + Vec3d{
                static_cast<double>(uv[0]) * vis_scale_,
                static_cast<double>(uv[1]) * vis_scale_,
                0.};
        }
        vis_mesh_->update_normals();
    }

    TriMesh *mesh_;
    ParamConfig config_;
    EdgeWeights lap_weights_;
    double total_area_ = 0;
    double vis_scale_ = 1;
    TriMesh *vis_mesh_;
    int tex_vis_obj_id = -1;
public:
    Vec3d vis_origin = Vec3d{0.}; // Sorry for this hack. But the vis is nicer.
};
namespace PodNames {
    static const char* const param = "Plugin-Ex10-Parameterization/ParamPOD";
}

ParamPOD* get_or_create_param_pod(TriMeshObject& object)
{
    if (auto *pod = dynamic_cast<ParamPOD*>(object.objectData(PodNames::param))) {
        return pod;
    }
    auto *pod = new ParamPOD(*object.mesh());
    if (!pod->init()) {
        delete pod;
        return nullptr;
    }
    object.setObjectData(PodNames::param, pod);
    return pod;
}

void ParameterizationPlugin::slotParamInit()
{
    for_each_target_trimesh([&](TriMeshObject &tmo) {
        auto *pod = get_or_create_param_pod(tmo);
        if (!pod) {
            emit log(LOGERR, "failed to init 2d param");
            return;
        }
        pod->set_config(tool_->get_config());
        pod->init();
        ensure_vis_object(tmo);

        pod->update_vis_obj();
        emit updatedObject(tmo.id(), UPDATE_ALL);
        emit updatedObject(pod->tex_vis_obj_id, UPDATE_ALL);
    });
}
void ParameterizationPlugin::slotParamRelax(RelaxationType _relax_type)
{
    for_each_target_trimesh([&](TriMeshObject &tmo) {
        auto *pod = get_or_create_param_pod(tmo);
        if (!pod) {
            emit log(LOGERR, "failed to init 2d param");
            return;
        }
        pod->set_config(tool_->get_config());
        ensure_vis_object(tmo);
        if (_relax_type == RelaxationType::Explicit) {
            int n_steps = tool_->sb_steps->value();
            for (int i = 0; i < n_steps; ++i) {
                pod->relax_explicit();
            }
        }
        if (_relax_type == RelaxationType::Implicit) {
            pod->relax_implicit();
        }

        pod->update_vis_obj();
        emit updatedObject(tmo.id(), UPDATE_ALL);
        emit updatedObject(pod->tex_vis_obj_id, UPDATE_ALL);
    });
}

void ParameterizationPlugin::ensure_vis_object(TriMeshObject &orig)
{
    auto *pod = dynamic_cast<ParamPOD*>(orig.objectData(PodNames::param));
    auto &vis_id = pod->tex_vis_obj_id;
    auto *vis_obj = PluginFunctions::triMeshObject(vis_id);
    if (vis_obj) {
        if (vis_obj->mesh()->n_vertices() == orig.mesh()->n_vertices()) {
            return;
        } else {
            emit deleteObject(vis_id);
            vis_id = -1;
        }
    }
    emit addEmptyObject(DATA_TRIANGLE_MESH, vis_id);
    const auto &orig_mesh = *orig.mesh();
    auto vis_tmo = PluginFunctions::triMeshObject(vis_id);
    vis_tmo->setName("Texture map vis for " + orig.name());
    pod->vis_mesh_ = vis_tmo->mesh();
    auto &new_mesh = *pod->vis_mesh_;
    new_mesh.request_vertex_texcoords2D();
    new_mesh.update_normals();
    new_mesh.request_vertex_colors();

    // Compute bounding box:
    Vec3d min_pos{std::numeric_limits<double>::infinity()};
    Vec3d max_pos{-std::numeric_limits<double>::infinity()};
    for (const auto vh: orig_mesh.vertices()) {
        min_pos.minimize(orig_mesh.point(vh));
        max_pos.maximize(orig_mesh.point(vh));
    }

    for (const auto vh: orig_mesh.vertices()) {
        new_mesh.add_vertex(orig_mesh.point(vh));
        // position relative to bounding box:
        //auto relpos = (orig_mesh.point(vh)-min_pos)/(max_pos-min_pos);
        //new_mesh.set_color(vh, Vec4f(relpos[0], relpos[1], relpos[2], 1));

        // color by normal:
        auto n = (orig_mesh.normal(vh) + Vec3d(1.0)) * .5;
        new_mesh.set_color(vh, Vec4f(n[0], n[1], n[2], 1));
    }
    pod->vis_origin = .5 * (min_pos + max_pos) + Vec3d(max_pos[0]-min_pos[0],0.,0.);
    for (const auto fh: orig_mesh.faces()) {
        new_mesh.add_face(fh.vertices().to_vector());
    }
    vis_tmo->setObjectDrawMode(
            ACG::SceneGraph::DrawModes::SOLID_POINTS_COLORED
            | ACG::SceneGraph::DrawModes::WIREFRAME, true);
    orig.setObjectDrawMode(
            ACG::SceneGraph::DrawModes::SOLID_TEXTURED_SHADED
            | ACG::SceneGraph::DrawModes::WIREFRAME, true);
    emit switchTexture("HarmonicParam", orig.id());
}



