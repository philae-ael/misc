lru: lru.cpp
	g++ -o lru -std=c++20 lru.cpp -O3 -DNDEBUG -ggdb

lru-addrsan: lru.cpp
	clang++ -o lru-addrsan -std=c++20 lru.cpp -O3 -DNDEBUG -ggdb -fsanitize=address

lru-undefsan: lru.cpp
	clang++ -o lru-undefsan -std=c++20 lru.cpp -O3 -DNDEBUG -ggdb -fsanitize=undefined

lru-memsan: lru.cpp
	clang++ -o lru-memsan -std=c++20 lru.cpp -O3 -DNDEBUG -ggdb -fsanitize=memory

.PHONY: run run-addrsan run-undefsan run-memsan run-valgrind perf perf-gecko

run: lru 
	./lru 

run-addrsan: lru-addrsan
	./lru-addrsan
run-undefsan: lru-undefsan
	./lru-undefsan
run-memsan: lru-memsan
	./lru-memsan
run-valgrind: lru
	valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt ./lru

perf: lru
	perf record -g -F 10000 ./lru

perf-gecko: perf
	perf script report gecko
