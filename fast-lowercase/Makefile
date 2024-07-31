CPPFLAGS = -Wall -O3 -ggdb -march=native
CFLAGS = $(CPPFLAGS)

DEPDIR = .d
DEPFLAGS = -MT "$@ $(DEPDIR)/$*.d" -MMD -MP -MF $(DEPDIR)/$*.d

.SUFFIXES:

OBJS := lowercase.o shakespeare.txt.o

.PHONY: clean clobber run

lowercase: $(OBJS)
	g++ $(LDFLAGS) -o $@ $^

%.o: %.cpp
	g++ -c $< $(CPPFLAGS) -o $@

%.o: %.c
	gcc -c $< $(CFLAGS) -o $@


clean:
	rm -f *.o

shakespeare.txt.c: shakespeare.txt
	bin2c shakespeare.txt --padd 0  --name data > shakespeare.txt.c

run: lowercase 
	./lowercase
