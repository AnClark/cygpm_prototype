out: parser

parser: lex.yy.c
	gcc -o $@ $<

lex.yy.c: setupini.l
	flex $<

clean:
	rm *.exe
	rm lex.yy.c lex.yy.cc
