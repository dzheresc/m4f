//! Sections 5.3-5.4: several securities and the Capital Asset Pricing
//! Model.
//!
//! Weights w, expected returns m and the covariance matrix c are plain
//! vectors; u = (1, ..., 1). The formulas are the book's matrix forms:
//! mu_V = w m^T, sigma_V^2 = w C w^T. Other names: r is the risk-free
//! return, mu_m and sigma_m (mu_M, sigma_M) the market portfolio's mean and
//! standard deviation.

/// A dense matrix, one `Vec` per row.
pub type Mat = Vec<Vec<f64>>;

// Linear algebra kept deliberately plain so all four languages compute
// alike.

/// a b^T.
pub fn dot(a: &[f64], b: &[f64]) -> f64 {
    let mut s = 0.0;
    for i in 0..a.len() {
        s += a[i] * b[i];
    }
    s
}

/// Sum of the entries.
pub fn sum(a: &[f64]) -> f64 {
    let mut s = 0.0;
    for x in a {
        s += x;
    }
    s
}

/// v M, for a row vector v. Row by row, so the matrix is read in memory
/// order and nothing is allocated per column. Each `out[j]` still sums
/// v[i] m[i][j] in order of i.
pub fn vec_mat(v: &[f64], m: &[Vec<f64>]) -> Vec<f64> {
    let mut out = vec![0.0; m[0].len()];
    for (vi, row) in v.iter().zip(m) {
        for (o, x) in out.iter_mut().zip(row) {
            *o += vi * x;
        }
    }
    out
}

/// Gauss-Jordan elimination with partial pivoting.
pub fn inverse(m: &[Vec<f64>]) -> Mat {
    let n = m.len();
    let mut a = vec![vec![0.0; 2 * n]; n];
    for i in 0..n {
        a[i][..n].copy_from_slice(&m[i][..n]);
        a[i][n + i] = 1.0;
    }
    for c in 0..n {
        let mut pivot = c;
        for r in c + 1..n {
            if a[r][c].abs() > a[pivot][c].abs() {
                pivot = r;
            }
        }
        a.swap(c, pivot);
        // Columns left of c are already unit columns (exact zeros off the
        // diagonal), so each row operation starts at column c.
        let d = a[c][c];
        for x in &mut a[c][c..] {
            *x /= d;
        }
        let pivot_row = a[c].clone();
        for (r, row) in a.iter_mut().enumerate() {
            if r == c {
                continue;
            }
            let k = row[c];
            if k == 0.0 {
                continue;
            }
            for (x, p) in row[c..].iter_mut().zip(&pivot_row[c..]) {
                *x -= k * p;
            }
        }
    }
    a.into_iter().map(|row| row[n..].to_vec()).collect()
}

/// Solves C x = b for each right-hand side b in `rhs`, for a symmetric C.
///
/// Uses a Cholesky factorisation C = L L^T (n^3/6 multiply-adds, then n^2
/// per right-hand side) when C is positive definite, as a covariance matrix
/// of non-redundant assets is; otherwise falls back to the explicit
/// inverse. For symmetric C, x = b C^-1 read as a row vector, so this
/// replaces `vec_mat(b, &inverse(c))` without forming the inverse.
pub fn solve_symmetric(c: &[Vec<f64>], mut rhs: Vec<Vec<f64>>) -> Vec<Vec<f64>> {
    let n = c.len();
    // Row-major lower triangle.
    let mut l = vec![0.0; n * n];
    let mut positive_definite = true;
    'rows: for i in 0..n {
        for j in 0..i + 1 {
            // Dot product of two row prefixes, as zipped slices so that the
            // inner loop has no bounds checks.
            let mut sum = c[i][j];
            for (a, b) in l[i * n..i * n + j].iter().zip(&l[j * n..j * n + j]) {
                sum -= a * b;
            }
            if i == j {
                if sum.is_nan() || sum <= 0.0 {
                    positive_definite = false;
                    break 'rows;
                }
                l[i * n + i] = sum.sqrt();
            } else {
                l[i * n + j] = sum / l[j * n + j];
            }
        }
    }
    if !positive_definite {
        let inv = inverse(c);
        for b in &mut rhs {
            *b = vec_mat(b, &inv);
        }
        return rhs;
    }
    for b in &mut rhs {
        // L z = b.
        for i in 0..n {
            let mut sum = b[i];
            for (a, z) in l[i * n..i * n + i].iter().zip(&b[..i]) {
                sum -= a * z;
            }
            b[i] = sum / l[i * n + i];
        }
        // L^T x = z.
        for i in (0..n).rev() {
            let mut sum = b[i];
            // Column i of L below the diagonal: every n-th entry.
            let column = l
                .get((i + 1) * n + i..)
                .unwrap_or_default()
                .iter()
                .step_by(n);
            for (a, x) in column.zip(&b[i + 1..]) {
                sum -= a * x;
            }
            b[i] = sum / l[i * n + i];
        }
    }
    rhs
}

/// c_ij = rho_ij sigma_i sigma_j, from the standard deviations `sigma` and
/// the correlation matrix `rho`.
pub fn covariance_matrix(sigma: &[f64], rho: &[Vec<f64>]) -> Mat {
    let n = sigma.len();
    (0..n)
        .map(|i| (0..n).map(|j| rho[i][j] * sigma[i] * sigma[j]).collect())
        .collect()
}

/// Section 5.3.1: mu_V = w m^T.
pub fn portfolio_mean(w: &[f64], m: &[f64]) -> f64 {
    dot(w, m)
}

/// sigma_V = sqrt(w C w^T).
pub fn portfolio_std(w: &[f64], c: &[Vec<f64>]) -> f64 {
    dot(&vec_mat(w, c), w).sqrt()
}

/// Section 5.3.2: efficient frontier. w = u C^-1 / (u C^-1 u^T).
pub fn min_variance_portfolio(c: &[Vec<f64>]) -> Vec<f64> {
    let mut w = solve_symmetric(c, vec![vec![1.0; c.len()]]).swap_remove(0);
    let s = sum(&w);
    for x in &mut w {
        *x /= s;
    }
    w
}

/// Minimum variance portfolio with expected return `mu_v`. With
/// a = u C^-1 u^T, b = u C^-1 m^T, c = m C^-1 m^T:
/// w = ((c - b mu_v) u C^-1 + (a mu_v - b) m C^-1) / (a c - b^2).
pub fn min_variance_line(m: &[f64], cov: &[Vec<f64>], mu_v: f64) -> Vec<f64> {
    let solved = solve_symmetric(cov, vec![vec![1.0; cov.len()], m.to_vec()]);
    let (u_cov_inv, m_cov_inv) = (&solved[0], &solved[1]);
    let a = sum(u_cov_inv);
    let b = dot(u_cov_inv, m);
    let c = dot(m_cov_inv, m);
    let det = a * c - b * b;
    (0..m.len())
        .map(|i| ((c - b * mu_v) * u_cov_inv[i] + (a * mu_v - b) * m_cov_inv[i]) / det)
        .collect()
}

/// Section 5.4: Capital Asset Pricing Model, risk-free return r. Tangency
/// portfolio: w_M = (m - r u) C^-1 / ((m - r u) C^-1 u^T).
pub fn market_portfolio(m: &[f64], c: &[Vec<f64>], r: f64) -> Vec<f64> {
    let excess: Vec<f64> = m.iter().map(|x| x - r).collect();
    let mut w = solve_symmetric(c, vec![excess]).swap_remove(0);
    let s = sum(&w);
    for x in &mut w {
        *x /= s;
    }
    w
}

/// Section 5.4.1: mu_V = r + (mu_M - r) / sigma_M * sigma_V.
pub fn capital_market_line(sigma_v: f64, r: f64, mu_m: f64, sigma_m: f64) -> f64 {
    r + (mu_m - r) / sigma_m * sigma_v
}

/// Section 5.4.2: beta_V = Cov(K_V, K_M) / sigma_M^2
/// = w C w_M^T / (w_M C w_M^T).
pub fn beta(w: &[f64], w_market: &[f64], c: &[Vec<f64>]) -> f64 {
    // C is symmetric, so w_M C = (C w_M^T)^T.
    let c_w_market = vec_mat(w_market, c);
    dot(&c_w_market, w) / dot(&c_w_market, w_market)
}

/// Section 5.4.3: mu_V = r + (mu_M - r) beta_V.
pub fn security_market_line(beta: f64, r: f64, mu_m: f64) -> f64 {
    r + (mu_m - r) * beta
}
