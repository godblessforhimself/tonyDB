BUILD_DIR      = build/
LIB_DIR        = lib/
AR             = ar -rc
YACC           = bison -dy
LEX            = flex
noting =-m32 -g -O3 -Wall
CFLAGS         = -std=c++11
RANLIB         = ranlib
CONST_SOURCES  = const.cpp
FS_SOURCES     = filesystem/utils/MyBitMap.cpp
RM_SOURCES     = recordmanager/rm.cpp
IX_SOURCES     = indexing/indexing.cpp
SM_SOURCES     = systemmanager/sm.cpp
QL_SOURCES     = qlmanager/ql.cpp
PARSER_SOURCES = parser/parser.y parser/scan.l
ENTRY_SOURCE = entry.cpp

all: clean const fs rm indexing sm parser ql $(ENTRY_SOURCE)
	g++ $(CFLAGS) $(ENTRY_SOURCE) -o entry build/ql.o build/const.o build/fs.o build/rm.o build/ix.o build/sm.o build/parser.o build/lex.o build/parser_h.o

ql: $(QL_SOURCES)
	g++ $(CFLAGS) -c $(QL_SOURCES) -o build/ql.o

const: $(CONST_SOURCES)
	g++ $(CFLAGS) -c $(CONST_SOURCES) -o build/const.o

fs: $(FS_SOURCES)
	g++ $(CFLAGS) -c $(FS_SOURCES) -o build/fs.o

rm: $(RM_SOURCES)
	g++ $(CFLAGS) -c $(RM_SOURCES) -o build/rm.o

indexing: $(IX_SOURCES)
	g++ $(CFLAGS) -c $(IX_SOURCES) -o build/ix.o

sm: $(SM_SOURCES)
	g++ $(CFLAGS) -c $(SM_SOURCES) -o build/sm.o

parser: $(PARSER_SOURCES)
	bison -dy parser/parser.y -o parser/y.tab.c --debug --verbose
	flex -o parser/lex.yy.c parser/scan.l
	g++ -c parser/lex.yy.c -o build/lex.o
	g++ $(CFLAGS) -c parser/y.tab.c -o build/parser.o
	g++ $(CFLAGS) -c parser/parser_helper.cpp -o build/parser_h.o
	#rm parser/y.tab.h parser/y.tab.c parser/lex.yy.c

clean: 
	echo "clean"
	rm -R build/
	mkdir build
	touch parser/*
	touch indexing/*
	touch qlmanager/*
	touch recordmanager/*
	touch systemmanager/*


