#ifndef INDEXING_H
#define INDEXING_H
#include "../const.h"
using namespace constSpace;
class IX_IndexHandle;
class BTree;
struct Head_Page
{
    char attrType[8];
    int attrLength, entryLimit, pageNum, root, nextFree, entryNum;
    static void setHeader(BufType b, AttrType attrType, int attrLength);
    void parse(BufType b);
    void set(BufType b);
    AttrType getType();
};
struct Content_Page
{
    int parent, dataSize, childSize, prevFree, nextFree;
    char* values;
    RID* rids;
    int* childs;
};
struct BTNode {
    int* parent;
    char* values;
    RID* rids;
    int* childs;
    int* data_size, *child_size, *prev_free, *next_free;
/* =================== */
    int entryLimit, attrLength;
    int pageID, bufIndex;
    AttrType attrType;
    BufPageManager* bufPageManager;
    BTNode(BufPageManager* bufPageManager);
    void withEmptyPage(BufType b, Head_Page headPage, int pID, int index);
    void withContentPage(BufType b, Head_Page headPage, int pID, int index);
    int comp(char* v1, char* v2);
    int search(char* data, const RID& rid);
    bool equals(int dataindex, char* data, const RID& rid);
    int getChild(int child) const;
    void setChild(int rank, int pageID);
    void getData(int rank, char* data, RID& rid) const;
    void setData(int rank, char* data, RID& rid);
    void removeData(int rank);
    void removeData(int rank, char* data, RID& rid);
    int removeChild(int rank);
    void insertData(int rank, char* data, const RID& rid);
    void insertChild(int rank, int id);
    int getChildSize() const;
    int getDataSize() const;
    int getParent() const;
    void markDirty() const;
    void setParent(int p);
    bool isLeaf() const;
    void print() const;
};
class IX_Manager {
public:
	friend class IX_IndexHandle;
	IX_Manager(FileManager* fileManager, BufPageManager* bufPageManager);              // Constructor
	~IX_Manager();                             // Destructor
    int CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength);
    int DestroyIndex(const char *fileName, int indexNo);
    int OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle);
    int CloseIndex(IX_IndexHandle &indexHandle);  // Close index
private:
    FileManager* fileManager;
    BufPageManager* bufPageManager;
};
class IX_IndexHandle {
    public:
    friend struct BTNode;
    friend class IX_Manager;
    friend class IX_IndexScan;
    void writeHeader();
    int search(char* e, const RID& rid); 
    bool insert(char* e, const RID& rid);
    bool remove(char* e, const RID& rid);
    void init(int fileID, BufType b, IX_Manager* ix_manager);
    void traverse();
    void traverse(int pageID);
    void printHeadPage();
    IX_IndexHandle();                             // Constructor
    ~IX_IndexHandle();                             // Destructor
    int InsertEntry(void *pData, const RID &rid);  // Insert new index entry
    int DeleteEntry(void *pData, const RID &rid);  // Delete index entry
    int SearchEntry(void *pData, RID& rid, bool compare); // 搜索大于pData rid并更新
    int ForcePages();                             // Copy index to disk
private:
    int _hot; int attrLength;
    int fileID;
    struct Head_Page headPage;
    FileManager* fileManager;
    BufPageManager* bufPageManager;
    void solveOverflow(BTNode&);
    void solveUnderflow(BTNode&);
    BTNode loadNode(int id);
    BTNode createNode();
    int searchEntryRecur(int v, void *pData, RID& rid);
    void removeLink(const BTNode& node);
    void addLink(const BTNode& node);
};
class IX_IndexScan {
private:
    void* value, *current_value;
    RID current_RID;
    AttrType attrType; CompOp compOp; int attrLength, numScanned;
    bool(*comparator)(void *, void *, AttrType, int);
    IX_IndexHandle* indexHandle;
public:
	IX_IndexScan();                                 // Constructor
    ~IX_IndexScan();                                 // Destructor
    int OpenScan(IX_IndexHandle &indexHandle, CompOp compOp, void *value);           
    int GetNextEntry(RID &rid);                         // Get next matching entry
    int CloseScan();                                 // Terminate index scan
};
#endif
