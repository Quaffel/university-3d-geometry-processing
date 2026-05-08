


```cpp
// variant 1:
// SpMat system = diag_vector.asDiagonal() - (settings.timestep * laplacian_coefficients);

// variant 2:
SpMat system = -(settings.timestep * laplacian_coefficients);
system += diag_vector.asDiagonal();
```