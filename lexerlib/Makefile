all: parser

parser: lex.yy.o main.o
	g++ $^ -o $@

main.o: main.cpp
	g++ -c $<

lex.yy.o: lex.yy.c
	gcc -c $<

lex.yy.c: setupini.l lex.export.h
	flex --header-file=lex.yy.h $<

clean:
	rm -f *.o lex.yy* *.a *.exe