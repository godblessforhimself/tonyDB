#include "rm.h"
#include "../filesystem/bufmanager/BufPageManager.h"
#include "../filesystem/fileio/FileManager.h"
#define markItDirty(x) (this->bufPageManager->markDirty((x)))
#define recordPtr(x) (PageReader::getRPtr(b, x, this->fileHeader.recordStart, this->fileHeader.recordSize))
void initFH(BufType fp, int rs) {
	struct FileHeader* fh = (FileHeader*)fp;
	int recordNum, bitmapSize, addition = sizeof(int) * 3;
	recordNum = 8 * (PAGESIZE - addition - 1) / (8 * rs + 1);
	bitmapSize = (recordNum % 8 == 0) ? recordNum / 8 : (recordNum / 8) + 1;
	fh->recordSize = rs;
	fh->recordNum = recordNum;
	fh->pageNum = 1;
	fh->next = 1;
	fh->bitmapStart = addition;
	fh->bitmapSize = bitmapSize;
	fh->recordStart = addition + bitmapSize;
	fh->debug();
}
RecordManager::RecordManager(FileManager* fileManager, BufPageManager* bufPageManager) {
	MyBitMap::initConst();
	this->fileManager = fileManager; 
	this->bufPageManager = bufPageManager;
}
RecordManager::~RecordManager() {
	this->bufPageManager->close();
}
bool RecordManager::createFile(const char* filename, int recordSize) {
	/* create a file, then init settings in first page*/
	//Debug::debug("RecordManager::createFile %s:\n", filename);
	if (!fileManager->createFile(filename)) {
		Debug::debug("RecordManager::createFile createFile failed!\n");
		return false;
	}
	int fileID;
	if (!fileManager->openFile(filename, fileID)) {
		Debug::debug("RecordManager::createFile openFile failed!\n");
		return false;
	}
	int index;
	BufType firstPage = bufPageManager->allocPage(fileID, 0, index, false);
	initFH(firstPage, recordSize);
	bufPageManager->markDirty(index);
	bufPageManager->writeBack(index);
	if (fileManager->closeFile(fileID) != 0) {
		Debug::debug("RecordManager::createFile closeFile failed!");
	}
	Debug::debug("RecordManager::createFile %s success!\n", filename);
	cout << "创建文件" << filename << "成功,记录大小" << recordSize << endl;
	return true;
}
bool RecordManager::destroyFile(const char* filename) {
	if (remove(filename)) {
		printf("delete %s fail\n", filename);
		return false;
	}
	printf("delete %s success\n", filename);
	return true;
}
int RecordManager::openFile(const char* filename, RecordHandle& recordHandle) {
	int fileID;
	if (!fileManager->openFile(filename, fileID)) {
		cout << "RecordManager::openFile打开文件" << filename << "失败\n";
		return -1;
	}
	int index;
	BufType firstPage = bufPageManager->getPage(fileID, 0, index);
	if (!recordHandle.init(firstPage, fileID)) {
		printf("RecordManager::openFile failed to init!\n");
		return -1;
	}
	recordHandle.setManager(this);
	recordHandle.setIndex(index);
	//printf("RecordManager::openFile %s success!\n", filename);
	return 0;
}
bool RecordManager::closeFile(RecordHandle& recordHandle) {
	recordHandle.writeFH();
	bufPageManager->close();
	if (fileManager->closeFile(recordHandle.getFileID()) != 0) {
		return false;
	}
	return true;
}

/* ======================================================================== */
char* getRecordPosition(BufType b, int recordStart, int slot, int recordSize) {
	char* ret = ((char*)b + recordStart + slot * recordSize);
	return ret;
}
RecordHandle::~RecordHandle() {
	forceDisk();
	fileManager->closeFile(fileID);
}
void RecordHandle::forceDisk() {
	writeFH();
	bufPageManager->close();
}
void RecordHandle::writeFH() {
	BufType b = bufPageManager->getPage(fileID, 0, index);
	*(FileHeader*)b = fileHeader;
	bufPageManager->markDirty(index);
	bufPageManager->writeBack(index);
}
int RecordHandle::getRec(const RID &rid, Record& record) {
	int recordSize = fileHeader.recordSize, maxPage = fileHeader.pageNum;
	int page = rid.getPage(), slot = rid.getSlot(), index;
	if (page >= maxPage || slot >= fileHeader.recordNum) {
		Debug::debug("RecordHandle.getRec: rid[%d,%d]>=[%d,%d]", page, slot, maxPage, fileHeader.recordNum);
		return -2;
	}
	BufType b = bufPageManager->getPage(fileID, page, index);
	if (!PageReader::getBitmap(b, fileHeader.bitmapStart, slot)) {
		Debug::debug("RecordHandle.getRec: rid[%d, %d] getBitmap false", page, slot);
		return -1;
	}
	bufPageManager->access(index);
	char *recordPosition = getRecordPosition(b, this->fileHeader.recordStart, slot, recordSize);
	record.set(rid, recordPosition, recordSize);
	// cout << "最后一字节" << (unsigned int)recordPosition[recordSize - 1] << endl;
	// cout << "getRec里的size=" << recordSize << endl;
	return 0;	
}
int RecordHandle::insertRec(const char *data, RID& rid) {
	// find the free position, then insert in it
	// todo: check the page exist!
	int free = fileHeader.next, index, maxPage = fileHeader.pageNum, recordNum = fileHeader.recordNum;
	assert(free <= maxPage);
	if (free == maxPage) {
		// new page
		BufType b = bufPageManager->allocPage(fileID, free, index);
		PageReader::initPage(b, free);
		fileHeader.pageNum++;
		PageReader::setCount(b, 1);
		PageReader::writeRecord(b, fileHeader.recordStart, fileHeader.recordSize, 0, data);
		PageReader::setBitmap(b, fileHeader.bitmapStart, 0, true);
		bufPageManager->markDirty(index);
		bufPageManager->writeBack(index);
		rid.set(free, 0);
		Debug::debug("insertRec rid[%d, %d] len %d", free, 0, fileHeader.recordSize);
		return 0;
	}
	/* so page[free] exist! */
	BufType b = bufPageManager->getPage(this->fileID, free, index);
	int count = PageReader::getCount(b);
	assert(count < recordNum);
	char *bm = (char*)b + fileHeader.bitmapStart;
	int slot = PageReader::getNext(-1, bm, false, fileHeader.recordNum);
	PageReader::writeRecord(b, this->fileHeader.recordStart, this->fileHeader.recordSize, slot, data);
	PageReader::setBitmap(b, this->fileHeader.bitmapStart, slot, true);
	PageReader::setCount(b, ++count);
	bufPageManager->markDirty(index);
	bufPageManager->writeBack(index);
	rid.set(free, slot);
	Debug::debug("insertRec rid[%d, %d] len %d", free, slot, fileHeader.recordSize);
	if (count == recordNum) {
		// update free linklist
		int next = PageReader::getNext(b), nindex;
		// check if next page exist
		assert(next <= maxPage);
		fileHeader.next = next;
		if (next != maxPage) {
			BufType nextFreePage = bufPageManager->getPage(this->fileID, next, nindex);
			PageReader::setPrev(nextFreePage, 0);
			bufPageManager->markDirty(nindex);
			bufPageManager->writeBack(nindex);
		}
	}
	return 0;
}
bool RecordHandle::deleteRec(const RID &rid) {
	FileHeader& fileHeader = this->fileHeader;
	int page = rid.getPage(), slot = rid.getSlot(), index;
	if (page >= fileHeader.pageNum || slot >= fileHeader.recordNum) {
		Debug::debug("RecordHandle.deleteRec: delete at [%d,%d] >= [%d,%d]", page, slot, fileHeader.pageNum, fileHeader.recordNum);
		return false;
	}
	assert((page < fileHeader.pageNum) && (slot < fileHeader.recordNum));
	// find the page 
	// cout << fileID << " ," <<  page << endl;
	BufType b = bufPageManager->getPage(fileID, page, index), temp;
	// check if bitmap[slot] is true 
	if (PageReader::getBitmap(b, fileHeader.bitmapStart, slot)) {
		// revise bitmap and count mark it dirty
		PageReader::setBitmap(b, fileHeader.bitmapStart, slot, false);
		markItDirty(index);
		int count = PageReader::getCount(b);
		PageReader::setCount(b, count - 1);
		//check link
		if (count == fileHeader.recordNum) {
			// add to linklist
			int prevPage = fileHeader.next, pindex, next;
			assert(prevPage != page);
			if (prevPage > page) {
				// free sequence is [head, page, prevPage]
				fileHeader.next = page;
				PageReader::setPrev(b, 0);
				assert(prevPage <= fileHeader.pageNum);
				if (prevPage != fileHeader.pageNum) {
					// prevPage exist
					temp = this->bufPageManager->getPage(this->fileID, prevPage, pindex);
					PageReader::setNext(b, prevPage);
					PageReader::setPrev(temp, page);
					markItDirty(pindex);
				}
				return true;
			}
			while (1) {
				temp = this->bufPageManager->getPage(this->fileID, prevPage, pindex);
				next = PageReader::getNext(temp);
				assert(next <= fileHeader.pageNum);
				if (next > page) break;
				prevPage = next;
			}
			//free page sequence is [prevPage, page, next]
			PageReader::setNext(temp, page);
			markItDirty(pindex);
			PageReader::setPrev(b, prevPage);
			if (next != fileHeader.pageNum) {
				// next page exist
				temp = this->bufPageManager->getPage(this->fileID, prevPage, pindex);
				PageReader::setNext(b, next);
				PageReader::setPrev(temp, page);
				markItDirty(pindex);
			}
			return true;
		}
	} else {
		printf("deleteRec::delete a record not exist:[%d,%d]", page, slot);
		return false;
	}
	return true;
}
BufType RecordHandle::getPageContent(int page, int& index) {
	return bufPageManager->getPage(fileID, page, index);
}
bool RecordHandle::updateRec(const Record &rec) {
	FileHeader& fileHeader = this->fileHeader;
	//check exist
	RID rid = rec.getRID();
	int page = rid.getPage(), slot = rid.getSlot(), index;
	if (page >= fileHeader.pageNum) {
		printf("updateRec::page overflow [%d, %d]\n", page, fileHeader.pageNum);
		return false;
	}
	BufType b = this->getPageContent(page, index);
	bool valid = PageReader::getBitmap(b, fileHeader.bitmapStart, slot);
	if (!valid) {
		printf("updateRec::not valid [%d, %d]\n", page, slot);
		return false;
	}
	//update data
	PageReader::writeRecord(b, fileHeader.recordStart, fileHeader.recordSize, slot, rec.getData());
	markItDirty(index);
	return true;
}
bool RecordHandle::init(const BufType fh, int fileID) {
	this->fileHeader = *(FileHeader*)fh;
	this->valid = true;
	this->fileID = fileID;
	return true;
}
RecordHandle::RecordHandle() {
	this->fileID = -1;
	this->valid = false;
}
int RecordHandle::getFileID() {
	return this->fileID;
}
/* ======================================================= */
int RecordScan::openScan(RecordHandle &recordHandle, constSpace::AttrType attrType, int attrLength, int attrOffset, constSpace::CompOp compOp, void *value) {
	this->recordHandle = &recordHandle;
	this->attrType = attrType;
	this->attrLength = attrLength;
	this->attrOffset = attrOffset;
	this->compOp = compOp;
	this->scanSlot = -1;
	this->scanPage = -1;
	this->pageNum = recordHandle.getPageNum();
	this->slotNum = recordHandle.getSlotNum();
	this->recordNum = recordHandle.getRecordNum();
	this->recordSize = recordHandle.getRecordSize();
	this->bitmapStart = recordHandle.getBitmapStart();
	this->bitmapSize = recordHandle.getBitmapSize();
	this->setFileHeader(recordHandle.fileHeader);
	if (compOp != NO_OP) {
		this->value = new char[attrLength];
		memcpy(this->value, value, attrLength);
	} else
		this->value = NULL;
	comparator = getComparator(compOp);
  	return 0;
}
int RecordScan::getNextRec(Record& record) {
	if (pageNum <= 1) {
		return -1;
	}
	if (scanPage < 0) {
		scanPage = 1;
		scanSlot = -1;
	}
	while (1) {
		if (scanPage >= pageNum) {
			return -1;
		}
		int index;
		BufType b = recordHandle->getPageContent(scanPage, index);
		char *bm = (char*)b + bitmapStart;
		int nextUsedSlot = PageReader::getNext(scanSlot, bm, true, recordNum);
		scanSlot = nextUsedSlot;
		assert(nextUsedSlot < slotNum);
		if (nextUsedSlot < 0) {
			// 跳到下一页
			scanPage++;
			scanSlot = -1;
		} else {
			// 检查是否有效
			char* rdata = (recordPtr(nextUsedSlot));
			void* v1 = (void*)(rdata + this->attrOffset);
			bool valid = (comparator == NULL) ? true : comparator(v1, this->value, this->attrType, this->attrLength);
			if (valid) {
				// 只在这里返回
				RID rid(this->scanPage, nextUsedSlot);
				record.set(rid, rdata, this->recordSize);
				return 0;
			}
		}
	}
}
RecordScan::~RecordScan() {
	if (value != NULL)
		delete[] (char*)value;
}
bool RecordScan::closeScan() {
	if (value != NULL)
		delete[] (char*)value;
	return true;
}

