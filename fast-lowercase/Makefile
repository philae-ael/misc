CPPFLAGS = -Wall -O1 -ggdb -march=native
CFLAGS = $(CPPFLAGS)

DEPDIR = .d
DEPFLAGS = -MT "$@ $(DEPDIR)/$*.d" -MMD -MP -MF $(DEPDIR)/$*.d

.SUFFIXES:

OBJS := lowercase.o shakespeare.txt.o

.PHONY: clean run

lowercase: $(OBJS)
	g++ $(LDFLAGS) -o $@ $^

%.o: %.cpp
	g++ -c $< $(CPPFLAGS) -o $@

%.o: %.c
	g++ -c $< $(CFLAGS) -o $@


clean:
	rm -f *.o

shakespeare.txt.o: shakespeare.txt
	objcopy -I binary -O elf64-x86-64 \
		--rename-section .data=.rodata,alloc,load,readonly,contents \
		--redefine-sym _binary_shakespeare_txt_start=data \
		--redefine-sym _binary_shakespeare_txt_end=data_end \
		--redefine-sym _binary_shakespeare_txt_size=data_size \
		shakespeare.txt shakespeare.txt.o

run: lowercase 
	./lowercase
