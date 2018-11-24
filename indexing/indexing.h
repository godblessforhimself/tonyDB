#ifndef INDEXING_H
#define INDEXING_H
#include "../const.h"
using namespace constSpace;
struct Head_Page
{
    char attrType[8];
    int attrLength, entryLimit;
    static void setHeader(BufType b, AttrType attrType, int attrLength) {

    }
    void parse(BufType b) {
    	Head_Page* ptr = (Head_Page*)b;
    	memcpy((void*)attrType,(void*)ptr, sizeof(attrType));
    	attrLength = ptr->attrLength;
    	entryLimit = ptr->entryLimit;
    	
	}
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
	int fileID;
	struct Head_Page headPage;
	FileManager* fileManager;
    BufPageManager* bufPageManager;
};
class IX_IndexScan {
public:
	IX_IndexScan();                                 // Constructor
    ~IX_IndexScan();                                 // Destructor
    int OpenScan(const IX_IndexHandle &indexHandle, CompOp compOp, void *value);           
    int GetNextEntry(RID &rid);                         // Get next matching entry
    int CloseScan();                                 // Terminate index scan
};
