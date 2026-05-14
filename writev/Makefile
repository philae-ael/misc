CC=clang 
CXX=clang++
CPPFLAGS = -Wall -O0 -ggdb -march=native -std=c++26
CFLAGS = $(CPPFLAGS)
LDFLAGS :=

.SUFFIXES:

OBJS := main.o 
OUTPUT := writev

.PHONY: clean run

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
