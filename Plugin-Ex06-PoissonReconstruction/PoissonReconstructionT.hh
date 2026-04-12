#pragma once

#include <vector>
#include <ObjectTypes/TriangleMesh/TriangleMesh.hh>
#include <ObjectTypes/PolyMesh/PolyMesh.hh>

using Real = double;

struct PoissonReconParameter {
    int max_octree_depth = 8;

    /// Smooth Signed Distance function (solving a 3D bi-Laplacian system with positional value and gradient constraints) [Calakli and Taubin, 2011] http://mesh.brown.edu/ssd/
    bool use_ssd = false; 

    bool verbose = true;
    double point_weight = 4.0; // 4 as default for screened PR, 0 gives us original PR
};

/// _out_mesh must be an OpenMesh mesh
template <class MeshT>
bool reconstruct(std::vector<Real> const &pts, MeshT& _out_mesh , const PoissonReconParameter&);


//extern template bool poisson_reconstruct(std::vector<Real> const &pts, PolyMesh& _out_mesh , const PoissonReconParameter&);
extern template bool reconstruct(std::vector<Real> const &pts, TriMesh& _out_mesh , const PoissonReconParameter&);
