#include "NormalsAndCurvaturePlugin.hh"
#include <ObjectTypes/TriangleMesh/PluginFunctionsTriangleMesh.hh>
#include <ObjectTypes/PolyhedralMesh/PolyhedralMesh.hh>
#include <ObjectTypes/PolyhedralMesh/PluginFunctionsPolyhedralMesh.hh>
#include <OpenMesh/Core/Utils/PropertyManager.hh>
#include <OpenVolumeMesh/Core/Iterators.hh>
#include <ACG/Scenegraph/DrawModes.hh>
#include "ui_NormalsAndCurvatureToolbarBase.h"

#include <cmath>
#include <vector>

using VH = OpenMesh::VertexHandle;
using EH = OpenMesh::EdgeHandle;
using FH = OpenMesh::FaceHandle;
using HEH = OpenMesh::HalfedgeHandle;
using ACG::Vec3d;

#define ASSIGNMENT_05_SOLUTION 1

/// Avoid run-time name mismatches from typos in property names
namespace PropNames {
    static const char* const gaussian_curvature = "Gaussian curvature";
    static const char* const angle_defect = "Angle defect";
    static const char* const mean_curvature = "mean curvature";
    static const char* const mean_curvature_normal = "mean curvature normal";
    static const char* const min_curvature = "min curvature";
    static const char* const max_curvature = "max curvature";
    static const char* const vertex_area = "vertex area";
    static const char* const cotan_weights = "cotan weight";
    static const char* const uniform_laplacian_weights = "uniform laplacian weights";
}

class NormalsAndCurvatureToolbar : public QWidget, public Ui::NormalsAndCurvatureToolbarBase
{
private:
    // this is a hack to allow us to easily disable buttons of a group (normals or curvature),
    // Qt won't just allow us to do setChecked(false) on a auto-exclusive group.
    QPushButton *dummy_normal;
    QPushButton *dummy_curvature;
public:
    NormalsAndCurvatureToolbar(QWidget * parent = nullptr)
        : QWidget(parent)
    {
        setupUi(this);

        dummy_normal = new QPushButton(groupBox1_vis_normals);
        dummy_normal->setCheckable(true);
        dummy_normal->setAutoExclusive(true);
        dummy_normal->setVisible(false);

        dummy_curvature = new QPushButton(groupBox2_curvature);
        dummy_curvature->setCheckable(true);
        dummy_curvature->setAutoExclusive(true);
        dummy_curvature->setVisible(false);
    }

    void uncheckNormalButtons() {
        dummy_normal->setChecked(true);
    }

    void uncheckCurvatureButtons() {
        dummy_curvature->setChecked(true);
    }

    AreaFormulation areaFormulation() const {
        if (rb_area_voronoi->isChecked()) return AreaFormulation::MixedVoronoi;
        assert(rb_area_barycentric->isChecked());
        return AreaFormulation::Barycentric;
    }
    std::optional<CurvatureKind> curvatureKind() const {
        if (btn_gaussian->isChecked()) return CurvatureKind::Gaussian;
        if (btn_angle_defect->isChecked()) return CurvatureKind::IntegratedGaussian;
        if (btn_curv_cotan_mean->isChecked()) return CurvatureKind::Mean;
        if (btn_curv_cotan_min->isChecked()) return CurvatureKind::Min;
        if (btn_curv_cotan_max->isChecked()) return CurvatureKind::Max;
        if (btn_curv_uniform_mean->isChecked()) return CurvatureKind::Mean;
        if (btn_curv_uniform_min->isChecked()) return CurvatureKind::Min;
        if (btn_curv_uniform_max->isChecked()) return CurvatureKind::Max;
        return std::nullopt;
    }
    std::optional<LaplacianWeights> laplacianWeights() const {
        if (btn_curv_cotan_mean->isChecked()) return LaplacianWeights::Cotan;
        if (btn_curv_cotan_min->isChecked()) return  LaplacianWeights::Cotan;
        if (btn_curv_cotan_max->isChecked()) return LaplacianWeights::Cotan;
        if (btn_curv_uniform_mean->isChecked()) return LaplacianWeights::Uniform;
        if (btn_curv_uniform_min->isChecked()) return LaplacianWeights::Uniform;
        if (btn_curv_uniform_max->isChecked()) return LaplacianWeights::Uniform;
        return std::nullopt;
    }
};


/// Compute angle (in radians) of the triangle corner that heh points to (i.e. heh.to() in heh.face())
static double compute_angle(TriMesh const& _mesh, HEH _heh)
{
    auto heh = _mesh.make_smart(_heh);
    // TODO: compute the angle of corner heh.to() in the face heh.face()
    // Hint: use cross- and dot product to determine sin(alpha) and cos(alpha),
    // then use std::atan2 (Q: why not acos() or asin()?)
    // Although OpenMesh has built-in functionality to compute angles, do NOT use it.
    return 1234;
}

static double compute_area(TriMesh const& _mesh, FH _fh)
{
    auto fh = _mesh.make_smart(_fh);
    // TODO: compute the face area.
    // Although OpenMesh has built-in functionality to compute aras, do NOT use it.
    return 1234;
}

/// Compute cotan of triangle corner that heh points to (i.e. heh.to() in heh.face())
static double compute_cotan(TriMesh const& _mesh, HEH _heh)
{
    auto heh = _mesh.make_smart(_heh);
    const Vec3d &p0 = _mesh.point(heh.from());
    const Vec3d &p1 = _mesh.point(heh.to()); // this is the corner whose cotan we want
    const Vec3d &p2 = _mesh.point(heh.next().to());
    // TODO: compute the cotan of the angle in the face heh.face() at the corner heh.to()
    // Hint: you do NOT need to compute the angle itself, or
    //       call any trigonometric functions like sin(), cos(), or tan().
}

static double compute_cotan_weight(TriMesh const& _mesh, EH _eh)
{
    auto eh = _mesh.make_smart(_eh);
    double w = 0;
    for (int subidx: {0, 1}) {
        auto heh = eh.h(subidx);
        if (heh.is_boundary()) continue;
        // heh.next() points at the vertex opposite of heh in heh.face
        w += compute_cotan(_mesh, heh.next());
    }
    return 0.5 * w;
}

static Vec3d compute_normal(TriMesh const& _mesh, FH _fh)
{
    auto fh = _mesh.make_smart(_fh);
    // TODO: Compute a unit normal vector for face _fh. Its orientation should be
    //       such that it points outwards on meshes where face vertices are
    //       ordered counter-clockwise (all supplied closed meshes follow this convention).
    //       If your mesh gets rendered strangely dark, your normal might be flipped or non-normalized.
    return Vec3d(0.);
}

void NormalsAndCurvaturePlugin::initializePlugin()
{
    tool_ = new NormalsAndCurvatureToolbar();
    connect(tool_->btn_import_ovm,        &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotImportOVMSurfaces);

    connect(tool_->btn_normals_uniform,   &QPushButton::clicked, [this](){slotUpdateNormals(VertexNormalKind::UniformInterpol);});
    connect(tool_->btn_normals_angle,     &QPushButton::clicked, [this](){slotUpdateNormals(VertexNormalKind::AngleBasedInterpol);});
    connect(tool_->btn_normals_area,      &QPushButton::clicked, [this](){slotUpdateNormals(VertexNormalKind::AreaBasedInterpol);});
    connect(tool_->btn_normals_mean_curv, &QPushButton::clicked, [this](){slotUpdateNormals(VertexNormalKind::MeanCurvature);});

    connect(tool_->btn_gaussian,          &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->btn_angle_defect,      &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->btn_curv_cotan_mean,   &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->btn_curv_cotan_min,    &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->btn_curv_cotan_max,    &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->btn_curv_uniform_mean, &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->btn_curv_uniform_min,  &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->btn_curv_uniform_max,  &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->rb_area_voronoi,       &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);
    connect(tool_->rb_area_barycentric,   &QPushButton::clicked, this, &NormalsAndCurvaturePlugin::slotShowCurvature);

    emit addToolbox( tr("GP Ex05: Normals and Curvature") , tool_ );
}


ScalarHProp NormalsAndCurvaturePlugin::computeNormalInterpolationWeights(
    TriMesh& _mesh, VertexNormalKind _kind)
{
    auto w = OpenMesh::makeTemporaryProperty<HEH, double>(_mesh);
    switch(_kind) {
        case VertexNormalKind::UniformInterpol:
            emit log (LOGINFO, "Computing Uniform weights for normal interpolation");
            for (const auto heh: _mesh.halfedges()) {
                w[heh] = 1.;
            }
            break;
        case VertexNormalKind::AngleBasedInterpol:
            emit log (LOGINFO, "Computing Angle-based weights for normal interpolation");
            for (const auto heh: _mesh.halfedges()) {
                w[heh] = compute_angle(_mesh, heh);
            }
            break;
        case VertexNormalKind::AreaBasedInterpol:
            emit log (LOGINFO, "Computing Area-based weights for normal interpolation");
            for (const auto fh: _mesh.faces()) {
                auto area = compute_area(_mesh, fh);
                for (const auto heh: fh.halfedges()) {
                    w[heh] = area;
                }
            }
            break;
        case VertexNormalKind::MeanCurvature:
            assert(false);
    }
    return w;
}

void NormalsAndCurvaturePlugin::computeVertexNormals(TriMesh &_mesh, VertexNormalKind _normal_kind)
{
    // These should already exist, but this would be required if you use this code outside of OpenFlipper:
    _mesh.request_face_normals();
    _mesh.request_vertex_normals();

    if (_normal_kind != VertexNormalKind::MeanCurvature) {
        for (const auto fh: _mesh.faces()) {
            // This normal can be accessed using _mesh.normal(fh) later.
            _mesh.set_normal(fh, compute_normal(_mesh, fh));
        }

        // the weight used for vertex v of face f is stored at the halfedge he with he.face = face and he.to=v
        auto interpol_weights = computeNormalInterpolationWeights(_mesh, _normal_kind);

        for (const auto vh: _mesh.vertices()) {
            Vec3d sum = Vec3d{0.};
            // "incoming" halfedges of vh have heh.to() == vh
            for (const auto heh: vh.incoming_halfedges()) {
                auto fh = heh.face();
                if (!fh.is_valid()) continue; // boundary
                sum += interpol_weights[heh] * _mesh.normal(fh);
            }
            _mesh.set_normal(vh, sum.normalized());
        }
    } else {
        const auto mean_curv_normal = computeMeanCurvatureNormal(_mesh, LaplacianWeights::Cotan);
        for (const auto vh: _mesh.vertices()) {
            auto mcn = mean_curv_normal[vh];
            const auto gn = _mesh.normal(vh); // try to use existing normal to figure out orientation
            if (dot(mcn, gn) < 0.0) {
                mcn = -mcn;
            }
            const auto k_norm = mcn.norm();
            if (std::isfinite(k_norm)) {
                _mesh.set_normal(vh, mcn / k_norm);
            } else {
                _mesh.set_normal(vh, Vec3d(0.0));
            }
        }
    }
}


void NormalsAndCurvaturePlugin::slotUpdateNormals(VertexNormalKind _normal_kind)
{
    tool_->uncheckCurvatureButtons();

    for (BaseObjectData * obj: PluginFunctions::objects(
                PluginFunctions::TARGET_OBJECTS,
                DATA_TRIANGLE_MESH))
    {
        TriMeshObject *tmo = PluginFunctions::triMeshObject(obj);

        computeVertexNormals(*tmo->mesh(), _normal_kind);
        tmo->setObjectDrawMode(ACG::SceneGraph::DrawModes::SOLID_PHONG_SHADED, true);
        emit updatedObject(tmo->id(), UPDATE_ALL);
    }
}

std::pair< ScalarVProp, ScalarVProp>
NormalsAndCurvaturePlugin::
computeGaussianCurvature(TriMesh &_mesh)
{
    auto gaussian_curv = OpenMesh::getOrMakeProperty<VH, double>(_mesh, PropNames::gaussian_curvature);
    auto integrated_gauss_curv = OpenMesh::getOrMakeProperty<VH, double>(_mesh, PropNames::angle_defect);

    gaussian_curv.set_range(_mesh.vertices(), 0.);
    integrated_gauss_curv.set_range(_mesh.vertices(), 0.);
    const auto vertex_area = computeVertexAreas(_mesh);
    double total_angle_defect = 0.;
    for (auto vh: _mesh.vertices()) {
        double angle_sum = 0.;
    // TODO:
    //  - compute integrated_gauss_curv[vh]
    //  - compute gaussian_curv[vh]
    //  - add to total_angle_defect
    // Notes:
    //  - You can use compute_angle() defined above
    //  - You can set the curvature to zero at boundary vertices.
    }
    emit log(LOGINFO, "Total angle defect: 2π * " + QString::number(total_angle_defect/(2*M_PI)));

    return {gaussian_curv, integrated_gauss_curv};
}
ScalarVProp
NormalsAndCurvaturePlugin::
computeMeanCurvature(TriMesh &_mesh, LaplacianWeights _lap_kind)
{
    auto mean_curv = OpenMesh::getOrMakeProperty<VH, double>(_mesh, PropNames::mean_curvature);
    mean_curv.set_range(_mesh.vertices(), 0.);
    const auto mean_curv_normal = computeMeanCurvatureNormal(_mesh, _lap_kind);

    for (auto vh: _mesh.vertices()) {
        const auto mcn = mean_curv_normal[vh];
        const auto k_norm = mcn.norm();
        if (!vh.is_boundary() && std::isfinite(k_norm))
        {
            // TODO: compute mean_curv[vh] from mean_curv_normal[vh].
            //       Make sure to determine the correct sign!
            //       For this purpose, you can access vertex normals using _mesh.normal(vh)
            mean_curv[vh] = 1234.;
        } else {
            mean_curv[vh] = 0.;
        }
    }

    return mean_curv;
}

VectorVProp
NormalsAndCurvaturePlugin::
computeMeanCurvatureNormal(TriMesh& _mesh, LaplacianWeights _lap_kind)
{
    auto mean_curv_normal = OpenMesh::getOrMakeProperty<VH, Vec3d>(_mesh, PropNames::mean_curvature_normal);

    ScalarEProp edge_weights = _lap_kind == LaplacianWeights::Cotan ?
        computeCotanWeights(_mesh)
        : computeUniformLaplacianWeights(_mesh);

    const auto vertex_area = computeVertexAreas(_mesh);

    mean_curv_normal.set_range(_mesh.vertices(), Vec3d(0.0));
    for (auto vh: _mesh.vertices()) {
        auto mcn = Vec3d(0.0);
        // This is equivalent to computing one entry of L * V,
        // where L is the n*n laplacian matrix, and V is a n*3 matrix of vertex positions.
        for (const auto vih: vh.incoming_halfedges()) {
            const double w = edge_weights[vih.edge()];
            const auto evec = _mesh.point(vih.from()) - _mesh.point(vh);
            mcn += w * evec;
        }

        if (_lap_kind == LaplacianWeights::Cotan) {
            mcn /= vertex_area[vh];
        } else if (_lap_kind == LaplacianWeights::Uniform) {
            mcn /= vh.valence();
        }
        mean_curv_normal[vh] = mcn;
    }

    return mean_curv_normal;
}

std::pair< ScalarVProp, ScalarVProp>
NormalsAndCurvaturePlugin::
computeMinMaxCurvature(TriMesh &_mesh, LaplacianWeights _lap_kind)
{
    auto prop_gaussian = computeGaussianCurvature(_mesh).first;
    auto prop_mean = computeMeanCurvature(_mesh, _lap_kind);

    auto min_prop = OpenMesh::getOrMakeProperty<VH, double>(_mesh, PropNames::min_curvature);
    auto max_prop = OpenMesh::getOrMakeProperty<VH, double>(_mesh, PropNames::max_curvature);

    min_prop.set_range(_mesh.vertices(), 0.);
    max_prop.set_range(_mesh.vertices(), 0.);

    for (auto vh: _mesh.vertices()) {
        if (_mesh.is_boundary(vh)) {
            continue;
        }
        const double H = prop_mean[vh];     // H = .5 * (k_1 + k_2)
        const double K = prop_gaussian[vh]; // K = k_1 * k_2
    // TODO: compute k_1 and k_2 from H and K,
    //       set min_prop[vh] and max_prop[vh] to k_1 and k_2.
    }
    return std::make_pair(min_prop, max_prop);
}

ScalarVProp
NormalsAndCurvaturePlugin::
computeVertexAreas(TriMesh& _mesh)
{
    auto _area_kind = tool_->areaFormulation();
    auto vertex_area = OpenMesh::getOrMakeProperty<VH, double>(_mesh, PropNames::vertex_area);
    vertex_area.set_range(_mesh.vertices(), 0.); // this initialisation is crucial, we only add to this

    for (const auto fh: _mesh.faces()) {
        auto heh0 = fh.halfedge();
        auto heh1 = heh0.next();
        auto heh2 = heh1.next();
        const auto v0 = heh0.to();
        const auto v1 = heh1.to();
        const auto v2 = heh2.to();
        const Vec3d& p0 = _mesh.point(v0);
        const Vec3d& p1 = _mesh.point(v1);
        const Vec3d& p2 = _mesh.point(v2);
        const Vec3d e0 = p1 - p0;
        const Vec3d e1 = p2 - p0;
        const double face_area = cross(e0, e1).norm() / 2;

        if (_area_kind == AreaFormulation::Barycentric) {
            vertex_area[v0] += face_area / 3.0;
            vertex_area[v1] += face_area / 3.0;
            vertex_area[v2] += face_area / 3.0;
        } else if (_area_kind == AreaFormulation::MixedVoronoi) {
            double l01 = (p1 - p0).sqrnorm();
            double l12 = (p2 - p1).sqrnorm();
            double l20 = (p0 - p2).sqrnorm();
            double cot0 = compute_cotan(_mesh, heh0); // cotan at heh0.to()
            double cot1 = compute_cotan(_mesh, heh1);
            double cot2 = compute_cotan(_mesh, heh2);
            if (cot0 < 0.0 || cot1 < 0.0 || cot2 < 0.0) {
                if (cot0 < 0.0) {
                    vertex_area[v0] += face_area / 2.0;
                    vertex_area[v1] += face_area / 4.0;
                    vertex_area[v2] += face_area / 4.0;
                } else if (cot1 < 0.0) {
                    vertex_area[v0] += face_area / 4.0;
                    vertex_area[v1] += face_area / 2.0;
                    vertex_area[v2] += face_area / 4.0;
                } else {
                    vertex_area[v0] += face_area / 4.0;
                    vertex_area[v1] += face_area / 4.0;
                    vertex_area[v2] += face_area / 2.0;
                }
            }else {
                vertex_area[v0] += (l01 * cot2 + l20 * cot1) / 8.0;
                vertex_area[v1] += (l12 * cot0 + l01 * cot2) / 8.0;
                vertex_area[v2] += (l20 * cot1 + l12 * cot0) / 8.0;
            }
        }
    }
    return vertex_area;
}

ScalarEProp
NormalsAndCurvaturePlugin::
computeUniformLaplacianWeights(TriMesh& _mesh)
{
    auto edge_weight = OpenMesh::getOrMakeProperty<EH, double>(_mesh, PropNames::uniform_laplacian_weights);
    edge_weight.set_range(_mesh.edges(), 1.);
    return edge_weight;
}

ScalarEProp
NormalsAndCurvaturePlugin::
computeCotanWeights(TriMesh& _mesh)
{
    auto cotan_weight = OpenMesh::getOrMakeProperty<EH, double>(_mesh, PropNames::cotan_weights);
    cotan_weight.set_range(_mesh.edges(), 0.);

    size_t n_edges_with_negative_weight = 0;
    for (auto eh: _mesh.edges())
    {
        double w = compute_cotan_weight(_mesh, eh);
        if (w < 0) {
            ++n_edges_with_negative_weight;
            // optionally highlight problematic edges:
            //_mesh.status(eh).set_selected(true);
        }
        cotan_weight[eh] = w;
    }
    emit log(LOGWARN, "We have " +
            QString::number(n_edges_with_negative_weight)
            + " edges with negative cotan weights (obtuse corners).");
    return cotan_weight;
}

void NormalsAndCurvaturePlugin::slotShowCurvature()
{
    auto maybe_curv = tool_->curvatureKind();
    auto maybe_lap = tool_->laplacianWeights();
    if (!maybe_curv.has_value()) {
        return;
    }
    tool_->uncheckNormalButtons();
    CurvatureKind _kind = *maybe_curv;

    for (BaseObjectData * obj: PluginFunctions::objects(
                PluginFunctions::TARGET_OBJECTS,
                DATA_TRIANGLE_MESH))
    {
        TriMeshObject *tmo = PluginFunctions::triMeshObject(obj);
        TriMesh &mesh = *tmo->mesh();
        ScalarVProp prop = [&](){
            switch(_kind) {
                case CurvatureKind::Mean:     return computeMeanCurvature(mesh, *maybe_lap);
                case CurvatureKind::Gaussian: return computeGaussianCurvature(mesh).first;
                case CurvatureKind::IntegratedGaussian: return computeGaussianCurvature(mesh).second;
                case CurvatureKind::Min:      return computeMinMaxCurvature(mesh, *maybe_lap).first;
                case CurvatureKind::Max:      return computeMinMaxCurvature(mesh, *maybe_lap).second;
            }
        }();
        // We often have some extreme outliers, so scaling between min/max is not great.
        // Instead, we scale based on the average:
        // (Feel free to experiment with better auto-scaling methods, e.g. based on quantiles, or maybe nonlinear?)

        //double prop_maxabs = mesh.vertices().max([&prop](VH vh) {return std::abs(prop[vh]);});
        double prop_avg = mesh.vertices().avg([&prop](VH vh) {return std::abs(prop[vh]);});
        auto scale = prop_avg == 0. ? 0. : 1. / (3*prop_avg);

        double prop_min = mesh.vertices().min(prop);
        double prop_max = mesh.vertices().max(prop);

        emit log(LOGINFO, obj->name() + ": Quantity value range: ["+
                QString::number(prop_min) + ", " +
                QString::number(prop_max) + "]");

        for (const auto vh: mesh.vertices()) {
            auto scaled_val = std::clamp(scale * prop[vh], -1., 1.);
#if 0
            auto color = scaled_val < 0 // neg: red, 0: white, pos: blue
                ? ACG::Vec4f{1.,            1.+scaled_val, 1.+scaled_val, 1.}
                : ACG::Vec4f{1.-scaled_val, 1.-scaled_val, 1.,            1.};
#else
            auto color = scaled_val > 0 // pos: red, 0:white, neg: blue
                ? ACG::Vec4f{1.,            1.-scaled_val, 1.-scaled_val, 1.}
                : ACG::Vec4f{1.+scaled_val, 1.+scaled_val, 1.,            1.};
#endif
            mesh.set_color(vh, color);
        }
        tmo->setObjectDrawMode(
            // no shading, displayed color does not depend on orientation or lighting and is simply interpolated vertex color:
                ACG::SceneGraph::DrawModes::SOLID_POINTS_COLORED
                | ACG::SceneGraph::DrawModes::WIREFRAME
                );
        //tmo->setObjectDrawMode(ACG::SceneGraph::DrawModes::SOLID_POINTS_COLORED_SHADED); // prettier, but lighting can be misleading
        emit updatedObject(tmo->id(), UPDATE_ALL);
    }
}
