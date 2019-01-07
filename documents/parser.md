* parser
* using flex and bison


* 目的：
    1. 支持create database [dbname]
    2. 支持drop database [dbname]
    3. 支持use [dbname]
    4. 支持show databases
    5. 创建表 CREATE TABLE tableName(attrName1 Type1, attrName2 Type2,…, attrNameN TypeN NOT NULL, PRIMARY KEY(attrName1))，注意要实现“NOT NULL”和“PRIMARY KEY”这两个关键字
    6. DROP TABLE tableName
    7. SHOW TABLE tableName
    8. INSERT INTO  [tableName(attrName1, attrName2,…, attrNameN)] VALUES (attrValue1, attrValue2,…, attrValueN)
    9. DELETE FROM  tableName  WHERE  whereClauses
    10. UPDATE  tableName  SET  tableName.attrName = attExpression
    11. SELECT  tableName. AttrName  FROM  tableName  WHERE  whereClauses

* |            |           |         |        |       |         |
    | ---------- | --------- | ------- | ------ | ----- | ------- |
    | DATABASE   | DATABASES | TABLE   | TABLES | SHOW  | CREATE  |
    | DROP       | USE       | PRIMARY | KEY    | NOT   | NULL    |
    | INSERT     | INTO      | VALUES  | DELETE | FROM  | WHERE   |
    | UPDATE     | SET       | SELECT  | IS     | INT   | VARCHAR |
    | DESC       | INDEX     | AND     | DATE   | FLOAT | FOREIGN |
    | REFERENCES |           |         |        |       |         |

* IDENTIFIER: [A-Za-z][_0-9A-Za-z]*

   VALUE_INT: [0-9]+

   VALUE_STRING: ’[^’]*’

* parser.y -> y.tab.o 
* scan.l -> lex.yy.o
* 链接这两个对象，引入parser.h
* 调用runParser()