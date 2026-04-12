#pragma once

// This file is based on Reconstruction.example.cpp of the reference PoissonRecon
// implementation, version 18.75

#include "PoissonReconstructionT.hh"

#include <OpenMesh/Core/Utils/PropertyManager.hh>
#include <OpenMesh/Core/Mesh/ArrayKernel.hh>


// The PoissonRecon code causes many warnings, which we don't really care about.
// Ignore them, so we can see our own warnings and errors better.
#if defined __GNUC__ || defined __clang__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wsign-compare"
#  pragma GCC diagnostic ignored "-Wdangling-else"
#endif

#if defined __clang__
#  pragma GCC diagnostic ignored "-Wunknown-warning-option"
#  pragma GCC diagnostic ignored "-Wreorder-ctor"
#  pragma GCC diagnostic ignored "-Wdelete-non-abstract-non-virtual-dtor"
#  pragma GCC diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
#  pragma GCC diagnostic ignored "-Wnontrivial-memcall"
#elif defined __GNUC__
#  pragma GCC diagnostic ignored "-Wmisleading-indentation"
#  pragma GCC diagnostic ignored "-Wuninitialized"
#  pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#  pragma GCC diagnostic ignored "-Wstringop-overflow"
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  pragma GCC diagnostic ignored "-Wreorder"
#  pragma GCC diagnostic ignored "-Wstrict-aliasing"
#  pragma GCC diagnostic ignored "-Wtype-limits"
#  pragma GCC diagnostic ignored "-Wclass-memaccess"
//#  pragma GCC diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif

#define SANITIZED_PR 1 // This must be set prior to including any PoissonRecon code to enable proper multithreading on non-x86 architectures.
                       // cf https://github.com/mkazhdan/PoissonRecon/issues/139
#include "PoissonRecon/Reconstructors.h"

#if defined __GNUC__ || defined __clang__
// re-enable warnings that were disabled:
#  pragma GCC diagnostic pop
#endif

#define ASSIGNMENT_06_SOLUTION 1


template<typename MeshT>
struct OpenMeshPolygonStream : public PoissonRecon::Reconstructor::OutputFaceStream< 2>
{
    OpenMeshPolygonStream(MeshT &_out_mesh) : mesh_(_out_mesh) {}

    size_t size() const override {
        // TODO: implement this member function
        //
        // Read the PoissonRecon readme section on "HEADER-ONLY LIBRARY" which
        // describes the stream interface we'll interface with:
        // https://github.com/mkazhdan/PoissonRecon/blob/cd6dc7d33f028b2e6496f5cd999c25cecd56aff2/README.md
        return 0;
    }
    size_t write(const std::vector< PoissonRecon::node_index_type> &polygon) override
    {
        // TODO: implement this member function
        // cf. https://github.com/mkazhdan/PoissonRecon/blob/cd6dc7d33f028b2e6496f5cd999c25cecd56aff2/Src/Reconstruction.example.cpp#L226
        return 0;
    }

protected:
    MeshT &mesh_;
};

// A stream into which we can write the output vertices of the extracted mesh
template<typename MeshT, typename Real = typename MeshT::Point::value_type, unsigned int Dim = MeshT::Point::dim()>
struct OpenMeshVertexStream : public PoissonRecon::Reconstructor::OutputLevelSetVertexStream<Real, Dim>
{
    using ScalarVProp = OpenMesh::PropertyManager<OpenMesh::VPropHandleT<double>>;
    OpenMeshVertexStream(MeshT &_out_mesh)
        : mesh_(_out_mesh)
       , weight_{OpenMesh::getOrMakeProperty<OpenMesh::VertexHandle, double>(mesh_, "weight")}
    {
        weight_.set_persistent(true);
        mesh_.request_vertex_normals();
    }

    size_t size() const override { return mesh_.n_vertices(); }
    size_t write(const PoissonRecon::Point< Real, Dim> &pos,
            const PoissonRecon::Point< Real, Dim> & grad,
            const Real & weight) override
    {
        // TODO: implement this member function.
        // Add the vertex to the mesh,
        // compute and set (mesh_.set_normal(vh, n)) a vertex normal using the supplied gradient,
        // and save the weight value to the weight_ property.
        // cf. https://github.com/mkazhdan/PoissonRecon/blob/cd6dc7d33f028b2e6496f5cd999c25cecd56aff2/Src/Reconstruction.example.cpp#L246
        return 0;
    }
protected:
    MeshT &mesh_;
    ScalarVProp weight_;
};

template<typename Real = double, unsigned int Dim = 3>
struct PackedSampleStream : public PoissonRecon::Reconstructor::InputOrientedSampleStream< Real, Dim>
{
    using PRPoint = PoissonRecon::Point<Real, Dim>;
    PackedSampleStream(std::vector<Real> const& _data)
        : data_(_data)
    {}

    void reset() override{ idx_ = 0;}

    bool read(PRPoint &p, PRPoint &n) override
    {
        // TODO: implement this member function to pass one entry of the packed data to PoissonRecon
        // cf. https://github.com/mkazhdan/PoissonRecon/blob/cd6dc7d33f028b2e6496f5cd999c25cecd56aff2/Src/Reconstruction.example.cpp#L138
        return false;
    }

protected:
    size_t idx_ = 0;
    std::vector<Real> const& data_;
};

template<typename ReconType=PoissonRecon::Reconstructor::Poisson,
    unsigned int FEMDegree = ReconType::DefaultFEMDegree,
    PoissonRecon::BoundaryType BType=ReconType::DefaultFEMBoundary,
    typename Real = double,
    unsigned int Dim = 3>
bool do_reconstruct(
        PoissonRecon::Reconstructor::InputOrientedSampleStream<Real, Dim> &in_stream,
        PoissonRecon::Reconstructor::OutputLevelSetVertexStream<Real, Dim> &out_vstream,
        PoissonRecon::Reconstructor::OutputFaceStream<2> &out_fstream,
        const PoissonReconParameter& _parameter)
{
    using namespace PoissonRecon;
    Timer timer;
    // If you experience crashes, it may be due to concurrency bugs in PoissonRecon,
    // you can try to disable parallelization:
    // (However the SANITIZED_PR at the topic "should" be sufficient)
    /// (workaround for <https://github.com/mkazhdan/PoissonRecon/issues/139>)
    //ThreadPool::ParallelizationType= ThreadPool::ParallelType::NONE;
    ThreadPool::ParallelizationType= (ThreadPool::ParallelType)0; // 0: pick first available of OPEN_MP and ASYNC
    
    // The tensor-product finite-elements signatures
    static const unsigned int FEMSig = FEMDegreeAndBType<FEMDegree, BType>::Signature;
    using FEMSigs = typename ParameterPack::IsotropicPack<unsigned int, Dim, FEMSig>;
    using Implicit = typename Reconstructor::template Implicit<Real, Dim, FEMSigs>;
    using Solver   = typename     ReconType::template Solver  <Real, Dim, FEMSigs>;

    // Parameters for performing the reconstruction
    typename ReconType::template SolutionParameters<Real> solverParams;

    if constexpr(std::is_same_v<ReconType,PoissonRecon::Reconstructor::Poisson>) {
        solverParams.pointWeight = _parameter.point_weight;
        // default params from default constructor, reproduced here for easy experimentation:
        solverParams.dirichletErode = false;
        solverParams.envelopeDepth = -1;
    } else if constexpr(std::is_same_v<ReconType,PoissonRecon::Reconstructor::SSD>) {
        // default params from default constructor, reproduced here for easy experimentation:
        solverParams.pointWeight = 5e+1f;
        solverParams.gradientWeight = 5e-4f;
        solverParams.biLapWeight = 1e-5f;
    }
    solverParams.showResidual = true;
    solverParams.confidence = false; // use normal norm as confidence?
    solverParams.verbose = _parameter.verbose;
    solverParams.depth = _parameter.max_octree_depth;

    // default params from default constructor, reproduced here for easy experimentation:
    solverParams.lowDepthCutOff = (Real)0.;
    solverParams.width = (Real)0.;

    solverParams.samplesPerNode = (Real)1.5;
    solverParams.cgSolverAccuracy = (Real)1e-3;
    solverParams.perLevelDataScaleFactor = (Real)32.;

    solverParams.solveDepth = (unsigned int)-1;
    solverParams.baseDepth = (unsigned int)-1;
    solverParams.fullDepth = (unsigned int)5;
    solverParams.kernelDepth = (unsigned int)-1;

    solverParams.baseVCycles = (unsigned int)1;
    solverParams.iters = (unsigned int)8;

    // Parameters for extracting the level-set surface
    Reconstructor::LevelSetExtractionParameters extractionParams;
    extractionParams.linearFit = std::is_same_v<ReconType,PoissonRecon::Reconstructor::SSD>;
    extractionParams.outputGradients = true;  // We'll use these as normals
    extractionParams.forceManifold = true;    // OpenMesh needs manifold inputs
    extractionParams.polygonMesh = true;      // OpenMesh can triangulate for us, if we do want to use an output tri mesh
    extractionParams.gridCoordinates = false; // use the default
    extractionParams.outputDensity = true;    // this is the weight
    extractionParams.verbose = _parameter.verbose;

    // Construct the implicit representation
    auto implicit = std::unique_ptr<Implicit>(Solver::Solve(in_stream, solverParams));
    implicit->extractLevelSet(out_vstream, out_fstream, extractionParams);

    printf("Time (Wall/CPU): %.2f / %.2f\n", timer.wallTime(), timer.cpuTime());
    printf("Peak Memory (MB): %d\n", MemoryInfo::PeakMemoryUsageMB());
    return true;
}

template <class MeshT>
bool reconstruct(std::vector<Real> const &_in_pts, MeshT& _out_mesh, const PoissonReconParameter& _parameter)
{
    auto sample_stream = PackedSampleStream(_in_pts);
    OpenMeshPolygonStream<MeshT> out_fstream(_out_mesh);
    OpenMeshVertexStream <MeshT> out_vstream(_out_mesh);
    if (_parameter.use_ssd) {
        return do_reconstruct<PoissonRecon::Reconstructor::SSD>(sample_stream, out_vstream, out_fstream, _parameter);
    } else {
        return do_reconstruct<PoissonRecon::Reconstructor::Poisson>(sample_stream, out_vstream, out_fstream, _parameter);
    }
}
