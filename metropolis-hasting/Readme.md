# Metropolis Haslings algotirhm (MCMC)

## What 

Create a sequence of random samples following a probability distribution 

The algorithm implemented is only the Metropolis Algorithm without the Hasling extension, I think

## References

- [Wikipedia (Markov chain Monte Carlo)](https://en.wikipedia.org/wiki/Markov_chain_Monte_Carlo)
- [Wikipedia (Metropolis–Hastings algorithm)](https://en.wikipedia.org/wiki/Metropolis%E2%80%93Hastings_algorithm)
- [The Metropolis-Hastings algorithm - Preprint, Robert, Christian P.](http://arxiv.org/abs/1504.01896)


## Notes

- Does not need the distribution to have a total mass of 1 -> Metropolis-Hastling($\lambda f$) =  Metropolis-Hastling(f)
- Can be used to compute an integral (the expected value of the given function) / normalize a function

## Questions and follow up
- convergence/Burn-in? Speed?
- How good is the sequence? Influence of the proposal function?
- Compare vs random et quasirandom sequences is it better?
- My intuition tolds me that the algorithm is a gradual exploration of the distribution
    - What about Bimodal vs unimodal distributions ? 
    - What about a "potentiel well" ? Tunelling like in quantic stuff?
    - How does it link with a random walk?
        -> is it a random walk ?
        -> Can we interpret a normal random walk as a a application og the algo?
- Does it works on distribution of infinite mass?
- Link with Metropolis Light Transportation?
