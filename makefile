CC=gcc
CFLAGS=-Wall
LDFLAGS= -lfl -Wall -ly
SRC=src/
OBJDIR = obj/

all: bin/tpcc

bin/tpcc: $(OBJDIR)main.o $(OBJDIR)tree.o $(OBJDIR)lex.yy.o $(OBJDIR)parser.tab.o $(OBJDIR)table.o $(OBJDIR)sem.o $(OBJDIR)trad.o
	$(CC) -o $@ $^ $(LDFLAGS)

obj/main.o: $(SRC)main.c $(SRC)tree.h $(OBJDIR)parser.tab.h $(SRC)table.h
	$(CC) -c $< -o $@ $(CFLAGS)

obj/lex.yy.o: $(OBJDIR)lex.yy.c $(SRC)tree.h $(OBJDIR)parser.tab.h
	$(CC) -I$(OBJDIR) -c $< -o $@ $(CFLAGS)

obj/lex.yy.c: $(SRC)lexer.lex
	flex $(SRC)lexer.lex
	mv lex.yy.c obj/

obj/parser.tab.o: $(OBJDIR)parser.tab.c $(OBJDIR)parser.tab.h $(SRC)tree.h
	$(CC) -o $@ -c $< $(CFLAGS)

obj/parser.tab.h obj/parser.tab.c: $(SRC)parser.y
	bison -d $(SRC)parser.y
	mv parser.tab.c parser.tab.h obj/

obj/tree.o: $(SRC)tree.c $(SRC)tree.h
	$(CC) -c $< -o $@ $(CFLAGS)

obj/table.o: $(SRC)table.c $(SRC)table.h $(SRC)tree.h
	$(CC) -c $< -o $@ $(CFLAGS)

obj/trad.o: $(SRC)trad.c $(SRC)trad.h $(SRC)tree.h $(SRC)table.h 
	$(CC) -c $< -o $@ $(CFLAGS)

obj/sem.o: $(SRC)sem.c $(SRC)sem.h $(SRC)tree.h $(SRC)table.h 
	$(CC) -c $< -o $@ $(CFLAGS)

bin/_anonymous: $(OBJDIR)_anonymous.o
	gcc -o $@ $^ -nostartfiles -no-pie

obj/_anonymous.o: _anonymous.asm
	nasm -f elf64 -o $@ $<

clean:
	rm -f obj/lex.yy.*
	rm -f obj/parser.tab.*
	rm -f obj/*.o
	rm -f bin/*
