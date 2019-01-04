#ifndef RM_H
#define RM_H
#include "../const.h"
#include "../filesystem/utils/pagedef.h"
#include <string.h>
using namespace constSpace;
/* 
	RecordManager 创建、关闭文件，初始化、关闭RecordHandle
	RecordHandle 读写记录
	RecordScan 由RecordHandle生成，根据条件进行遍历
*/
class FileManager;
struct BufPageManager;
class RecordHandle;
class RecordScan;
struct FileHeader {
	int recordSize;		
	int recordNum;		// every page record limit number
	int pageNum;		// include fileheader page,[1,+)
	int next;			// next free page index, init value is 1
	int bitmapStart;	// the offset of bitmap in page
	int bitmapSize;		// the size of bitmap
	int recordStart;	// the offset of record
	void debug() {
		Debug::debug("FileHeader[记录长度 %d, 记录数量上限 %d, 页数 %d, "
			"下一页 %d, 位图偏移 %d, 位图字节数 %d, 记录偏移 %d]", \
			recordSize, recordNum, pageNum, next, bitmapStart, bitmapSize, recordStart);
	}
};
/*
content page
int prev,next,count;
char[] bitmap;
char[] records;
*/
class PageReader {
public:
	static int getPrev(BufType page) {
		return page[0];
	}
	static int getNext(BufType page) {
		return page[1];
	}
	static int getCount(BufType page) {
		return page[2];
	}
	static void setCount(BufType page, int v) {
		page[2] = v;
	}
	static void setPrev(BufType page, int v) {
		page[0] = v;
	}
	static void setNext(BufType page, int v) {
		page[1] = v;
	}
	/* init page */
	static void initPage(BufType page, int index) {
		memset((void*)page, 0, PAGESIZE);
		setPrev(page, 0);
		setNext(page, index + 1);
		setCount(page, 0);
	}
	/* read state of record i;
		change state of record i;
		*/
	static void charToBin(char c) {
		for (int i = 0; i < 8; ++i) {
			printf("%d", (c & (1 << i)) ? 1 : 0);
		}
	}
	static bool getBitmap(char *bm, int offset) {
		char value = *(bm + (offset >> 3));
		return (value & (1 << (offset % 8)));
	}

	static bool getBitmap(BufType p, int bitmapStart, int slot) {
		char states = *((char*)p + (bitmapStart + (slot >> 3)));
		//Debug::debug("getBitmap [%d,%d] = ", slot, (slot >> 3));
		//charToBin(states);
		if (states & (1 << (slot & 7)))
			return true;
		return false;
	}
	static void setBitmap(BufType p, int bitmapStart, int slot, bool used) {
		char* data = ((char*)p + (bitmapStart + (slot >> 3)));
		if (used) {
			*data |= 1 << (slot & 7);
		} else {
			*data &= ~(1 << (slot & 7));
		}
	}
	static int getFreeSlot(BufType p, int bitmapStart, int bitmapSize) {
		/* if 8bit is 1, */
		char* bitmap = ((char*)p + bitmapStart);
		int ptr = 0;
		while (1) {
			if (ptr >= bitmapSize) {
				return -1;
			}
			if (~bitmap[ptr]) {
				break;
			}
			ptr++;
		}
		for (int i = 0; i < 8; ++i) {
			if ((bitmap[ptr] & (1 << i)) == 0) {
				return i + 8 * ptr;
			}
		}
		return -2;
	}
	static int getNext(int least, char *bm, bool used, int bmsize) {
		// 获取下一个
		for (int i = least + 1; i < bmsize; ++i) {
			if (getBitmap(bm, i) == used)
				return i;
		}
		return -1;
	}
	static int getUsedSlot(BufType p, int pslot, int bitmapStart, int bitmapSize) {
		/* if 8bit is 0, */
		char* bitmap = ((char*)p + bitmapStart);
		int ptr = pslot >> 3;
		while (1) {
			while (1) {
				if (ptr >= bitmapSize) {
					return -1;
				}
				if (bitmap[ptr]) {
					break;
				}
				ptr++;
			}
			for (int i = 0; i < 8; ++i) {
				if ((bitmap[ptr] & (1 << i))) {
					int slot = i + 8 * ptr;
					if (slot >= pslot)
						return slot;
				}
			}
			ptr++;
		}
	}
	static char* getRPtr(BufType p, int slot, int recordStart, int recordSize) {
		return ((char*)p + (recordStart + slot * recordSize));
	}
	static void writeRecord(BufType p, int recordStart, int recordSize, int slot, const char* data) {
		char* dest = ((char*)p + (recordStart + recordSize * slot));
		memcpy(dest, data, recordSize);
	}
};
class RecordManager {
public:
	RecordManager(FileManager* fileManager, BufPageManager* bufPageManager);
	~RecordManager();
	friend class RecordHandle;
	bool createFile(const char* filename, int recordSize);
	bool destroyFile(const char* filename);
	int openFile(const char* filename, RecordHandle& recordHandle);
	bool closeFile(RecordHandle& recordHandle);
private:
	FileManager* fileManager;
	BufPageManager* bufPageManager;
};
class RecordHandle {
public:
	friend class RecordScan;
	~RecordHandle();
	int getRec(const RID &rid, Record& record);
	int insertRec(const char *data, RID& rid);
	bool deleteRec(const RID &rid);
	bool updateRec(const Record &rec);
	bool init(const BufType firstPage, int fileID);
	void forceDisk();
	void writeFH();
	RecordHandle();
	int getFileID();
	void setIndex(int index) {
		this->index = index;
	}
	void setManager(RecordManager* rm) {
		this->recordManager = rm;
		this->fileManager = rm->fileManager;
		this->bufPageManager = rm->bufPageManager;
	}
	int getPageNum() const{
		return this->fileHeader.pageNum;
	}
	int getSlotNum() const {
		return this->fileHeader.recordNum;
	}
	int getRecordSize() const{
		return this->fileHeader.recordSize;
	}
	int getRecordNum() const{
		return this->fileHeader.recordNum;
	}
	int getBitmapStart() const {
		return this->fileHeader.bitmapStart;
	}
	int getBitmapSize() const {
		return this->fileHeader.bitmapSize;
	} 
private:
	BufType getPageContent(int page, int& index);
	RecordManager* recordManager;
	FileManager* fileManager;
	BufPageManager* bufPageManager;
	struct FileHeader fileHeader;
	int fileID, index;
	bool valid;
};
class RecordScan {
public:
	RecordScan() {value = NULL;}
	~RecordScan();
	int openScan(RecordHandle &recordHandle, constSpace::AttrType attrType, int attrLength, int attrOffset, constSpace::CompOp compOp, void *value);
	int getNextRec(Record& record);  
	bool closeScan();
private:
	RecordHandle *recordHandle;
	constSpace::AttrType attrType;
	int attrLength, attrOffset, scanPage, scanSlot, pageNum, slotNum, recordSize, bitmapStart, bitmapSize;
	FileHeader fileHeader;
	void setFileHeader(const FileHeader& fileHeader) {
		this->fileHeader = fileHeader;
	}
	void* value;
	constSpace::CompOp compOp;
	bool(*comparator)(void *, void *, constSpace::AttrType, int);
	int recordNum;
};
#endif
