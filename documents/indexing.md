#### 索引模块indexing

* 参考资料：
  * 邓俊辉 《数据结构》 B-Tree 课件
  * stanford.RedBase (https://web.stanford.edu/class/cs346/2015/redbase-ix.html)

* 主要的类与结构体：

  * IX_IndexScan

  * ```c++
    int OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value);           
    int GetNextEntry(RID &rid);                         // Get next matching entry
    int CloseScan(); 
    ```

  * IX_IndexHandle

  * ```c++
    int InsertEntry(void *pData, const RID &rid);  // Insert new index entry
    int DeleteEntry(void *pData, const RID &rid);  // Delete index entry
    int ForcePages(); 
    ```

  * IX_Manager

  * ```c++
    int CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength);
    int DestroyIndex(const char *fileName, int indexNo);
    int OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle);
    int CloseIndex(IX_IndexHandle &indexHandle);
    ```

  * BTree

  * ```c++
    int search(char* e, const RID& rid);
    bool insert(char* e, const RID& rid);
    bool remove(char* e, const RID& rid);
    ```

  * Head_Page

  * ```c++
    struct Head_Page
    {
        char attrType[8];
        int attrLength, entryLimit, size;
    };
    ```

* 设计和思路：

  * IX_IndexScan，IX_IndexHandle，IX_Manager供数据库调用

  * 文件首页为头页，保存属性类型、长度、每页最多entry数、总页数这些数据

  * 其他页面为B+树的结点，结构为

  * ```c++
    struct Content_Page
    {
        int parent, dataSize, childSize;
        char* values;
        RID* rids;
        int* childs;
    };
    ```

  * 数据由value和rid组成，项数由dataSize记录；子结点由childs组成，项数为childSize；

  * IX_IndexHandle底层通过BTree的对象，实现插入、删除等功能。

  * BTree通过filesystem进行文件读写

  * IX_IndexScan通过BTree的部分遍历，进行查询。

* 其他：

  * 修改了filesystem里的MyBitMap模块，使得filesystem的头文件可以被include多次并链接
  * 测试indexing的功能尚待解决