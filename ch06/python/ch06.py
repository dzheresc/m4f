#!/usr/bin/env python3
"""Chapter 6 worked examples.

The output is byte-identical to the C++ and Kotlin versions. The numbers are
illustrative (not the book's own); the formulas follow the book.
"""

import forwards
import futures


def fmt(v: float) -> str:
    """Four decimals; adding 0.0 turns -0.0 into 0.0."""
    return f"{v + 0.0:.4f}"


def fmt_list(values: list[float]) -> str:
    """ "[a, b, c]" with each entry formatted by fmt()."""
    return "[" + ", ".join(fmt(v) for v in values) + "]"


def forward_price_section(out: list[str], f0: float) -> None:
    """Section 6.1.1."""
    out.append(
        "\n== 6.1.1 Forward price: F(0,T) = S(0) e^(rT), S(0)=100 r=0.08 "
        "T=0.75 =="
    )
    out.append(f"No dividends: F(0,T)={fmt(f0)}")
    with_dividends = forwards.forward_price_dividends(
        100.0, 0.08, 0.75, [(2.0, 0.25), (2.0, 0.5)]
    )
    out.append(
        f"Dividends 2 at t=0.25 and 2 at t=0.5: F(0,T)={fmt(with_dividends)}"
    )
    with_yield = forwards.forward_price_yield(100.0, 0.08, 0.03, 0.75)
    out.append(f"Dividend yield 3%: F(0,T)={fmt(with_yield)}")
    currency = forwards.forward_price_yield(1.25, 0.05, 0.03, 1.0)
    out.append(
        "Currency 1.25 USD per EUR, r_USD=0.05 r_EUR=0.03, T=1: "
        f"F={fmt(currency)}"
    )
    for quoted in (108.0, 104.0):
        if quoted > f0:
            side = "cash and carry: buy stock, sell forward"
        else:
            side = "reverse: short stock, buy forward"
        profit = forwards.forward_arbitrage_profit(quoted, 100.0, 0.08, 0.75)
        out.append(f"Quoted F={fmt(quoted)}: {side}, profit at T={fmt(profit)}")


def forward_value_section(out: list[str], f0: float) -> None:
    """Section 6.1.2."""
    out.append(
        "\n== 6.1.2 Value of a forward contract: V(t) = (F(t,T) - F(0,T)) "
        "e^(-r(T-t)) =="
    )
    for st in (104.0, 98.0):
        ft = forwards.forward_price(st, 0.08, 0.25)
        value = forwards.long_forward_value(f0, ft, 0.08, 0.25)
        neg_value = -forwards.long_forward_value(f0, ft, 0.08, 0.25)
        out.append(
            f"t=0.5 S(t)={fmt(st)}: F(t,T)={fmt(ft)} long V(t)={fmt(value)} "
            f"short V(t)={fmt(neg_value)}"
        )


def futures_pricing_section(out: list[str]) -> None:
    """Section 6.2.1."""
    out.append(
        "\n== 6.2.1 Futures pricing: f(n) = E*(S(N) | S(n)), S(0)=100 u=0.10 "
        "d=-0.05 r=0.03 N=3 =="
    )
    tree = futures.futures_tree(100.0, 0.10, -0.05, 0.03, 3)
    for n in range(4):
        forward = [
            100.0 * 1.1**k * 0.95 ** (n - k) * 1.03 ** (3 - n)
            for k in range(n, -1, -1)
        ]
        out.append(
            f"n={n}: futures={fmt_list(tree[n][::-1])} "
            f"forward={fmt_list(forward)}"
        )
    out.append(
        "Marking to market: 100 units, initial margin 1000, maintenance 750"
    )
    prices = [106.0, 104.5, 101.0, 103.5, 99.0, 102.0]
    days = futures.margin_account(prices, 100.0, 1000.0, 750.0)
    for n, (balance, call) in enumerate(days, start=1):
        out.append(
            f"day {n}: f={fmt(prices[n])} balance={fmt(balance)} "
            f"margin call={fmt(call)}"
        )


def hedging_section(out: list[str]) -> None:
    """Section 6.2.2."""
    out.append(
        "\n== 6.2.2 Hedging with futures: four equally likely scenarios =="
    )
    ds = [-6.0, -1.0, 2.0, 5.0]
    df = [-5.0, -2.0, 3.0, 4.0]
    p = [0.25, 0.25, 0.25, 0.25]
    h = futures.min_variance_hedge_ratio(ds, df, p)
    out.append(f"dS={fmt_list(ds)} df={fmt_list(df)}")
    out.append(
        f"h*={fmt(h)}; std of spot change: "
        f"unhedged {fmt(futures.hedged_std(0.0, ds, df, p))}, "
        f"h=1 {fmt(futures.hedged_std(1.0, ds, df, p))}, "
        f"h* {fmt(futures.hedged_std(h, ds, df, p))}"
    )
    cost = [112.0 - (112.0 - 106.0), 95.0 - (95.0 - 106.0)]
    out.append(
        "Perfect hedge: buy at T with a long futures at f(0)=106; spot at T "
        f"112 or 95 -> cost {fmt_list(cost)}"
    )


def report() -> str:
    """The full chapter 6 report."""
    out = [
        "Chapter 6: Forward and Futures Contracts (illustrative numbers, book"
        " formulas)"
    ]
    f0 = forwards.forward_price(100.0, 0.08, 0.75)
    forward_price_section(out, f0)
    forward_value_section(out, f0)
    futures_pricing_section(out)
    hedging_section(out)
    return "\n".join(out) + "\n"


def main() -> None:
    print(report(), end="")


if __name__ == "__main__":
    main()
