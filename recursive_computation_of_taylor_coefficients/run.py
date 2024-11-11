from functools import cache

import sympy
from sympy import Function, dsolve, series, srepr, sympify
from sympy.abc import x, y


@cache
def recursive_taylor(rv, x, y, level, x0, y0, yexpr):
    match rv:
        case rv if rv == x:
            if level == 0:
                return x0
            if level == 1:
                return 1
            return 0
        case rv if rv == y:
            if level == 0:
                return y0
            return (
                1
                / sympify(level)
                * recursive_taylor(yexpr, x, y, level - 1, x0, y0, yexpr)
            )
        case sympy.Add(args=(a, b)):
            return recursive_taylor(a, x, y, level, x0, y0, yexpr) + recursive_taylor(
                b, x, y, level, x0, y0, yexpr
            )
        case sympy.Mul(args=(a, b)):
            return sum(
                recursive_taylor(a, x, y, i, x0, y0, yexpr)
                * recursive_taylor(b, x, y, level - i, x0, y0, yexpr)
                for i in range(level + 1)
            )
        case c if isinstance(c, int) or isinstance(c, float) or isinstance(
            c, sympy.Integer
        ):
            return sympify(rv) if level == 0 else 0
        case _:
            raise Exception("idk", rv)


with sympy.evaluate(False):
    expr = y * x * x
print(srepr(expr))
__import__("pprint").pprint(
    [recursive_taylor(y, x, y, i, 0, 1, expr) for i in range(5)]
)

f = Function("f")
print(
    series(dsolve(f(x) * x * x - f(x).diff(x), f(x), ics={f(0): 1}).args[1], x)
    # .as_coefficients_dict()
    # .values()
)
