.PHONY: all run


all: out/parser

run: out/parser
	out/parser

out/parser: parser.c
	-@mkdir out -p
	gcc -ggdb -std=c23 parser.c -lreadline -o out/parser

