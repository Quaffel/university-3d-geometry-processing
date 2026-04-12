# exercises - series 6

## team members

- Simon Kolly
- Niklas Radomski
- Nicolas Willimann

## exercise notes

### octree depth

- octree depth correlates with the resolution of the triangle mesh
  - larger values for the octree depths significantly increase computation time
- default setting of 8 produces good results for all provided meshes
  - computation finishes very quickly
  - overall shape preserved, finer details are only exposed for higher depth values

### weight output property

- higher values occur in regions where the input point cloud is dense
- can be interpreted as a confidence value for the approximation in the corresponding neighborhood
  - useful for visualization and post-processing, e.g., trimming
- probably correlates with local depth of octree

### screened poisson surface reconstruction

- according to the [paper](https://www.cs.jhu.edu/~misha/MyPapers/ToG13.pdf) that introduces Screened Poisson Surface Reconstruction:
  - motivated by the empiric observation that standard Poisson Surface Reconstruction has the tendency to oversmooth the data
  - introduces an additional screening term that encourages the reconstructed surface to pass directly through a (sparse) set of points from the dataset
  - when properly configured, this modification significantly improves fit accuracy without amplifying noise
- weight parameter controls importance of screening term, and thus how strongly we want to incentivize reconstructions that pass directly through data points
  - larger values bear the risk of noise amplification

### smooth signed distance (ssd) surface reconstruction

- alternative variational formulation for the problem of reconstructing a watertight surface from a finite set of oriented points
  - Poisson: implicit function is forced to approximate the (discontinuous) indicator function of the volume
  - SSD: implicit function is forced to be a smooth approximation of the (continuous) signed distance function
- alternative construction significantly simplifies algorithm while preserving quality

### observations

- when using SSD (Smooth Signed Distance) Surface Reconstruction, ghost artifacts (two additional surfaces, one being significantly larger than the amadillo figure itself) start to show up
  - in some models (e.g., `ArmadilloBack.ply`), the ghost surface is discontinuous with the figure itself, whereas in others, they form a single continuous surface
  - all ghost surfaces have low weight values assigned which underpins usefulness of weight values for post-processing
- we observe the same lighting issue that was reported by another group on the forum
  - figures seem to "glow" "inside" and seem to have only shadows "outside"
