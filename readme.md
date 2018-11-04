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
