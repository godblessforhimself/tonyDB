%{
    #include "parser.h"
    extern "C" {
        void yyerror(char *);
        int yyparse(void);
        int yywrap(void);
        extern int yylex(void);
    }
    int exit_flag = 0;
#ifndef yyrestart
    void yyrestart(FILE*);
#endif
%}
%token <ival> TOKEN_CREATE
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
%type <sval> tbName dbName colName
%type <pnode> type field fieldList columnList value valueList valueLists tableList whereClause setClause selector condition col expr
%type <cval> op
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
            }
        }
    |   TOKEN_DESC tbName
        {
            // 显示表的详细信息 desc tbName
            if (gl_systemManager->printTable($2, cout) != 0) {
                printf("print %s failed!\n", $2);
            }
        }
    |   TOKEN_INSERT TOKEN_INTO tbName TOKEN_VALUES valueLists
        {
            // 向表中插入数据 insert into tbName values valueLists
            cout << "start insert!\n";
            parser_node *current = $5;
            while (current != NULL) {
                parser_node *&value = current->u.ValueLists.value;
                if (gl_qlManager->Insert($3, value) != 0) {
                    cout << "insert into " << $3 << " ";
                    value->printValueList(cout);
                    cout << " failed!\n";
                } else {
                    cout << "insert success!\n";
                }
                current = current->u.ValueLists.next;
            }
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
        }
    |   TOKEN_SELECT selector TOKEN_FROM tableList TOKEN_WHERE whereClause
        {
            // 选择表中数据 select [] from [] where []
            if (gl_qlManager->Select($2, $4, $6) != 0) {
                cout << "select error!\n";
                // todo: 错误信息
            }
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
    |   columnList
        {
            parser_node *node = allocNode();
            node->set_selector($1);
            $$ = node;
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
            node->set_attr_type(DATE, 0);
            $$ = node;
        }
    |   TOKEN_FLOAT
        {
            parser_node *node = allocNode();
            node->set_attr_type(FLOAT, 0);
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

valueLists: 
        '(' valueList ')'
        {
            parser_node *node = allocNode();
            node->init_value_lists($2);
            $$ = node;
        }
    |   valueLists ',' '(' valueList ')'
        {
            parser_node *node = allocNode();
            $1->append_value_list($4, node);
            $$ = $1;
        }
    ;
setClause: 
        colName '=' value
        {
            parser_node *single = allocNode(), *list = allocNode();
            single->single_set($1, $3);
            list->init_list(single);
            $$ = list;
        }
    |   setClause ',' colName '=' value
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
    cout << s << endl;
}
int yywrap(void) {
   return 1;
}
void runParser() {
    yydebug = 0;
    while (1) {
        cout << "> " << std::flush;
        yyparse();
        if (exit_flag) 
            break;
        yyrestart(stdin);
    }
    return;
}