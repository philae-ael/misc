.PHONY: all run


all: out/parser

run: out/parser
	out/parser

out/parser:
	mkdir out
	g++ parser.cpp -lreadline -o out/parser

