#include "Remeshing.hh"
#include <queue>

#define ASSIGNMENT_09_SOLUTION 1

Remeshing::
    Remeshing(TriMesh& _mesh, RemeshingConfig const& _config)
: mesh_(_mesh)
, config_(_config)
{
    init(_config);
}
void Remeshing::init(RemeshingConfig const & _config)
{
    config_ = _config;
    update_edge_len_cache();
    calc_target_lengths();
}

void Remeshing::update_edge_len_cache() {
    for(auto eh : mesh_.edges()) {
        edge_length_[eh] = mesh_.calc_edge_length(eh);
    }
}

bool Remeshing::remesh() {
    bool changed = false;
    changed |= split_long_edges();
    changed |= collapse_short_edges();
    changed |= equalize_valences();
    if (!changed) {
        return false;
    } else {
        tangential_relaxation();
        return true;
    }
}

void Remeshing::tangential_relaxation(int n_iters) {
    int valence(0);
    Vec3d u, n;
    Vec3d laplace;

    auto update = OpenMesh::makeTemporaryProperty<VH, Vec3d>(mesh_);

    mesh_.update_normals();
    mesh_.update_vertex_normals();

    // smooth
    for (int iters = 0; iters < n_iters; ++iters) {
        for (auto vh : mesh_.vertices()) {
            if (mesh_.is_boundary(vh)) continue;
                // ------------- TODO: IMPLEMENT HERE ---------
                //  Compute uniform laplacian curvature approximation vector
                //  Compute the tangential component of the laplacian vector and move the vertex
                //  Store smoothed vertex location in the update vertex property.
        }

        for (auto vh : mesh_.vertices()) {
            if (!mesh_.is_boundary(vh)) {
                mesh_.point(vh) += update[vh];
            }
        }
    }
    update_edge_len_cache();
}

void Remeshing::calc_target_lengths() {
    double length, mean_length, H, K;

    const double target_edge_len = config_.target_length_ratio * mesh_.edges().avg(edge_length_);

    if(config_.type == RemeshingType::Uniform) {
        // Equal target for now, scaling at the end
        target_length_.set_range(mesh_.vertices(), target_edge_len);
    }
    else if(config_.type == RemeshingType::Adaptive) {
        if (!(OpenMesh::hasProperty<VH, double>(mesh_, "max curvature")
           && OpenMesh::hasProperty<VH, double>(mesh_, "min curvature"))) {
            target_length_.set_range(mesh_.vertices(), 1.);
            std::cerr << "ERROR: max and/or min curvature vertex property not available. Use curvature plugin to compute it! Defaulting to uniform remeshing..." << std::endl;
        } else {
            // Remember, min and max curv are signed quantities:
            auto max_curv = OpenMesh::getProperty<VH, double>(mesh_, "max curvature");
            auto min_curv = OpenMesh::getProperty<VH, double>(mesh_, "min curvature");

            auto smoothed_target_len = OpenMesh::makeTemporaryProperty<VH, double>(mesh_);
            // === TODO: your code goes here ===
            // Get the maximal curvature at each vertex.
            // Use the Curvature plugin you implemented in an earlier week to compute
            // curvature, it will leave a vertex property that we can re-use.
            // Calculate the desired edge length as the 1 divided by the
            // absolute value of maximal curvature (plus a small epsilon)
            // at each vertex, and assign it to the property target_length
            // Smooth the target length uniformly (~5 iterations should be good).
            // You can use the smoothed_target_len for intermediate storage of smoothed values
            // (but you must copy the it back to target_length_)

            // Now we apply global scaling to the target length, such that the average target length
            // has the correct ratio to the original average edge length:
            // === TODO: your code goes here ===
            // - apply global scaling to target_length_ to match the user-specified length ratio
        }
    } else {
        throw std::runtime_error("unknown remeshing type");
    }

    std::cout << "orig avg elen " << mesh_.edges().avg(edge_length_) << std::endl;;
    std::cout << "avg target len: " << mesh_.vertices().avg(target_length_) << std::endl;;
}

bool Remeshing::split_long_edges()
{
    // === TODO: your code goes here ===
    //  Compute the desired length as the mean between the property target_length of two vertices of the edge
    //  If the edge is longer than 4/3 * desired length
    //    add the midpoint to the mesh as a new vertex
    //    set the interpolated normal and interpolated target length property to the vertex
    //    split the edge with this vertex (use OpenMesh function split_edge(EH, VH))
    //
    //  return true if any splits were performed, false if the mesh remained unchanged.

    return false;
}

bool Remeshing::collapse_short_edges() {
    int n_collapses = 0;
    // with std::greater, short edges will be at the top of the queue:
    std::priority_queue<PairLE, std::vector<PairLE>, std::greater<PairLE>> queue;
    for(auto eh : mesh_.edges())
        queue.push(std::make_pair(edge_length_[eh], eh));
    // === TODO: your code goes here ===
    // Compute the desired length as the mean between the property target_length_ of two vertices of the edge
    // If the edge is shorter than 4/5 of the desired length
    //    Check if halfedge connects a boundary vertex with a non-boundary vertex. If so, don't collapse.
    //    Check if halfedges collapsible (mesh_.is_collapse_ok())
    //    Select the halfedge to be collapsed if at least one halfedge can be collapsed
    //    Collapse the halfedge
    //    If the remaining vertex is not a boundary vertex that would be pulled inwards,
    //    move the remaining vertex to the middle of the former edge
    //    Add any other edges whose length was changed to the queue
    // Leave the loop running until the queue is empty
    // This will always terminate, as a finite number of edges exist.
    std::cerr<<"collapsed " << n_collapses << " edges" << std::endl;

    mesh_.garbage_collection(); // this is required to remove gaps from deleted entities
    return n_collapses > 0;
}

bool Remeshing::equalize_valences()
{
    int n_flips=0;
    std::queue<OpenMesh::EdgeHandle> queue;
    for(auto eh : mesh_.edges())
        queue.push(eh);
    // === TODO: your code goes here ===
    //  Extract valences of the four vertices involved to an eventual flip.
    //  Compute the sum of the squared valence deviances before flip
    //  Compute the sum of the squared valence deviances after an eventual flip
    //  If valence deviance is decreased and flip is possible, flip the vertex
    // Leave the loop running until the queue is empty
    std::cerr<<"\nflipped "<<n_flips<<" edges" << std::endl;
    return n_flips > 0;
}
