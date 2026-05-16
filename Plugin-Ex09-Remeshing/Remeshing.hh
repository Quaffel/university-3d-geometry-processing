#pragma once

#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>
#include <OpenFlipper/common/perObjectData.hh>

using ScalarVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<double>>;
using ScalarEProp = OpenMesh::PropertyManager<OpenMesh::EPropHandleT<double>>;
using ScalarHProp = OpenMesh::PropertyManager<OpenMesh::HPropHandleT<double>>;
using VectorVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<OpenMesh::Vec3d>>;

using EH = OpenMesh::EdgeHandle;
using VH = OpenMesh::VertexHandle;
using Vec3d = ACG::Vec3d;



enum class RemeshingType {
    Uniform,
    Adaptive
};

struct RemeshingConfig {
    RemeshingType type = RemeshingType::Uniform;
    double target_length_ratio = 1.0; // target avg. edge length, relative to input avg. edge length
};
class Remeshing : public PerObjectData {
public:
    using PairLE = std::pair<double, OpenMesh::EdgeHandle>;
    using PairLHE = std::pair<double, OpenMesh::HalfedgeHandle>;

public:
    Remeshing(TriMesh& _mesh, RemeshingConfig const & _config);
    void init(RemeshingConfig const & _config);

    void calc_target_lengths();


    /// return true if the mesh was modified.
    bool remesh();

    void tangential_relaxation(int n_iters = 10);


    /// return true if any splits were performed
    bool split_long_edges();
    /// return true if any collapses were performed
    bool collapse_short_edges();
    /// return true if any flips were performed
    bool equalize_valences();

private:
    void update_edge_len_cache();

private:
    TriMesh& mesh_;
    RemeshingConfig config_;
    ScalarVProp target_length_{mesh_, "target length"};
    ScalarEProp edge_length_{mesh_, "cached edge length"};
};
