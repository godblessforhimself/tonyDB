#include "parser.h"
#include "y.tab.h"
#define MAX_IDENTIFIER_LENGTH (256)
#define CHAR_POOL_SIZE (4096)
#define match(x) (strncmp(string, (x), MAX_IDENTIFIER_LENGTH) == 0)
char scanPool[CHAR_POOL_SIZE];
int scanPtr = 0;
void reset_ptr() {
    scanPtr = 0;
}
void reset_ptr(char *&str) {
    //
    int len = strlen(str);
    strncpy(scanPool, str, len + 1);
    scanPool[len] = 0;
    str = scanPool;
    scanPtr = len + 1;
}
void *alloc_len(int len) {
    char *s;
    if(scanPtr + len > CHAR_POOL_SIZE){
        printf("out of memory\n");
        exit(1);
    }
    s = scanPool + scanPtr;
    scanPtr += len;
    return s;
}
parser_node *allocNode() {
    return (parser_node*)alloc_len(sizeof(parser_node));
}
AttrType *allocAttrType(AttrType type) {
    AttrType *dst = (AttrType*)alloc_len(sizeof(AttrType));
    *dst = type;
    return dst;
}
char *mk_string(char *src, int len) {
    char *dst;
    dst = (char*)alloc_len(len + 1);
    strncpy(dst, src, len + 1);
    dst[len] = 0;
    return dst;
}
int get_id(char *s) {
    int i = 0;
    char string[MAX_IDENTIFIER_LENGTH];
    memset(string, 0, sizeof(string));
    while (s[i] != '\0' && i < MAX_IDENTIFIER_LENGTH) {
        string[i] = tolower(s[i]);
        i++;
    }
    s[i] = 0;
    if (match("create")) {
        yylval.ival = TOKEN_CREATE;
        return TOKEN_CREATE;
    } else if (match("database")) {
        return yylval.ival = TOKEN_DATABASE;
    } else if (match("databases")) {
        return yylval.ival = TOKEN_DATABASES;
    } else if (match("drop")) {
        return yylval.ival = TOKEN_DROP;
    } else if (match("use")) {
        return yylval.ival = TOKEN_USE;
    } else if (match("show")) {
        return yylval.ival = TOKEN_SHOW;
    } else if (match("table")) {
        return yylval.ival = TOKEN_TABLE;
    } else if (match("tables")) {
        return yylval.ival = TOKEN_TABLES;
    } else if (match("not")) {
        return yylval.ival = TOKEN_NOT;
    } else if (match("null")) {
        return yylval.ival = TOKEN_NULL;
    } else if (match("primary")) {
        return yylval.ival = TOKEN_PRIMARY;
    } else if (match("key")) {
        return yylval.ival = TOKEN_KEY;
    } else if (match("insert")) {
        return yylval.ival = TOKEN_INSERT;
    } else if (match("into")) {
        return yylval.ival = TOKEN_INTO;
    } else if (match("values")) {
        return yylval.ival = TOKEN_VALUES;
    } else if (match("delete")) {
        return yylval.ival = TOKEN_DELETE;
    } else if (match("from")) {
        return yylval.ival = TOKEN_FROM;
    } else if (match("where")) {
        return yylval.ival = TOKEN_WHERE;
    } else if (match("update")) {
        return yylval.ival = TOKEN_UPDATE;
    } else if (match("set")) {
        return yylval.ival = TOKEN_SET;
    } else if (match("select")) {
        return yylval.ival = TOKEN_SELECT;
    } else if (match("is")) {
        return yylval.ival = TOKEN_IS;
    } else if (match("int")) {
        return yylval.ival = TOKEN_INT;
    } else if (match("char")) {
        return yylval.ival = TOKEN_VARCHAR;
    } else if (match("desc")) {
        return yylval.ival = TOKEN_DESC;
    } else if (match("index")) {
        return yylval.ival = TOKEN_INDEX;
    } else if (match("and")) {
        return yylval.ival = TOKEN_AND;
    } else if (match("date")) {
        return yylval.ival = TOKEN_DATE;
    } else if (match("float")) {
        return yylval.ival = TOKEN_FLOAT;
    } else if (match("foreign")) {
        return yylval.ival = TOKEN_FOREIGN;
    } else if (match("references")) {
        return yylval.ival = TOKEN_REFERENCES;
    } else if (match("exit")) {
        return yylval.ival = TOKEN_EXIT;
    } else {
        yylval.sval = mk_string(s, i);
        return TOKEN_STRING_IDENTIFIER;
    }
}