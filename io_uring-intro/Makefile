.PHONY: all run


all: out/io_uring_test

run: out/io_uring_test
	out/io_uring_test

out/io_uring_test: main.c
	-@mkdir out -p
	gcc -ggdb -std=gnu11 main.c -o $@ -lc

