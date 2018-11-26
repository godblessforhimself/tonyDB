#ifndef INDEXING_H
#define INDEXING_H
#include "../const.h"
using namespace constSpace;
class IX_IndexHandle;
class BTree;
struct Head_Page
{
    char attrType[8];
    int attrLength, entryLimit, size;
    void setHeader(BufType b, AttrType attrType, int attrLength) {
        char typestr[8];
        switch (attrType) {
            case constSpace::INT:
                strncpy(typestr, "INT", sizeof(typestr));
                break;
            case FLOAT:
                strncpy(typestr, "FLOAT", sizeof(typestr));
                break;
            case STRING:
                strncpy(typestr, "STRING", sizeof(typestr));
                break;
            default:
                strncpy(typestr, "INVALID", sizeof(typestr));
        }
        memcpy((void*)b, typestr, sizeof(attrType));
        this->attrLength = attrLength;
        entryLimit = (PAGESIZE - sizeof(int) * 4) / (attrLength + sizeof(RID) + sizeof(int));
    }
    void parse(BufType b) {
    	Head_Page* ptr = (Head_Page*)b;
    	memcpy((void*)attrType,(void*)ptr, sizeof(attrType));
    	attrLength = ptr->attrLength;
    	entryLimit = ptr->entryLimit;
    	
	}
};
struct Content_Page
{
    int parent, dataSize, childSize;
    char* values;
    char* rids;
    char* childs;
};
struct BTNode {
    int* parent;
    char* values;
    RID* rids;
    int* childs;
    int* data_size, *child_size;
/* =================== */
    int entryLimit, attrLength;
    int pageID;
    AttrType attrType;
    BTNode();
    void withEmptyPage(BufType b, BTree btree, int pID);
    void withContentPage(BufType b, BTree btree, int pID);
    int comp(char* v1, char* v2);
    int search(char* data, const RID& rid);
    bool equals(int dataindex, char* data, const RID& rid);
    int getChild(int child);
    void setChild(int rank, int pageID);
    void getData(int rank, char* data, RID& rid);
    void setData(int rank, char* data, RID& rid);
    void removeData(int rank);
    void removeData(int rank, char* data, RID& rid);
    int removeChild(int rank);
    void insertData(int rank, char* data, const RID& rid);
    void insertChild(int rank, int id);
    void insertFrom(int r1, BTNode& node, int r2);
    int getChildSize();
    int getDataSize();
    int getID();
    int getParent();
    void setParent(int p);
    bool isLeaf();
};
class BTree {
public:
    friend struct BTNode;
    int search(char* e, const RID& rid);
    bool insert(char* e, const RID& rid);
    bool remove(char* e, const RID& rid);
    void init(BufType b, int fileID, FileManager* fileManager, BufPageManager* bufPageManager);
private:
    int _size; int _order; int _root;
    int _hot; int attrLength;
    int fileID;
    struct Head_Page headPage;
    FileManager* fileManager;
    BufPageManager* bufPageManager;
    void solveOverflow(BTNode&);
    void solveUnderflow(BTNode&);
    BTNode loadNode(int id);
    BTNode createNode();
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
	void init(int fileID, BufType b, IX_Manager* ix_manager);
    IX_IndexHandle();                             // Constructor
    ~IX_IndexHandle();                             // Destructor
    int InsertEntry(void *pData, const RID &rid);  // Insert new index entry
    int DeleteEntry(void *pData, const RID &rid);  // Delete index entry
    int ForcePages();                             // Copy index to disk
private:
    BTree btree;
};
class IX_IndexScan {
public:
	IX_IndexScan();                                 // Constructor
    ~IX_IndexScan();                                 // Destructor
    int OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value);           
    int GetNextEntry(RID &rid);                         // Get next matching entry
    int CloseScan();                                 // Terminate index scan
};
#endif
