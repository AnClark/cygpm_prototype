out: parser

parser: lex.yy.c
	gcc -o $@ $<

parser++: lex.yy.cc
	g++ -o $@ $<

lex.yy.c: setupini.l
	flex $<

lex.yy.cc: setupini.l
	flex++ --header-file=lex.yy.hpp -+ $<

clean:
	rm *.exe
	rm lex.yy.*
