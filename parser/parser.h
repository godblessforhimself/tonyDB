#ifndef PARSERHEADER
#define PARSERHEADER
#include <stdio.h>
#include "../const.h"
#include "../globalHolder.h"
using namespace constSpace;
union yytypeUnion{
    int ival;
    float fval;
    char* sval;
    parser_node* pnode;
    CompOp cval;
};
#define YYSTYPE yytypeUnion
void runParser();
// 全局缓冲区
void *alloc_len(int len);
parser_node *allocNode();
AttrType *allocAttrType(AttrType type);
char *mk_string(char *src, int len);
int get_id(char *s);
void reset_ptr();
void reset_ptr(char *&str);
// 缓冲区结束
#endif
