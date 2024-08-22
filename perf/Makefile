CPPFLAGS = -Wall -O3 -ggdb -march=native
CFLAGS = $(CPPFLAGS)
ASMFLAGS = 

DEPDIR = .d

.SUFFIXES:

OBJS := main.o

.PHONY: clean run

bench: $(OBJS)
	g++ $(LDFLAGS) -o $@ $^

%.o: %.cpp
	g++ -c $< $(CPPFLAGS) -o $@

%.o: %.c
	gcc -c $< $(CFLAGS) -o $@

%.o: %.c
	gcc -c $< $(CFLAGS) -o $@

%.o: %.S
	gcc -c $< $(ASMFLAGS) -o $@

clean:
	rm -f *.o

run: bench 
	./bench
