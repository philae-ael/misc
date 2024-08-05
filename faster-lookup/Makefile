CC=clang 
CXX=clang++
CPPFLAGS = -Wall -O3 -ggdb -march=native -std=c++23
CFLAGS = $(CPPFLAGS)
LDFLAGS := -lprimesieve

.SUFFIXES:

OBJS := main.o 
OUTPUT := search

.PHONY: clean clobber run graph

$(OUTPUT): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp ../bench.h
	$(CXX) -c $< $(CPPFLAGS) -o $@

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@


clean:
	rm -f *.o
	rm $(OUTPUT)

run: $(OUTPUT)
	./$(OUTPUT)

graph: run 
	python ./display_res.py
