#include "Remeshing.hh"
#include "OpenMesh/Core/Utils/Predicates.hh"
#include <algorithm>
#include <queue>
#include <utility>
#include <vector>

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

    std::cout << "orig avg elen " << mesh_.edges().avg(edge_length_) << std::endl;
    std::cout << "avg target len: " << mesh_.vertices().avg(target_length_) << std::endl;;
}

bool Remeshing::split_long_edges()
{
    assert(edge_length_.isValid());

    std::vector<std::pair<double, OpenMesh::SmartEdgeHandle>> edges_by_length;
    edges_by_length.reserve(mesh_.n_edges());
    for (OpenMesh::SmartEdgeHandle edge : mesh_.edges()) {
        if (!edge.is_valid()) {
            continue;
        }

        edges_by_length.emplace_back(edge_length_[edge], edge);
    }
    
    // TODO: Verify order
    std::sort(edges_by_length.begin(), edges_by_length.end(), [](
        PairLE& first_edge_priority_pair, PairLE& second_edge_priority_pair
    ) -> bool {
        return first_edge_priority_pair.first > second_edge_priority_pair.first;
    });
    
    bool performed_one_or_more_splits = false;
    for (std::pair<double, OpenMesh::SmartEdgeHandle>& edge_priority_pair : edges_by_length) {
        OpenMesh::SmartEdgeHandle edge = edge_priority_pair.second;

        TriMesh::Point first_endpoint = mesh_.point(edge.v0());
        TriMesh::Point second_endpoint = mesh_.point(edge.v1());
        TriMesh::Point distance_vector = second_endpoint - first_endpoint;

        double average_target_length = edge.vertices().avg([this](OpenMesh::SmartVertexHandle endpoint) {
            return target_length_[endpoint];
        });
        
        double edge_length = distance_vector.norm();
        if (edge_length <= 4. / 3. * average_target_length) {
            continue;
        }

        performed_one_or_more_splits = true;
        OpenMesh::SmartVertexHandle midpoint = mesh_.add_vertex(first_endpoint + distance_vector / 2);
        
        target_length_[midpoint] = average_target_length;
        TriMesh::Point average_norm = edge.vertices().avg([this](OpenMesh::SmartVertexHandle endpoint) {
            return mesh_.normal(endpoint);
        });
        
        mesh_.split_edge(edge, midpoint);
    }

    return performed_one_or_more_splits;
}

bool Remeshing::collapse_short_edges() {
    int n_collapses = 0;
    // with std::greater, short edges will be at the top of the queue:
    std::priority_queue<PairLE, std::vector<PairLE>, std::greater<PairLE>> queue;
    for (auto eh : mesh_.edges()) {
        queue.push(std::make_pair(edge_length_[eh], eh));
    }

    int count = 0;
    for (; !queue.empty(); queue.pop()) {
        count++;

        auto edge_priority_pair = queue.top();
        double edge_length = edge_priority_pair.first;
        OpenMesh::SmartEdgeHandle edge = edge_priority_pair.second;

        if (!edge.is_valid() || edge.deleted()) {
            std::cout << "eliminated invalid or deleted edge" << std::endl;
            continue;
        }

        double average_target_length = edge.vertices().avg([this](OpenMesh::SmartVertexHandle endpoint) {
            return target_length_[endpoint];
        });

        double maximum_length = 4. / 5. * average_target_length;
        if (edge_length >= maximum_length || edge_length_[edge] >= maximum_length) {
            continue;
        }

        auto candidate_edges = edge.halfedges()
            .filtered([](OpenMesh::SmartHalfedgeHandle halfedge) -> bool {
                // Boundary vertices must not be collapsed into non-boundary vertices
                return !(halfedge.from().is_boundary() && !halfedge.to().is_boundary());
            })
            .filtered([this](OpenMesh::SmartHalfedgeHandle halfedge) -> bool {
                // Halfedge must be collapsible
                return mesh_.is_collapse_ok(halfedge);
            });

        if (candidate_edges.begin() == candidate_edges.end()) {
            // Verify whether there is at least one halfedge that satisfies the constraints.
            // (argmin does not support empty ranges)
            continue;
        }

        OpenMesh::SmartHalfedgeHandle edge_to_be_collapsed = candidate_edges
            .argmin([](OpenMesh::SmartHalfedgeHandle halfedge) -> uint {
                // Collapse vertex with lower valence into vertex with higher valence
                return halfedge.from().valence();
            });

        auto enqueue_incident_edges = [&queue, this](OpenMesh::SmartVertexHandle collapsed_vertex) -> void {
            for (OpenMesh::SmartEdgeHandle current_edge : collapsed_vertex.edges()) {
                double updated_edge_length = mesh_.calc_edge_length(current_edge);

                edge_length_[current_edge] = updated_edge_length;
                queue.push(std::make_pair(updated_edge_length, current_edge));
            }
        };

        if (!edge_to_be_collapsed.to().is_boundary()) {
            // Boundary vertices should not move.
            // Non-boundary vertices move towards edge midpoint to better preserve shape.
            TriMesh::Point midpoint = edge_to_be_collapsed.edge().vertices().avg(
                [this](OpenMesh::SmartVertexHandle it) {
                    return mesh_.point(it);
            });

            mesh_.set_point(edge_to_be_collapsed.to(), midpoint);
        }

        mesh_.collapse(edge_to_be_collapsed);

        enqueue_incident_edges(edge_to_be_collapsed.to());
        n_collapses++;
    }

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
