bison -y -d parser.y
flex parser.l
gcc -c y.tab.c lex.yy.c
gcc y.tab.o lex.yy.o -o excutable

# parser.y -(bison)> y.tab.h&.c
# *.l -flex> lex.yy.c
