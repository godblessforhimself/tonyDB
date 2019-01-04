bison -y -d parser.y --debug --verbose; #mv y.tab.c parser_autogen.cpp;
flex scan.l; #mv lex.yy.c scan_autogen.cpp
gcc -c lex.yy.c -o lex.yy.o
g++ -c y.tab.c -o parser.o
ar cqs parserlib.o lex.yy.o parser.o

