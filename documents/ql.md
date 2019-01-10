### 查询解析模块

#### 该部分由ql和parser组成

#### ql

* 主要接口

* ```c++
  class QL_Manager {
  public:
      // 插入数据
      int Insert(const char *tbName, parser_node *, RelationEntry *, AttributeEntry *attr, int attrcount, RecordHandle &fileHandle, IX_IndexHandle *indexHandle, int indexcount);
      // 查询数据
      int Select(parser_node *selector, parser_node *tables, parser_node *whereClause);
      // 删除数据
      int Delete(char *tbName, parser_node *whereClause);
      // 更新数据
      int Update(char *tbName, parser_node *setClause, parser_node *whereClause);
  };
  ```

* 插入：检查对应属性类型是否正确，主键是否重复、外键是否存在；若索引存在，则插入对应的索引。

* 查询：

  * 单表查询：是否优化都很快，最多需要遍历一次单个表。若查询条件中有具有主键，可以使用其索引进行查询。
  * 多表连接：只对两表连接进行了优化。3个及以上表通过多层循环实现，数据量大时不现实。两表连接时，使用索引，可以优化到o(nlogm)甚至o(min(n,m))(两个索引且其中一个属性为唯一的主键)。

* 删除数据：删除可能影响到其他表的外键、存在索引的属性。

* 更新数据：检查外键、主键、索引。

#### parser


* parser：using flex and bison


* 功能：
    1. 支持create database [dbname] 
    2. 支持drop database [dbname]
    3. 支持use [dbname]
    4. 不支持show databases
    5. 创建表 CREATE TABLE tableName(attrName1 Type1, attrName2 Type2,…, attrNameN TypeN NOT NULL, PRIMARY KEY(attrName1))，实现NOT NULL和PRIMARY KEY和FOREIGN KEY这两个关键字
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

* ```
	IDENTIFIER: [A-Za-z][_0-9A-Za-z]*
   
  VALUE_INT: [0-9]+
   
  VALUE_STRING: '([^']|[\\\'])*' 
  ```

* parser.y -> y.tab.o 

* scan.l -> lex.yy.o

* 链接这两个对象，引入parser.h

* 调用runParser()