#include "indexing.h"
/* private function */
void setFileName(char* dest, const char* fileName, int indexNo) {

}
/* ix manager implement */
IX_Manager::IX_Manager(FileManager* fileManager, BufPageManager* bufPageManager) {
	this->fileManager = fileManager;
	this->bufPageManager = bufPageManager;
}
IX_Manager::~IX_Manager() {

}
int IX_Manager::CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength) {
    // 设置好文件名
	char indexFileName[256];
    setFileName(indexFileName, fileName, indexNo);
    // 创建，打开文件，申请headpage的缓存，写进缓存，然后更新
    bool created = fileManager->createFile(indexFileName);
    assert(created);
    int fileID;
    bool opened = fileManager->openFile(indexFileName, fileID);
    assert(opened);
    int index;
    BufType b = bufPageManager->allocPage(fileID, 0, index, false);
    Head_Page::setHeader(b, attrType, attrLength);
    bufPageManager->markDirty(index);
    bufPageManager->writeBack(index);
    assert(fileManager->closeFile(index) == 0);
    return 0;
}
int IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    char indexFileName[256];
    setFileName(indexFileName, fileName, indexNo);
    if (remove(indexFileName)) 
        return -1;
    return 0;
}
int IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
	int fileID;
    bool opened = fileManager->openFile(indexFileName, fileID);
    if (!opened) {
    	
    	return -1;
	}
	int index;
	BufType b = bufPageManager->getPage(fileID, 0, index);
	indexHandle.init(fileID, b, this);
    return 0;
}
int IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {

}
/* IX_IndexHandle implement */
void IX_IndexHandle::init(int fileID, BufType b, IX_Manager* ix_manager) {
	this->fileID = fileID;
	headPage.parse(b);
	fileManager = ix_manager->fileManager;
	bufPageManager = ix_manager->bufPageManager;
}
IX_IndexHandle::IX_IndexHandle();                             // Constructor
IX_IndexHandle::~IX_IndexHandle();                             // Destructor
int IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {

}
int IX_IndexHandle::DeleteEntry(void *pData, const RID &rid);  // Delete index entry
int ForcePages();
