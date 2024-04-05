.PHONY: all run


all: out/parser

run: out/parser
	out/parser

out/parser: parser.cpp
	-@mkdir out -p
	g++ parser.cpp -lreadline -o out/parser

