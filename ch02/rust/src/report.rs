//! The chapter's worked examples, printed as text. The text is
//! byte-identical across the C++, Kotlin, Python and Rust implementations.

use std::fmt::{self, Display, Write};

use crate::interest::{
    annuity_future_value, annuity_present_value, continuous_effective_rate,
    continuous_from_periodic, continuous_present_value, continuous_value, effective_rate,
    loan_installment, periodic_from_continuous, periodic_from_periodic, periodic_present_value,
    periodic_value, perpetuity_present_value, simple_present_value, simple_rate, simple_value,
};
use crate::money_market::{
    coupon_bond_price, implied_rate, money_market_rollover, money_market_value, yield_to_maturity,
    zero_coupon_price,
};

/// Four decimals, round-half-even on the exact double, like `{:.4f}` in
/// C++ `std::format`. Adding 0.0 turns -0.0 into 0.0 so that no "-0.0000"
/// appears.
struct F4(f64);

impl Display for F4 {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:.4}", self.0 + 0.0)
    }
}

/// Prints the chapter's worked examples to `os`.
pub fn print_report(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "Chapter 2: Risk-Free Assets (illustrative numbers, book formulas)"
    )?;
    print_simple_interest(os)?;
    print_periodic_compounding(os)?;
    print_streams_of_payments(os)?;
    print_continuous_compounding(os)?;
    print_comparing_methods(os)?;
    print_zero_coupon_bonds(os)?;
    print_coupon_bonds(os)?;
    print_money_market_account(os)
}

fn print_simple_interest(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 2.1.1 Simple interest: V(t) = (1 + t r) P ==")?;
    for t in [0.5, 1.0, 2.0] {
        writeln!(
            os,
            "P=2000 r=0.08 t={}: V(t)={}",
            F4(t),
            F4(simple_value(2000.0, 0.08, t))
        )?;
    }
    writeln!(
        os,
        "Present value of 1000 due at t=0.5, r=0.10: {}",
        F4(simple_present_value(1000.0, 0.10, 0.5))
    )?;
    writeln!(
        os,
        "Rate turning 1000 into 1100 in 2 years: {}",
        F4(simple_rate(1000.0, 1100.0, 2.0))
    )
}

fn print_periodic_compounding(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P =="
    )?;
    for m in [1, 2, 4, 12, 365] {
        writeln!(
            os,
            "P=1000 r=0.12 m={m}: V(1)={} effective rate={}",
            F4(periodic_value(1000.0, 0.12, m, 1.0)),
            F4(effective_rate(0.12, m))
        )?;
    }
    writeln!(
        os,
        "Present value of 1000 due in 3 years, r=0.10 annual: {}",
        F4(periodic_present_value(1000.0, 0.10, 1, 3.0))
    )
}

fn print_streams_of_payments(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 2.1.3 Streams of payments ==")?;
    writeln!(
        os,
        "Annuity of 10 x 1000 at 10%: PV={} FV={}",
        F4(annuity_present_value(1000.0, 0.10, 10)),
        F4(annuity_future_value(1000.0, 0.10, 10))
    )?;
    writeln!(
        os,
        "Perpetuity of 100 a year at 5%: PV={}",
        F4(perpetuity_present_value(100.0, 0.05))
    )?;
    let installment = loan_installment(100000.0, 0.06 / 12.0, 300);
    writeln!(
        os,
        "Loan 100000 over 25 years, monthly at 6%: installment={} balance after 10 years={}",
        F4(installment),
        F4(annuity_present_value(installment, 0.06 / 12.0, 180))
    )
}

fn print_continuous_compounding(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 2.1.4 Continuous compounding: V(t) = e^(t r) P ==")?;
    writeln!(
        os,
        "P=1000 r=0.12: V(1)={} effective rate={}",
        F4(continuous_value(1000.0, 0.12, 1.0)),
        F4(continuous_effective_rate(0.12))
    )?;
    writeln!(
        os,
        "Present value of 1000 due in 3 years, r=0.10 continuous: {}",
        F4(continuous_present_value(1000.0, 0.10, 3.0))
    )
}

fn print_comparing_methods(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 2.1.5 Comparing compounding methods (r=0.10, one year) =="
    )?;
    writeln!(
        os,
        "Growth factor: simple {}, m=1 {}, m=2 {}, m=12 {}, continuous {}",
        F4(simple_value(1.0, 0.10, 1.0)),
        F4(periodic_value(1.0, 0.10, 1, 1.0)),
        F4(periodic_value(1.0, 0.10, 2, 1.0)),
        F4(periodic_value(1.0, 0.10, 12, 1.0)),
        F4(continuous_value(1.0, 0.10, 1.0))
    )?;
    writeln!(
        os,
        "12% monthly = {} continuous",
        F4(continuous_from_periodic(0.12, 12))
    )?;
    writeln!(
        os,
        "10% continuous = {} annual = {} monthly",
        F4(periodic_from_continuous(0.10, 1)),
        F4(periodic_from_continuous(0.10, 12))
    )?;
    writeln!(
        os,
        "12% quarterly = {} monthly = {} annual",
        F4(periodic_from_periodic(0.12, 4, 12)),
        F4(periodic_from_periodic(0.12, 4, 1))
    )
}

fn print_zero_coupon_bonds(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 2.2.1 Zero-coupon bonds ==")?;
    writeln!(
        os,
        "B(0,1)=0.92 implies: annual {}, semi-annual {}, continuous {}",
        F4(implied_rate(0.92, 1.0, Some(1))),
        F4(implied_rate(0.92, 1.0, Some(2))),
        F4(implied_rate(0.92, 1.0, None))
    )?;
    writeln!(
        os,
        "Face 100, T=2 at 10%: annual {}, continuous {}",
        F4(100.0 * zero_coupon_price(0.10, 2.0, Some(1))),
        F4(100.0 * zero_coupon_price(0.10, 2.0, None))
    )
}

fn print_coupon_bonds(os: &mut impl Write) -> fmt::Result {
    writeln!(
        os,
        "\n== 2.2.2 Coupon bonds: F=100, annual coupon 10, 3 years =="
    )?;
    for y in [0.12, 0.10, 0.08] {
        writeln!(
            os,
            "yield {}: price {}",
            F4(y),
            F4(coupon_bond_price(100.0, 10.0, 3, y))
        )?;
    }
    writeln!(
        os,
        "price 95.0000: yield to maturity {}",
        F4(yield_to_maturity(95.0, 100.0, 10.0, 3))
    )
}

fn print_money_market_account(os: &mut impl Write) -> fmt::Result {
    writeln!(os, "\n== 2.2.3 Money market account ==")?;
    writeln!(
        os,
        "A(0)=100 at 8% continuous: A(1)={}",
        F4(money_market_value(100.0, 0.08, 1.0))
    )?;
    let a1 = money_market_rollover(100.0, &[0.98, 0.985, 0.97, 0.99]);
    writeln!(
        os,
        "Rolling over 3-month zero-coupon bonds (B=0.98, 0.985, 0.97, 0.99): A(1)={} continuous \
         rate {}",
        F4(a1),
        F4((a1 / 100.0).ln())
    )
}
