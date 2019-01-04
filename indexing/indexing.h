#ifndef INDEXING_H
#define INDEXING_H
#include "../const.h"
using namespace constSpace;
class IX_IndexHandle;
class BTree;
struct Head_Page
{
    char attrType[8];
    int attrLength, entryLimit, pageNum, root, nextFree, entryNum, right;
    static void setHeader(BufType b, AttrType attrType, int attrLength);
    void parse(BufType b);
    void set(BufType b);
    AttrType getType();
};
struct Content_Page
{
    int parent, left, right, prevFree, nextFree, entryCount, childCount, isLeaf;
    char* values; RID* rids;
    int* childs;
};
struct BTNode {
    int* parent, *left, *right, *prev_free, *next_free, *entry_count, *child_count, *is_leaf;
    char* values; RID* rids;
    int* childs;
/* =================== */
    static int entryLimit;
    static int attrLength;
    static AttrType attrType;
    static BufPageManager* bufPageManager;
    int pageID, bufIndex;
    BTNode();
    BTNode(BufPageManager* bufPageManager);
    void withEmptyPage(BufType b, Head_Page headPage, int pID, int index);
    void withContentPage(BufType b, Head_Page headPage, int pID, int index);
    void insertData(int rank, char* data, const RID& rid); // 拓展型
    void setData(int rank, char* data, RID& rid); // 拓展型
    static int comp(char* v1, char* v2); // 拓展型
    static int comp(char* v1, const RID& r1, char* v2, const RID& r2); // 拓展型
    int search(char* data, const RID& rid); // 拓展型
    void getData(int rank, char* data, RID& rid) const; // 拓展型
    void removeData(int rank, char* data, RID& rid); // 拓展型
    static int getParentRank(const BTNode&, const BTNode&);
    int getChild(int child) const;
    void setChild(int rank, int pageID);
    int getNextData(int& rank, char* data, RID& rid) const;
    void removeData(int rank);
    int removeChild(int rank);
    void insertChild(int rank, int id);
    int getChildSize() const;
    int getDataSize() const;
    int getParent() const;
    void markDirty() const;
    void setParent(int p);
    void setLeaf(bool);
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
    int search(char* e, const RID& rid); // 原型
    bool insert(char* e, const RID& rid); // 原型
    bool remove(char* e, const RID& rid); // 原型
    void init(int fileID, BufType b, IX_Manager* ix_manager);
    void traverse();
    void traverse(int pageID);
    void printFreeLink();
    void printHeadPage();
    void findFirstValue(void* value, int& pageID, int& rank);
    int findLastValue(void* value, int& pageID, int& rank, RID& rid);
    IX_IndexHandle();                             // Constructor
    ~IX_IndexHandle();                             // Destructor
    int InsertEntry(void *pData, const RID &rid);  // Insert new index entry
    int InsertNull(const RID &rid);
    int DeleteEntry(void *pData, const RID &rid);  // Delete index entry
    //int SearchEntry(void *pData, RID& rid, bool compare); // 搜索大于pData rid并更新
    int ForcePages();                             // Copy index to disk
    BTNode loadNode(int id);
    BTNode loadFirstLeaf();
    int loadNextNode(BTNode& node);
private:
    int _hot; int attrLength;
    int fileID;
    struct Head_Page headPage;
    FileManager* fileManager;
    BufPageManager* bufPageManager;
    int updateParent(BTNode& node_v);
    int updateParent(BTNode& node_v, int rank);
    void updateToRoot(int v);
    void solveOverflow(BTNode&);
    void solveUnderflow(BTNode&);
    BTNode createNode();
    void removeLeafLink(BTNode& node);
    void addLeafLinkAfter(BTNode& node_v, BTNode& node_after);
    //int searchEntryRecur(int v, void *pData, RID& rid);
    void removeLink(const BTNode& node);
    void addLink(const BTNode& node);
};
class IX_IndexScan {
private:
    void* value, *current_value;
    RID current_RID;
    int rank;
    AttrType attrType; CompOp compOp; 
    int attrLength, numScanned;
    bool(*comparator)(void *, void *, AttrType, int);
    IX_IndexHandle* indexHandle;
    BTNode node_current;
public:
	IX_IndexScan();                                 // Constructor
    ~IX_IndexScan();                                 // Destructor
    int OpenScan(IX_IndexHandle &indexHandle, CompOp compOp, void *value);   // value = NULL 时， compOp只能为EQ NE        
    int GetNextEntry(RID &rid);                         // Get next matching entry
    int CloseScan();                                 // Terminate index scan
    int tryLoadNext(BTNode& node, int& rank, char* data, RID& rid);
    int findNext(RID& rid);
};
#endif
