#include "PoissonReconstructionT.hh"
#include "PoissonReconstructionT_impl.hh"

// explicit template instantiation: the translation unit for this file will contain a compiled version
// of the templated function with MeshT = TriMesh.
template bool reconstruct(std::vector<Real> const &pts, TriMesh& _out_mesh, const PoissonReconParameter&);
