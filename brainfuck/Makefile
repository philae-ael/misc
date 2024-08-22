CC=gcc
CXX=g++
CPPFLAGS = -Wall -O3 -ggdb -march=native -std=c++2b
CFLAGS = $(CPPFLAGS)
LDFLAGS := 

.SUFFIXES:

OBJS := main.o 
OUTPUT := bf

.PHONY: clean run 

$(OUTPUT): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) -c $< $(CPPFLAGS) -o $@

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@


clean:
	rm -f *.o
	rm $(OUTPUT)

run: $(OUTPUT)
	./$(OUTPUT)
