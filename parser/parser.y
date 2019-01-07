%{
    #include "parser.h"
    using namespace std;
    extern "C" {
        void yyerror(char *);
        int yyparse(void);
        int yywrap(void);
        extern int yylex(void);
    }
    int exit_flag = 0;
    int attrcount = 0, indexcount = 0, insertcount = 0;
    RelationEntry tbentry;
    AttributeEntry *attrentry = NULL;
    RecordHandle *fileHandle = NULL;
    IX_IndexHandle *indexHandles = NULL;
#ifndef yyrestart
    void yyrestart(FILE*);
#endif
%}
%token <ival> TOKEN_EOF
        TOKEN_CREATE
        TOKEN_DATABASE
        TOKEN_DATABASES
        TOKEN_DROP
        TOKEN_USE
        TOKEN_SHOW
        TOKEN_TABLE
        TOKEN_TABLES
        TOKEN_NOT
        TOKEN_NULL
        TOKEN_KEY
        TOKEN_PRIMARY
        TOKEN_INTO
        TOKEN_INSERT
        TOKEN_VALUES
        TOKEN_DELETE
        TOKEN_FROM
        TOKEN_WHERE
        TOKEN_UPDATE
        TOKEN_SET
        TOKEN_SELECT
        TOKEN_IS
        TOKEN_INT
        TOKEN_VARCHAR
        TOKEN_DESC
        TOKEN_INDEX
        TOKEN_AND
        TOKEN_DATE
        TOKEN_FLOAT
        TOKEN_FOREIGN
        TOKEN_REFERENCES
        OP_LT
        OP_LE
        OP_GT
        OP_GE
        OP_EQ
        OP_NE
        TOKEN_EXIT

%token <sval> TOKEN_STRING_IDENTIFIER SYS_COMMAND VALUE_STRING
%token <ival> VALUE_INT
%token <fval> VALUE_FLOAT
%type <sval> tbName dbName colName insert
%type <pnode> type field fieldList columnList value valueList tableList whereClause setClause selector condition col expr colList
%type <cval>  op
%%
program: stmt
        {
            reset_ptr();
            YYACCEPT;
        }
    |   SYS_COMMAND
        {
            // !执行系统命令
            reset_ptr();
            system($1);
            YYACCEPT;
        }
    |   TOKEN_EXIT
        {
            exit_flag = 1;
            YYACCEPT;
        }
    |   TOKEN_EOF
        {
            //exit_flag = 1;
            YYACCEPT;
        }
    ;
stmt:   sysStmt ';'
    |   dbStmt ';'
    |   tbStmt ';'
    |   idxStmt ';'
    ;
sysStmt: TOKEN_SHOW TOKEN_DATABASES
        {
            // show databases
            printf("未实现show databases\n");
        }
    ;
dbStmt: TOKEN_CREATE TOKEN_DATABASE dbName
        {
            // 创建数据库 create database dbname
            if (gl_systemManager->createDb($3) != 0) {
                printf("create database %s failed!\n", $3);
            } else {
                printf("create database %s success!\n", $3);
                gl_bufPageManager->close();
            }
        }
    |   TOKEN_DROP TOKEN_DATABASE dbName
        {
            // 删除数据库 drop database dbname
            if (gl_systemManager->dropDb($3) != 0) {
                printf("drop database %s failed!\n", $3);
            } else {
                printf("drop database %s success!\n", $3);
            }
        }
    |   TOKEN_USE dbName
        {
            // 切换数据库 use dbname
            if (gl_systemManager->openDb($2) != 0) {
                printf("use database %s failed!\n", $2);
            } else {
                printf("use database %s success!\n", $2);
            }
        }
    |   TOKEN_SHOW TOKEN_TABLES
        {
            // 显示当前数据库的数据表 show tables;
            if (gl_systemManager->showTables() != 0) {
                printf("show tables error!\n");
            }
        }
    ;
tbStmt: TOKEN_DROP TOKEN_TABLE tbName
        {
            // 删除当前数据库的表 drop table tbName
            if (gl_systemManager->dropTable($3) != 0) {
                printf("drop table error!\n");
            }
        }
    |   TOKEN_CREATE TOKEN_TABLE tbName '(' fieldList ')'
        {
            // 在当前数据库创建表 create Table tbName fieldList
            // $5->print(cout);
            if (gl_systemManager->createTable($3, $5) != 0) {
                printf("create table error!\n");
            } else {
                cout << "创建表" << $3 << "成功\n";
            }
        }
    |   TOKEN_DESC tbName
        {
            // 显示表的详细信息 desc tbName
            if (gl_systemManager->printTable($2, cout) != 0) {
                printf("print %s failed!\n", $2);
            }
        }
    |   insert
        {
            // 释放
            if (attrentry != NULL) 
                delete[] attrentry;
            if (fileHandle != NULL)
                delete fileHandle;
            if (indexHandles != NULL)
                delete[] indexHandles;
            cout << "插入" << insertcount << "条记录\n";
            cout << "总共用时" << tickTock.tock() * 1000 << "ms\n";
        }
    |   TOKEN_DELETE TOKEN_FROM tbName TOKEN_WHERE whereClause
        {
            // 删除表中数据 delete from tbName where []
            if (gl_qlManager->Delete($3, $5) != 0) {
                cout << "delete error!\n";
                // todo: 错误信息
            }
        }
    |   TOKEN_UPDATE tbName TOKEN_SET setClause TOKEN_WHERE whereClause
        {
            // 更新表中数据 update tbName set [] where []
            if (gl_qlManager->Update($2, $4, $6) != 0) {
                cout << "update error!\n";
                // todo: 错误信息
            }
            cout << "更新完成\n";
        }
    |   TOKEN_SELECT selector TOKEN_FROM tableList TOKEN_WHERE whereClause
        {
            // 选择表中数据 select [] from [] where []
            tickTock.tick();
            if (gl_qlManager->Select($2, $4, $6) != 0) {
                cout << "select error!\n";
                // todo: 错误信息
            }
            double second = tickTock.tock();
            cout << "select使用" << second * 1000 << "ms\n";
        }
    ;
insert: TOKEN_INSERT TOKEN_INTO tbName TOKEN_VALUES '(' valueList ')'
        {
            insertcount=0;
            // 创建
            RecordScan tbscan, attrscan; Record record;
            int openScan(RecordHandle &recordHandle, constSpace::AttrType attrType, int attrLength, int attrOffset, constSpace::CompOp compOp, void *value);
            if (tbscan.openScan(gl_systemManager->relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, $3) != 0) {
                cout << "打开关系表" << $3 << "失败\n";
                return -1;
            }
            if (tbscan.getNextRec(record) != 0) {
                cout << "打开关系表" << $3 << "失败\n";
                return -1;
            }
            tbentry = *(RelationEntry*)record.getData();
            attrcount = tbentry.attrCount;
            attrentry = new AttributeEntry[attrcount];
            if (attrscan.openScan(gl_systemManager->attrHandle, STRING, MAX_ATTRNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, $3) != 0) {
                cout << "打开属性表失败\n";
                return -1; 
            }
            indexcount = 0;
            int primaryKC = 0; // 主键数目
            for (int i = 0; i < attrcount; ++i) {
                if (attrscan.getNextRec(record) != 0) {
                    cout << "缺少属性\n";
                    return -1;
                }
                attrentry[i] = *(AttributeEntry*)record.getData();
                if (attrentry[i].indexNo >= 0) {
                    indexcount++;
                }
                if (attrentry[i].isPrimarykey) {
                    primaryKC++;
                    if (attrentry[i].attrType != AttrType::INT)
                        primaryKC++;
                }
            }
            if (primaryKC == 1 && usePKBufferOptimize) {   // 设置全局优化标志
                usePKOptimize = true;
            } else {
                usePKOptimize = false;
            }
            int BMsize = (attrcount >> 3) + 1;
            int entrysize = tbentry.tupleLength;
            fileHandle = new RecordHandle();
            if (gl_recordManager->openFile($3, *fileHandle) != 0) {
                if (gl_recordManager->createFile($3, BMsize + entrysize) == false) {
                    printf("create %s failed!\n", $3);
                    return -1;
                }
                if (gl_recordManager->openFile($3, *fileHandle) != 0) {
                    printf("open %s failed.\n", $3);
                    return -1;
                }
                cout << "创建文件" << $3 << "成功\n";
            }
            indexHandles = new IX_IndexHandle[indexcount];
            int nowposofhandle = 0;
            for (int i = 0; i < attrcount; ++i) {
                if (attrentry[i].indexNo >= 0) {
                    if (gl_indexingManager->OpenIndex($3, attrentry[i].indexNo, indexHandles[nowposofhandle]) != 0) {
                        cout << "打开索引" << attrentry[i].indexNo << "失败\n";
                        return -1;
                    }
                }
            }
            // 准备好了
            $$ = $3;
            if (usePKOptimize) {
                PKBuffer.resetPKBuffer();
            }
            if (gl_qlManager->Insert($3, $6, &tbentry, attrentry, attrcount, *fileHandle, indexHandles, indexcount) != 0) {
                cout << "插入表" << $3 << "失败\n";
                // return -1;
            }
            insertcount=1;
            tickTock.tick();
        }
    |   insert ',' '(' valueList ')'
        {   
            // 清空内存
            $$ = $1;
            if (gl_qlManager->Insert($1, $4, &tbentry, attrentry, attrcount, *fileHandle, indexHandles, indexcount) != 0) {
                $4->printValueList(cout);
                cout << "插入表" << $1 << "失败\n";
                // return -1;
            }
            insertcount++;
            if (insertcount % 10000 == 0) {
                double second = tickTock.tock();
                cout << "插入" << insertcount << "条,花去时间" << second * 1000 << "ms\n";
            }
            reset_ptr($1); // 保存tbName 清空其他的
        }
    ;
idxStmt: TOKEN_CREATE TOKEN_INDEX tbName '(' colName ')'
        {
            // 当前数据库的tbName表的colName建立索引 create index tbName (colName)
            if (gl_systemManager->createIndex($3, $5) != 0) {
                cout << "创建索引" << $3 << "." << $5 << "失败\n";
            }
        }
    |   TOKEN_DROP TOKEN_INDEX tbName '(' colName ')'
        {
            // 删除索引
            if (gl_systemManager->dropIndex($3, $5) != 0) {
                cout << "删除索引" << $3 << "." << $5 << "失败\n";
            }
        }
    ;
fieldList: field
        {
            parser_node *node = allocNode();
            node->init_list($1);
            $$ = node;
        }
    |   fieldList ',' field
        {
            parser_node *node = allocNode();
            $1->append_list($3, node);
            $$ = $1;
        }
    ;
field:  colName type
        {   
            // 普通域
            parser_node *node = allocNode();
            node->set_field_normal($1, $2);
            $$ = node;
        }
    |   colName type TOKEN_NOT TOKEN_NULL
        {   
            // 非空域
            parser_node *node = allocNode();
            node->set_field_notnull($1, $2);
            $$ = node;
        }
    |   TOKEN_PRIMARY TOKEN_KEY '(' columnList ')'
        {
            // 主键列表
            $$ = $4;
        }
    |   TOKEN_FOREIGN TOKEN_KEY '(' colName ')' TOKEN_REFERENCES tbName '(' colName ')' 
        {
            // 外键 
            parser_node *node = allocNode();
            node->set_field_foreign_key($4, $7, $9);
            $$ = node;
        }
    ;
selector: 
        '*' 
        {
            parser_node *node = allocNode();
            node->set_selector(NULL);
            $$ = node;
        }
    |   colList
        {
            parser_node *node = allocNode();
            node->set_selector($1);
            $$ = node;
        }
    ;
colList:col
        {
            parser_node *node = allocNode();
            node->init_list($1);
            $$ = node;
        }
    |   colList ',' col
        {
            parser_node *node = allocNode();
            $1->append_list($3, node);
            $$ = $1;
        }
    ;
tableList:  
        tbName
        {
            parser_node *node = allocNode();
            node->init_string_list($1);
            $$ = node;
        }
    |   tableList ',' tbName
        {
            parser_node *node = allocNode();
            $1->append_string_list($3, node);
            $$ = $1;
        }
    ;
columnList: 
        colName
        {
            parser_node *node = allocNode();
            node->init_string_list($1);
            $$ = node;
        }
    |   columnList ',' colName
        {
            parser_node *node = allocNode();
            $1->append_string_list($3, node);
            $$ = $1;
        }
    ;
type: 
        TOKEN_INT '(' VALUE_INT ')'
        {
            parser_node *node = allocNode();
            node->set_attr_type(AttrType::INT, $3);
            $$ = node;
        }
    |   TOKEN_VARCHAR '(' VALUE_INT ')'
        {
            parser_node *node = allocNode();
            node->set_attr_type(STRING, $3);
            $$ = node;
        }
    |   TOKEN_DATE
        {
            parser_node *node = allocNode();
            node->set_attr_type(STRING, 10);
            $$ = node;
        }
    |   TOKEN_FLOAT
        {
            parser_node *node = allocNode();
            node->set_attr_type(FLOAT, 4);
            $$ = node;
        }
    ;
value:  VALUE_INT
        {
            parser_node *node = allocNode();
            node->set_value($1);
            $$ = node;
        }
    |   VALUE_STRING
        {
            parser_node *node = allocNode();
            node->set_value($1);
            $$ = node;
        }
    |   VALUE_FLOAT
        {
            parser_node *node = allocNode();
            node->set_value($1);
            $$ = node;
        }
    |   TOKEN_NULL
        {
            parser_node *node = allocNode();
            node->set_value();
            $$ = node;
        }
    ;
valueList: 
        value
        {
            parser_node *node = allocNode();
            node->init_value_list($1);
            $$ = node;
        }
    |   valueList ',' value
        {
            parser_node *node = allocNode();
            $1->append_value_list($3, node);
            $$ = $1;
        }
    ;

setClause: 
        colName OP_EQ value
        {
            parser_node *single = allocNode(), *list = allocNode();
            single->single_set($1, $3);
            list->init_list(single);
            $$ = list;
        }
    |   setClause ',' colName OP_EQ value
        {
            parser_node *single = allocNode(), *list = allocNode();
            single->single_set($3, $5);
            $1->append_list(single, list);
            $$ = $1;
        }
    ;
whereClause:    
        condition
        {
            parser_node *node = allocNode();
            node->init_list($1);
            $$ = node;
        }
    |   whereClause TOKEN_AND condition
        {
            parser_node *node = allocNode();
            $1->append_list($3, node);
            $$ = $1;
        }
    ;
condition:  
        col op expr
        {
            parser_node *node = allocNode();
            node->set_normal_condition($1, $2, $3);
            $$ = node;
        }
    |   col TOKEN_IS TOKEN_NULL
        {
            parser_node *node = allocNode();
            node->set_null_cond($1);
            $$ = node;
        }
    |   col TOKEN_IS TOKEN_NOT TOKEN_NULL
        {
            parser_node *node = allocNode();
            node->set_notnull_cond($1);
            $$ = node;
        }
    ;
col:    colName
        {
            parser_node *node = allocNode();
            node->set_col(NULL, $1);
            $$ = node;
        }
    |   tbName '.' colName
        {
            parser_node *node = allocNode();
            node->set_col($1, $3);
            $$ = node;
        }
    ;
expr:   col
        {
            parser_node *node = allocNode();
            node->col_or_value($1, NULL);
            $$ = node;
        }
    |   value
        {
            parser_node *node = allocNode();
            node->col_or_value(NULL, $1);
            $$ = node;
        }
    ;
op:     OP_LT
        {
            $$ = LT_OP;
        }
   |    OP_LE
        {
            $$ = LE_OP;
        }
   |    OP_GT
        {
            $$ = GT_OP;
        }
   |    OP_GE
        {
            $$ = GE_OP;
        }
   |    OP_EQ
        {
            $$ = EQ_OP;
        }
   |    OP_NE
        {
            $$ = NE_OP;
        }
   ;
dbName: TOKEN_STRING_IDENTIFIER
    ;
tbName: TOKEN_STRING_IDENTIFIER
    ;
colName: TOKEN_STRING_IDENTIFIER
    ;
%%

void yyerror(char *s) {
   // cout << "错误:" << s << endl;
}
int yywrap(void) {
   return 0;
}
void runParser() {
    yydebug = 0;
    while (1) {
        cout << "> " << std::flush;
        yyparse();
        if (exit_flag) 
            break;
        //yyrestart(stdin);
    }
    cout << "\n";
    return;
}