//! Sections 11.1-11.2: the binomial tree model of the short rate.
//!
//! Time steps of length tau; node (n,k) is reached after n steps of which k
//! were "up". A `Lattice` holds one value per node: `l[n][k]`, k = 0..n. The
//! short rate r(n,k) applies over [n, n+1], so the one-step bond is
//! B(n,n+1;k) = e^(-r(n,k) tau) and the money market account grows by
//! e^(r tau) each step. Risk-neutral probabilities p*(n,k) of an up move may
//! vary by node.

use crate::max;

/// One value per node: `l[n][k]`, k = 0..n.
pub type Lattice = Vec<Vec<f64>>;

/// A value attached to node (n, k). A trait object, like the C++
/// `std::function`.
pub type NodeFn<'a> = &'a dyn Fn(i32, i32) -> f64;

/// Additive tree: r(n,k) = r0 + k up + (n-k) down (`down` is negative for a
/// fall).
pub fn rate_lattice(r0: f64, up: f64, down: f64, num_steps: i32) -> Lattice {
    (0..num_steps + 1)
        .map(|n| {
            (0..n + 1)
                .map(|k| r0 + f64::from(k) * up + f64::from(n - k) * down)
                .collect()
        })
        .collect()
}

/// The same up probability `p` at every node of levels 0..num_steps-1.
pub fn constant_probs(p: f64, num_steps: i32) -> Lattice {
    (0..num_steps as usize).map(|n| vec![p; n + 1]).collect()
}

/// B(n,n+1) = e^(-r tau).
#[inline]
pub fn one_step_price(r: f64, tau: f64) -> f64 {
    (-r * tau).exp()
}

/// A(0..len(path)) along a path of 'u'/'d' moves: A(n+1) = A(n) e^(r(n,k)
/// tau).
pub fn money_market(rates: &[Vec<f64>], tau: f64, path: &str) -> Vec<f64> {
    let mut account = Vec::with_capacity(path.len() + 1);
    account.push(1.0);
    let mut a = 1.0;
    let mut k = 0;
    for (n, step) in path.bytes().enumerate() {
        a *= (rates[n][k] * tau).exp();
        account.push(a);
        if step == b'u' {
            k += 1;
        }
    }
    account
}

/// Backward induction from level N = `terminal.len()`-1 down to 0:
///
/// ```text
/// V(n,k) = e^(-r(n,k) tau) [ p* V(n+1,k+1) + (1-p*) V(n+1,k) + payment(n,k) ]
/// ```
///
/// with the payment (if any) made at n+1 but fixed at (n,k), and, if `early`
/// is given, V(n,k) = max(V(n,k), early(n,k)) (American exercise). (C++
/// defaults both to none.)
pub fn discount_back(
    rates: &[Vec<f64>],
    probs: &[Vec<f64>],
    tau: f64,
    terminal: Vec<f64>,
    payment: Option<NodeFn>,
    early: Option<NodeFn>,
) -> Lattice {
    let num_steps = terminal.len() - 1;
    let mut values: Lattice = vec![Vec::new(); num_steps + 1];
    values[num_steps] = terminal;
    for n in (0..num_steps).rev() {
        let next = &values[n + 1];
        let (p_row, r_row) = (&probs[n][..n + 1], &rates[n][..n + 1]);
        let row = (0..n + 1)
            .map(|k| {
                let p = p_row[k];
                let mut cont = p * next[k + 1] + (1.0 - p) * next[k];
                if let Some(payment) = payment {
                    cont += payment(n as i32, k as i32);
                }
                let v = one_step_price(r_row[k], tau) * cont;
                match early {
                    Some(early) => max(v, early(n as i32, k as i32)),
                    None => v,
                }
            })
            .collect();
        values[n] = row;
    }
    values
}

/// 11.2: B(n,N;k) for all nodes, from B(N,N) = 1.
pub fn bond_lattice(rates: &[Vec<f64>], probs: &[Vec<f64>], tau: f64, maturity: i32) -> Lattice {
    discount_back(
        rates,
        probs,
        tau,
        vec![1.0; maturity as usize + 1],
        None,
        None,
    )
}

/// B(0,1), ..., B(0,N), by forward induction over the Arrow-Debreu state
/// prices Q(n,k), the time-0 value of 1 paid at node (n,k):
/// B(0,n+1) = sum_k Q(n,k) e^(-r(n,k) tau), and Q(n+1,.) receives
/// Q(n,k) e^(-r(n,k) tau) times p* (up) or 1 - p* (down). One O(N^2) pass
/// instead of a backward induction per maturity; equal to
/// `bond_lattice(..)[0][0]` up to rounding.
pub fn bond_prices(rates: &[Vec<f64>], probs: &[Vec<f64>], tau: f64, num_steps: i32) -> Vec<f64> {
    let num_steps = num_steps as usize;
    let mut out = Vec::with_capacity(num_steps);
    let mut q = vec![1.0];
    let mut next = Vec::with_capacity(num_steps + 1);
    for n in 0..num_steps {
        next.clear();
        next.resize(n + 2, 0.0);
        let (r_row, p_row) = (&rates[n][..n + 1], &probs[n][..n + 1]);
        let mut bond = 0.0;
        for k in 0..n + 1 {
            let d = q[k] * one_step_price(r_row[k], tau);
            bond += d;
            let p = p_row[k];
            next[k + 1] += p * d;
            next[k] += (1.0 - p) * d;
        }
        out.push(bond);
        std::mem::swap(&mut q, &mut next);
    }
    out
}

/// y(n,N) = -ln B(n,N) / ((N-n) tau).
pub fn yield_rate(bond_price: f64, n: i32, maturity: i32, tau: f64) -> f64 {
    -bond_price.ln() / (f64::from(maturity - n) * tau)
}

/// 11.2.1: the p*(n,k) that prices the bond with lattice `bond` at node
/// (n,k): B(n,N;k) e^(r tau) = p* B(n+1,N;k+1) + (1-p*) B(n+1,N;k).
pub fn implied_prob(bond: &[Vec<f64>], rates: &[Vec<f64>], tau: f64, n: i32, k: i32) -> f64 {
    let (n, k) = (n as usize, k as usize);
    (bond[n][k] * (rates[n][k] * tau).exp() - bond[n + 1][k])
        / (bond[n + 1][k + 1] - bond[n + 1][k])
}

/// `implied_prob` at every node of levels 0..N-2 of `bond`.
pub fn implied_probs(bond: &[Vec<f64>], rates: &[Vec<f64>], tau: f64) -> Lattice {
    let maturity = bond.len() as i32 - 1;
    (0..maturity - 1)
        .map(|n| {
            (0..n + 1)
                .map(|k| implied_prob(bond, rates, tau, n, k))
                .collect()
        })
        .collect()
}
