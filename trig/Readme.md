# Compute values of trig functions using cordic algorithm


## Algorithm
Let $\vec{v_0} = \begin{bmatrix} 1 \\ 0 \end{bmatrix}$ and $\vec{v_{n+1}} = R_n\vec{v_n}$ with $R_n$ a rotation matrix of angle $\gamma_n$.

We have $\vec{v_n} = \begin{bmatrix} \cos\Gamma_n \\ \sin\Gamma_n\end{bmatrix}$ where $\Gamma_n = \sum_{i=0}^{n-1}\gamma_i$.

A clever choice of values for $\gamma_i$ would allow to approximate $\cos\Gamma$, $\sin\Gamma$ and $\tan \Gamma$.

Such a choice can be: $\gamma_i = \pm \arctan 2^{- i}$ that allows to approxiate values of $\theta$ such that $-2 \le \tan \theta \le 2$.

With this choice $R_n = K_n \begin{bmatrix} 1 & -\sigma_i2^{-i} \\ \sigma_i2^{-i} & 1 \end{bmatrix}$ with $\sigma_i = \pm 1$ and $K_n = \frac{1}{\sqrt{1 + 2^{-2i}}}$.

Given an angle $\theta$, the signs $\sigma_i$ should be computed. To do this, how much the angle was rotated should be tracked.
$\beta_0 = \theta$ $\beta_{n+1} = \beta_n - \sigma_n\gamma_n$.
Thus the values of$\gamma_i = \arctan(2^{-i})$ should be precomputed for small values of i and approximated using $\arctan(x) = x + O(x^3)$


## References:
- [What inspired me](https://www.youtube.com/watch?v=NVRXK1Idbv8)
- [Wiki CORDIC algorithm](https://en.wikipedia.org/wiki/CORDIC)
