"""Mathematics for Finance (Capinski & Zastawniak), Chapter 2: Risk-Free Assets.

Section 2.1, time value of money. Throughout, P is the principal (present value),
r the annual interest rate, t the time in years, m the number of compounding
periods per year.
"""
from __future__ import annotations

import math


# 2.1.1 Simple interest: V(t) = (1 + t r) P
def simple_value(P: float, r: float, t: float) -> float:
    return (1 + t * r) * P


def simple_present_value(V: float, r: float, t: float) -> float:
    return V / (1 + t * r)


def simple_rate(P: float, V: float, t: float) -> float:
    """Rate r such that P grows to V in time t under simple interest."""
    return (V / P - 1) / t


# 2.1.2 Periodic compounding: V(t) = (1 + r/m)^(t m) P
def periodic_value(P: float, r: float, m: int, t: float) -> float:
    return (1 + r / m) ** (t * m) * P


def periodic_present_value(V: float, r: float, m: int, t: float) -> float:
    return V * (1 + r / m) ** (-t * m)


def effective_rate(r: float, m: int) -> float:
    """Effective annual rate r_e = (1 + r/m)^m - 1."""
    return (1 + r / m) ** m - 1


# 2.1.3 Streams of payments: n payments C at the end of each period, per-period
# rate i (for annual payments with annual compounding, i = r).
def annuity_factor(i: float, n: int) -> float:
    """PA(i, n) = (1 - (1+i)^-n) / i, the present value of n unit payments."""
    return (1 - (1 + i) ** -n) / i


def annuity_present_value(C: float, i: float, n: int) -> float:
    return C * annuity_factor(i, n)


def annuity_future_value(C: float, i: float, n: int) -> float:
    return C * ((1 + i) ** n - 1) / i


def perpetuity_present_value(C: float, i: float) -> float:
    return C / i


def loan_installment(PA: float, i: float, n: int) -> float:
    """Payment C repaying a loan PA in n installments: C = PA / annuity_factor."""
    return PA / annuity_factor(i, n)


# 2.1.4 Continuous compounding: V(t) = e^(t r) P
def continuous_value(P: float, r: float, t: float) -> float:
    return math.exp(t * r) * P


def continuous_present_value(V: float, r: float, t: float) -> float:
    return V * math.exp(-t * r)


def continuous_effective_rate(r: float) -> float:
    return math.exp(r) - 1


# 2.1.5 Comparing compounding methods: rates giving the same growth.
def continuous_from_periodic(r_m: float, m: int) -> float:
    return m * math.log(1 + r_m / m)


def periodic_from_continuous(r_c: float, m: int) -> float:
    return m * (math.exp(r_c / m) - 1)


def periodic_from_periodic(r_m: float, m: int, k: int) -> float:
    """Rate compounded k times a year equivalent to r_m compounded m times a year."""
    return k * ((1 + r_m / m) ** (m / k) - 1)
