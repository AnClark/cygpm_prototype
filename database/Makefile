all: main

main: lex.yy.o database.o db_query.o main.o
	g++ $^ -o $@ -static -lsqlite3 

main.o: main.cpp
	g++ -c $<

db_query.o: db_query.cpp database.h
	g++ -c $<

database.o: database.cpp database.h lex.export.h
	g++ -c $<

lex.yy.o: lex.yy.c tokens.h
	gcc -c $<

lex.yy.c: setupini.l
	flex $<

clean:
	rm -f *.exe* *.o
	rm -f *.db*
	rm -f lex.yy*
