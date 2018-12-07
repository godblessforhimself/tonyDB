# database class project , by tony 2015011284

## 记录管理模块

- ### 记录：长度可变的字节数组，唯一标识RID

- ### 文件：一个数据库对应一个文件

- ### 第一页：记录长度、第i页的记录数目、总记录数目

- ### 位数组，记录每页空闲位置

- ### 一页最多几个记录？

  - ### 8KB/1BYTE=8k,  8kbit=1kB

  - ### 每页的头1KB是位数组。

  - ### 第一页由4B的uint组成，头两个为记录长度、总记录数目，接下来共

- ### 扫描文件，属性值

### 确定的接口

- ```c++
  bool CreateFile(const char *fileName, int recordSize);//create a file return success
  bool DestroyFile(const char *fileName);	//destroy a file , return success
  RM_FileHandle& OpenFile(const char *fileName); // open a file, return fileHandler
  bool CloseFile(RM_FileHandle &fileHandle);	//close a file
  RM_Record& GetRec(const RID &rid) const;	//
  RID &InsertRec(const char *pData);
  DeleteRec(const RID &rid);
  UpdateRec(const RM_Record &rec)
  OpenScan(const RM_FileHandle &fileHandle, AttrType attrType,int attrLength, int attrOffset, CompOp compOp, void *value,ClientHint pinHint = NO_HINT);
  RM_Record &GetNextRec();  
  CloseScan();
  GetData(char *&pData) const;
  GetRid(RID &rid) const;
  ```



b+树

结点分为叶结点和内部结点

叶结点m个数据项，内部结点存c个索引值和c个孩子指针 c<=m

每个索引值表示 其子结点的叶结点集的数据里的最小数据项



插入x

找到叶子结点的插入前位置i

插入x

若叶子结点溢出，找到叶子结点的中间值m，拆分，更新，向上更新，并及时停止传播



移除x

找到叶子结点中的x数据项，移除，更新父结点的值，处理该结点的下溢