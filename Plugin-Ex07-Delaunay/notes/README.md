# exercises - series 7

## team members

- Simon Kolly
- Niklas Radomski
- Nicolas Willimann

## exercise notes

### exercise 1: express "in-circle" test using a 3x3 matrix

  $$
  vol(A', B', C', D') = -\frac{1}{6} \det \begin{bmatrix}
    A_x & A_y & A_x^2 + A_y^2 & 1 \\
    B_x & B_y & B_x^2 + B_y2 & 1 \\
    C_x & C_y & C_x^2 + C_y^2 & 1 \\
    D_x & D_y & D_x^2 + D_y^2 & 1
  \end{bmatrix}
  $$

- proof
  - For brevity, we represent the $4 \times 4$ matrix in a more compact form and drop the coefficient:

    $$
    = - \det \begin{bmatrix}
      (A')^T & 1 \\
      (B')^T & 1 \\
      (C')^T & 1 \\
      (D')^T & 1
    \end{bmatrix}
    $$

  - The transpose of a matrix has the same determinant as the original matrix. It thus holds that:

    $$
    = - \det \begin{bmatrix}
      A' & B' & C' & D' \\
      1 & 1 & 1 & 1
    \end{bmatrix}
    $$

  - Swapping two (adjacent) rows in a matrix swaps the sign of the respective determinant. If we move the row full of 1s from the bottom to the top, we perform 3 swaps, so the sign needs to be inverted:

    $$
    = \det \begin{bmatrix}
      1 & 1 & 1 & 1 \\
      A' & B' & C' & D'
    \end{bmatrix} \\
    $$

  - Essential Gaussian row modifications do not change a matrix' determinant (equally applies to column modifications as transpose does not change the determinant, as established earlier). We can thus subtract the first column from all other columns without changing the determinant.

    Rationale: Subtracting a column from the other columns modifies the other columns in a linearly dependent way.
    Consider a triangle in 2d: If you move the triangle's top vertex along an axis that is parallel to the basis (i.e., in a linearly dependent way), the area of the triangle does not change.
    This trivially holds, as the area of a triangle solely depends on its height and the length of its basis.

    $$
    = \det \begin{bmatrix}
      1 & 0 & 0 & 0 \\
      A' & B' - A' & C' - A' & D' - A'
    \end{bmatrix}
    $$

  - Laplace expansion (aka as the Laplacian development theorem) allows us to express the determinant of a 4x4 matrix as a sum of the determinants of several 3x3 submatrices.
    More generally: Laplace expansion expresses the determinant of an $n \times n$ matrix as a weighted sum of minors (determinants of some $(n - 1) \times (n - 1)$ submatrices).  

    The values in the first row are used as coefficients for the determinants of the minors. This significantly simplifies the expression in our case, as all except one values in the first row are zeros.  

    $$
    \begin{align*}
    &= 1 \cdot \det \left[B' - A' \mid C' - A' \mid D' - A' \right] + 0 \cdot \det [\dots] + \dots \\
    &=  \det \left[a \mid b \mid c \right]
    \end{align*}
    $$

  - The composed matrix $[a \mid b \mid c]$ describes transformation that scales unit vectors of Euclidean space to vectors $a$, $b$, and $c$.
    Determinant thus describes the change of volume from a unit cube to the tetrahedron spanned by $a$, $b$, and $c$.

    $$
    = 6 \cdot vol(A', B', C', D')
    $$

    This is exaclty the same as the initial expression, which concludes our proof.
    Note that all mentions of volumes in this proof refer to signed volumes.
