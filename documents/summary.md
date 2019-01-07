### 项目总结报告

##### ​																	金涛 2015011284

#### 系统架构设计

* 记录管理模块
  * 该模块是DBMS的文件系统，管理存储数据库记录以及元数据的文件。该模块依赖于我们预先给定的一个页式文件系统，在此基础上扩展而成。
* 索引模块
  * 为存储在文件中的记录建立B+树索引，加快查找速度。
* 系统管理模块
  * 实现基本的数据定义语言（DDL），对数据库和数据表进行管理。
* 查询解析模块
  * 实现基本的数据操作语言（DML），对数据库里的数据进行增删改查等基本操作。

#### 各模块详细设计

##### 记录管理模块
* 该模块是DBMS的文件系统，管理存储数据库记录以及元数据的文件。该模块依赖于我们预先给定的一个页式文件系统，在此基础上扩展而成。
* 记录：Record由RID和char*指针组成，RID由两个int组成，表示页号与页中的秩
* 文件：一个文件即一个数据库，由head页与content页组成。content页中每条数据的长度相同。
* head页：记录整个库的信息
* content页：头部固定长度存储当前页的一些信息和与其他页的链接，固定长度之后，存储数据。
* 位图：存在content页头部，每个字节表示八条数据的是否使用。
* 每页数据条数：页大小固定为8KB，减去content页头部固定长度，除以数据长度和位图所需大小，即每页最多存储的数据数目。
* 扫描器：存当前扫描位置，每次顺序查找，检查位图记录。
* 插入记录：head页记录了下一个具有空闲位置的页面，再从该content页的位图中查找到第一个未被使用的数据位置，进行数据插入。
* 删除记录：RID指出了页号和秩号，进行删除。维护页链和位图等数据。
* 更新记录：调用时要保证该记录存在。
* null的实现：在数据的末尾添加小的位图，记录是否为null。实际上这是对记录管理模块透明的。

##### 索引模块

* 为存储在文件中的记录建立B+树索引，加快查找速度。
* 实现了两次
  * 第一次实现了B树，数据存在所有结点里，但是不支持先序遍历
  * 第二次实现了B+树，非叶子结点不存数据，只存其每个左孩子中的最小值。
* B/B+树：
  * 一个结点即物理上的一页（8KB），可以最大的减少查询时加载页的数量
  * 每个结点存n个数据项和n（或n+1）个子指针
  * 子指针指向一个低层的结点
  * 所有叶子结点高度相同，这是通过从根结点增加高度和减少高度实现的
* 最终实现B+树，因为可以先序遍历，优化区间查询
* B+树
  * 每个结点数据项和子指针数目相同
  * 每个非叶结点的数据项为对应子指针指向的结点的数据项的最小值
  * 叶结点保存数据
  * 数据由valueRID组成
  * 数据的大小定义：
    * 若value段大小不同，则由value段定义
    * value段大小相同，则由RID定义
  * 由于数据来自记录管理模块，RID不可能相同，因此插入的数据不可能相同。
  * null的实现：实际上对该模块的函数变量分成了两类：
    * 原型：即实际的数据，null与空指针NULL对应
    * 拓展型：实际的数据末尾连接一个字节，末尾字节表明该数据是否为null

##### 系统管理模块

* 实现基本的数据定义语言（DDL），对数据库和数据表进行管理。
* 原本通过dbcreate.cpp dbdelete.cpp 等产生二进制文件实现一些创建删除功能
* 最终统一使用lex/yacc实现所有该模块的功能
* 读取命令，调用sm的接口进行操作
* 主要功能有
  * 创建、删除、切换数据库
  * 列出所有表
  * 显示指定表的信息
  * 创建、删除表
  * 创建、删除索引

##### 查询解析模块

* 实现基本的数据操作语言（DML），对数据库里的数据进行增删改查等基本操作。
* 使用lex/yacc实现解析
* ql实现了接口
* lex/yacc调用ql的接口实现交互
* 主要功能：
  * 插入数据
  * 删除数据
  * 更新数据
  * 查询数据

#### 主要接口说明

##### 记录管理模块

```c++
class RecordManager {
public:
    // 创建记录的数据长度为recordSize的文件，成功返回true
	bool createFile(const char* filename, int recordSize); 
    // 删除文件 成功返回true
	bool destroyFile(const char* filename);
    // 打开文件，实际上设置了RH的具体数据，成功返回0；
	int openFile(const char* filename, RecordHandle& recordHandle);
    // 关闭RH，把RH的head页和bufpagemanager的脏页写回。
	bool closeFile(RecordHandle& recordHandle);
};
class RecordHandle {
public:
    // 获取指定记录 成功返回0
	int getRec(const RID &rid, Record& record);
    // 插入数据 成功返回0
	int insertRec(const char *data, RID& rid);
    // 删除数据 成功返回true
	bool deleteRec(const RID &rid);
    // 更新数据 成功返回true
	bool updateRec(const Record &rec);
};
class RecordScan {
public:
	// 初始化扫描器
	int openScan(RecordHandle &recordHandle, constSpace::AttrType attrType, int attrLength, int attrOffset, constSpace::CompOp compOp, void *value);
    // 获取下一条记录
	int getNextRec(Record& record);  
}
```

##### 索引模块

```c++
class IX_Manager {
public:
    // 创建索引文件 成功返回0
    int CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength);
    // 删除索引文件 成功返回0   
    int DestroyIndex(const char *fileName, int indexNo);
    // 打开索引文件 成功返回0
    int OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle);
    // 关闭索引文件 成功返回0
    int CloseIndex(IX_IndexHandle &indexHandle);
}
class IX_IndexHandle {
public:
    // 将head页写回硬盘
    void writeHeader();
    // 插入数据，pData是原型
    int InsertEntry(void *pData, const RID &rid);
	// 删除数据，pData是原型
    int DeleteEntry(void *pData, const RID &rid);  
    // 加载第一个叶子结点
    void loadFirstLeaf(BTNode&);
    // 加载node的下一个叶子结点
    int loadNextNode(BTNode& node);
}
class IX_IndexScan {
public:
    // 打开扫描器
    int OpenScan(IX_IndexHandle &indexHandle, CompOp compOp, void *value);   
    // 获取下一条记录
    int GetNextEntry(RID &rid);
    // 关闭扫描器
    int CloseScan();
};
```

##### 系统管理模块

```c++
class SM_Manager {
public:
	// 创建数据库 成功返回0
    int createDb(const char *dbName);
	// 删除数据库 成功返回0
    int dropDb(const char *dbName);
    // 打开数据库，加载表和属性两个RH
    int openDb(const char *dbName);
    // 关闭当前数据库
    int closeDb();
    // 显示当前库的所有表
    int showTables();
    // 显示某一表的属性信息
    int printTable(const char *tbName, ostream &o);
    // 创建表
    int createTable(const char *tbName, parser_node *fieldList);
    // 创建属性的索引
    int createIndex(const char *relName, const char *attrName);
    // 删除表
    int dropTable(const char *relName);
    // 删除属性的索引
    int dropIndex(const char *relName, const char *attrName);、
};
```

##### 查询解析模块

```c++
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

##### 语法解析模块

* scan.l 		使用lex的词法分析器scan
* parser.h         定义了YYSTYPE以及声明一些被用到的函数
* parser.y         使用yacc进行语法分析
* parser_helper.cpp   定义了parser.h里的函数以及一个全局字符栈，保存词法分析读取的记录和parser.y生成的数据结构

#### 实验结果

* 使用bison & flex 

* 在根目录下make，最终生成entry

* 运行entry，在命令行进行交互或者管道符重定向交互

* 基本的语句测试通过

* 对于较慢语句进行了小小的优化

  * 插入具有唯一主键的记录时，如果当前表中数据数目较少，且主键为int类型，则把主键记录在缓冲区中。

  * 此优化在数据条数大于缓冲区允许条数时不可用。目前设置为10M

  * 可作的拓展：缓冲区保存多对int，一对int为可能处于的一个区间。若插入的键不在所有区间里，就不需要从硬盘上读主键的内容。减少磁盘交换次数能大大减少时间。

  * 查询多个表时限定为连接操作，暴力遍历的时间复杂度为每个表的数据数乘积$o(n_1n_2...n_t)$，相当费时；本项目只对两表连接，其中一个同等关系的两个属性都具有索引，至少一个属性为主键的查询语句进行优化：同时使用两个索引的前序遍历。时间复杂度为o(min(m,n))。

  * 可作的拓展，两个属性只有一个属性有索引时，使用索引作为内循环的时间复杂度$o(mlogn)$ 。无索引时查找一个值需要遍历所有页，使用索引时查找一个值只要从根结点向下到叶子；B+树高度一般都很小，$h=1+log_mn$，m为每个结点的路数，以数据长度为4字节为例，$m=8KB\div(4B+4B+12B)=400$，因此h=3时n=16w，h=4时n=6400w，即n随h指数级上涨。

     

* macbook pro

* cpu i7-4770HQ 2.20GHz 

* 对于dataset_small

  * 插入时间分析

    * | 表名       | 数据条数 | 使能                   | 大致插入时间 |
      | ---------- | -------- | ---------------------- | ------------ |
      | customer   | 1.5w     | 不检查                 | 131ms        |
      | customer   | 1.5w     | 检查主键               | 6.4s         |
      | customer   | 1.5w     | 检查主键、外键         | 6.8s         |
      | customer   | 1.5w     | 检查主键，使用主键优化 | 342ms        |
      | restaurant | 864      | 不检查                 | 10ms         |
      | restaurant | 864      | 检查主键               | 32ms         |
      | restaurant | 864      | 检查主键、外键         | 36ms         |
      | restaurant | 864      | 检查主键、使用主键优化 | 14ms         |
      | food       | 1w       | 不检查                 | 91ms         |
      | food       | 1w       | 检查主键               | 2.9s         |
      | food       | 1w       | 检查主键、外键         | 28.7s        |
      | food       | 1w       | 检查主键、使用主键优化 | 190ms        |
      | orders     | 1w       | 不检查                 | 96ms         |
      | orders     | 1w       | 检查主键               | 2.9s         |
      | orders     | 1w       | 检查主键、外键         | 58s          |
      | orders     | 1w       | 检查主键、使用主键优化 | 193ms        |

  * 查询时间分析

    * 单表查询较快，ms级别，1.5w记录查询约60ms

    * 双表连接及多表连接：慢，30s级别。以下查询分别用时20s,29s

    * ```sql
      select orders.id from orders, food where orders.food_id=food.id;	
      select orders.id from orders, customer where orders.customer_id=customer.id;
      ```

    * 双表连接，使用索引优化（优化条件：where子句具有同等关系且两个属性都有索引，其中一个属性是主键）：同样的两个查询用时81ms，85ms

  * 
#### 小组分工

* 本小组仅有一人，故分工部分略，具体工作量见设计部分。

#### 参考文献

* stanford的[redbase](https://web.stanford.edu/class/cs346/2015/)课程项目
* github上junkumar的关于redbase的[实现](https://github.com/junkumar/redbase)
* 博客[BTree和B+Tree详解](https://www.cnblogs.com/vianzhang/p/7922426.html)
* 邓俊辉《数据结构》mooc视频B树部分
* lex、yacc官方文档
* lex、yacc的[博客1](https://segmentfault.com/a/1190000000396608#articleHeader13)，[博客2](http://www.cnblogs.com/ghjbk/p/6953619.html)，[博客3](https://blog.csdn.net/huyansoft/article/details/8860224)
* 数据库课程项目相关资料（略）