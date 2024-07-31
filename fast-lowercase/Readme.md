# Trying to go kinda fast to make strings lowercase


Of course, heavily inspired by stuff like simdjson and so on.

```
lowercase_0 took: 0.4818 byte per cycle, took 22.32 ms for 5000 iterations
lowercase_smarter took: 0.5428 byte per cycle, 19.94msec for 5000 iterations
lowercase_branchless took: 0.5184 byte per cycle, 20.83msec for 5000 iterations
lowercase_table took: 1.5598 byte per cycle, 7.79msec for 5000 iterations
lowercase_simd took: 9.9243 byte per cycle, 2.34msec  for 5000 iterations
```


The python implementation of lowercase take 1.5ms per iteration, thus is a tad bit slower.
The python algorithm seems to be [the naive one](https://github.com/python/cpython/blob/f071f01b7b7e19d7d6b3a4b0ec62f820ecb14660/Objects/bytes_methods.c#L251)
