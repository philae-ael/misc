# LRU
Implementation is my own thus may suffer from big inefficiencies, based on an addressable priority list based on a binary heap


Potential improvements: 
- The Hashmap (perf reports the hashmap as one of the bottleneck)
- Siftup / SiftDown 
- The fact that it's a binary heap: try fibonacci heaps ? there will be less need for a hacky way of getting addressability
- The time counter is susceptible to overflow which fuck up the datastructure: When an overflow will occurr, touch all entries timestamps

The execution time seems fine? execution happens in 0.5s-1.6s with benchmarck and so on (depending on some parameters)

See Sequential an Parallel Algorithm and Data Structures (Sander & al.) Chapter 6.1/6.2;
