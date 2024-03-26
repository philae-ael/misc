import numpy as np
import matplotlib.pyplot as plt


def metropolis(N, dist, x0=0.0, candidate_dist=None):
    if candidate_dist is None:
        candidate_dist = lambda x: np.random.normal(x, 1, 1)  # noqa: E731

    xi = x0
    for _ in range(N):
        candidate = candidate_dist(xi)
        acceptance = dist(candidate) / dist(xi)
        u = np.random.random(1)
        if u <= acceptance:
            xi = candidate
    return xi


def f(x):
    return 0.5* (1 / np.sqrt(2 * np.pi) / 4 * np.exp(-0.5 * ((x - 2) / 4) ** 2) + 1 / np.sqrt(
        2 * np.pi
    ) / 1 * np.exp(-0.5 * (x / 1) ** 2))


xs = []
xi = 0.0
for _ in range(5000):
    xi = metropolis(20, f, xi)
    xs.append(xi)

counts, bins = np.histogram(xs, 50, density=True)
plt.stairs(counts, bins)
x = np.linspace(-8, 12, 200)
plt.plot(x, f(x))
plt.show()
