"""Section 2.1 of Chapter 2: time value of money.

Mathematics for Finance (Capinski & Zastawniak), Chapter 2: Risk-Free Assets.

Book notation: P is the principal (`principal`), V the future value
(`value`), r the annual interest rate, t the time in years, m the number of
compounding periods per year; r, t, m, n, i and k keep the book's letters
(Google Python style guide, section 3.16.5).
"""

from __future__ import annotations

import math


# 2.1.1 Simple interest: V(t) = (1 + t r) P.
def simple_value(principal: float, r: float, t: float) -> float:
    return (1 + t * r) * principal


def simple_present_value(value: float, r: float, t: float) -> float:
    return value / (1 + t * r)


def simple_rate(principal: float, value: float, t: float) -> float:
    """Rate r such that `principal` grows to `value` in time t."""
    return (value / principal - 1) / t


# 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P.
def periodic_value(principal: float, r: float, m: int, t: float) -> float:
    return (1 + r / m) ** (t * m) * principal


def periodic_present_value(value: float, r: float, m: int, t: float) -> float:
    return value * (1 + r / m) ** (-t * m)


def effective_rate(r: float, m: int) -> float:
    """Effective annual rate r_e = (1 + r/m)^m - 1."""
    return (1 + r / m) ** m - 1


# 2.1.3 Streams of payments: n payments C (`payment`) at the end of each
# period, per-period rate i (for annual payments with annual compounding,
# i = r).
def annuity_factor(i: float, n: int) -> float:
    """PA(i, n) = (1 - (1+i)^-n) / i, the present value of n unit payments."""
    return (1 - (1 + i) ** -n) / i


def annuity_present_value(payment: float, i: float, n: int) -> float:
    return payment * annuity_factor(i, n)


def annuity_future_value(payment: float, i: float, n: int) -> float:
    return payment * ((1 + i) ** n - 1) / i


def perpetuity_present_value(payment: float, i: float) -> float:
    return payment / i


def loan_installment(loan: float, i: float, n: int) -> float:
    """Payment C repaying a loan PA in n installments: C = PA / PA(i, n)."""
    return loan / annuity_factor(i, n)


# 2.1.4 Continuous compounding: V(t) = e^(t r) P.
def continuous_value(principal: float, r: float, t: float) -> float:
    return math.exp(t * r) * principal


def continuous_present_value(value: float, r: float, t: float) -> float:
    return value * math.exp(-t * r)


def continuous_effective_rate(r: float) -> float:
    return math.exp(r) - 1


# 2.1.5 Comparing compounding methods: rates giving the same growth. r_m is a
# rate compounded m times a year, r_c a continuously compounded rate.
def continuous_from_periodic(r_m: float, m: int) -> float:
    return m * math.log(1 + r_m / m)


def periodic_from_continuous(r_c: float, m: int) -> float:
    return m * (math.exp(r_c / m) - 1)


def periodic_from_periodic(r_m: float, m: int, k: int) -> float:
    """Rate compounded k times a year equivalent to r_m compounded m times."""
    return k * ((1 + r_m / m) ** (m / k) - 1)
