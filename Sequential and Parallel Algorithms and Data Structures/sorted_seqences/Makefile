CC=clang 
CXX=clang++
CPPFLAGS = -Wall -O0 -ggdb -march=native -std=c++23
CFLAGS = $(CPPFLAGS)
LDFLAGS := 

.SUFFIXES:

OBJS := test.o 
OUTPUT := test

.PHONY: clean run 

$(OUTPUT): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

%.o: %.cpp ./bst.h
	$(CXX) -c $< $(CPPFLAGS) -o $@

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@


clean:
	rm -f *.o
	rm $(OUTPUT)

run: $(OUTPUT)
	./$(OUTPUT)
