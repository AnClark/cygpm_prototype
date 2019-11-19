all: parserc++

parserc++: lex.cc main.cc Scanner.h Scanner.ih Scannerbase.h tokens.h
	g++ lex.cc main.cc -o parserc++

lex.cc: lexer
	flexc++ lexer

clean:
	rm -f lex.cc
	rm -f Scanner.h Scannerbase.h
	rm -f *.exe