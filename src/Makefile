OBJECTS := \
	lex.yy.o \
	sha512.o \
	utils.o \
	database.o \
	db_query.o \
	main.o

all: main

main: $(OBJECTS)
	g++ $^ -o $@ -static -lsqlite3 

main.o: main.cpp
	g++ -c $<

db_query.o: db_query.cpp database.h
	g++ -c $<

database.o: database.cpp database.h lex.export.h
	g++ -c $<

utils.o: utils.cpp utils.h stdafx.hpp.gch
	g++ -c $<

lex.yy.o: lex.yy.c tokens.h
	gcc -c $<

lex.yy.c: setupini.l
	flex $<

stdafx.hpp.gch: stdafx.hpp
	g++ $<

include Makefile.sha512

clean:
	rm -f *.exe* *.o
	rm -f *.db*
	rm -f lex.yy*
