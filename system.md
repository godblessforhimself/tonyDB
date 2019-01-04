#### 系统管理模块

##### 参考资料

* 课程文件《数据库大作业详细说明》系统管理模块
* stanford.redbase(https://web.stanford.edu/class/cs346/2015/redbase-sm.html#metadata)
* redbase github项目https://github.com/junkumar/redbase/blob/master/src/sm_manager.cc

##### 设计思路

* 参数简单的命令通过cpp编译生成可执行文件，命令行执行调用
* 每个表的信息使用Relcat的一个条目和Attrcat的多个条目记录，前者记录一个表的对象，后者记录表中每一个属性的详细信息
* 创建表和删除表通过记录管理模块和索引模块的接口实现。

##### 接口及实现

* 关系和属性使用两个结构记录：

* ``` c++
  struct Relcat {
      char *relName;
      int tupleLength;
      int attrCount;
      int indexCount;
  };
  struct Attrcat {
      char* relName;
      char* attrName;
      int offset;
      AttrType attrType;
      int attrLenth;
      int indexNo;
  };
  ```

* > 创建数据库CREATE DATABASE DBname           dbcreate.cpp
  >
  > 删除数据库DROP DATABASE DBname              dbdrop.cpp
  >
  > 切换数据库USE DATABASE DBname                 dbuse.cpp
  >
  > 列出现有的所有数据库以及其包含的所有表名 SHOW DATABASE DBname dbshow.cpp
  >
  > 这4个cpp通过调用SM_Manager和RecordManage,filesystem等模块实现功能


```c++
SM_Manager的主要接口
int OpenDb(const char *dbName);
int CloseDb();
int CreateTable(const char *relName, int attrCount, AttrInfo *attributes);
int CreateIndex(const char *relName, const char *attrName);
int DropTable(const char *relName);
int DropIndex(const char *relName, const char *attrName);
int Load(const char *relName, const char *fileName);
int Help();
int Help(const char *relName);
int Print(const char *relName);
int Set(const char *paramName, const char *value);
int Get(const string& paramName, string& value) const;
```

* > int OpenDb(const char *dbName); 
  >
  > 供use database打开数据库，通过系统命令chdir和RecordManage的openFile打开关系表和属性的数据库

* > int CloseDb();
  >
  > Use database关闭数据库，通过RecordHandle的关闭实现，更新表和属性。

* > int CreateTable(const char *relName, int attrCount, AttrInfo *attributes);
  >
  > CREATE TABLE *tableName(attrName1 Type1, attrName2 Type2,…, attrNameN TypeN NOT NULL, PRIMARY KEY(attrName1))* 调用该函数实现

* > int CreateIndex(const char *relName, const char *attrName);
  >
  > 调用IX_Manage新建一个索引文件，通过RecordManage遍历attr的所有值，调用IX_Handle的插入，关闭IX_Handle。
  >
  > 然后更新attrcat对应项的indexNo

* > int DropTable(const char *relName);
  >
  > 调用IX_Manager::DestroyIndex删除索引文件，调用RM_Manager::DestroyFile删除数据表文件

* > int DropIndex(const char *relName, const char *attrName);
  >调用IX_Manager::DestroyIndex删除索引文件

##### 其他

* 创建表和删除表命令的解析不是很简单，可能需要使用parser等工具进行语法分析
* 等待编写测试模块和查询解析模块