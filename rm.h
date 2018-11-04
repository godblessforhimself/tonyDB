#include "../filesystem/bufmanager/BufPageManager.h"
#include "../filesystem/fileio/FileManager.h"
#include "../filesystem/utils/pagedef.h"
#define PAGESIZE (8000)
class RID {
public:
	RID(int p, int s) {
		this.pageIndex = p;
		this.slotIndex = s;
	}
	RID() {
		this.pageIndex = -1;
		this.slotIndex = -1;
	}
	int getPage() {
		return this.pageIndex;
	}
	int getSlot() {
		return this.slotIndex;
	}
	void copy(RID rid) {
		this.pageIndex = rid.pageIndex;
		this.slotIndex = rid.slotIndex;
	}
private:
	int pageIndex;
	int slotIndex;
};
struct FileHeader {
	int recordSize;		
	int recordNum;		// every page record limit number
	int pageNum;		// include fileheader page,[1,+)
	int next;			// next free page index, init value is 1
	int bitmapStart;	// the offset of bitmap in page
	int bitmapSize;		// the size of bitmap
	int recordStart;	// the offset of record
};
void initFH(BufType fp, int rs);
/*
content page
int prev,next,count;
char[] bitmap;
char[] records;
*/
static class PageReader {
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
		memset((void*)page, 0, PAGESIZE)；
		setPrev(page, 0);
		setNext(page, index + 1);
		setCount(page, 0);
	}
	/* read state of record i;
		change state of record i;
		*/
	static bool getBitmap(BufType p, int bitmapStart, int slot) {
		char states = *((char*)p + bitmapStart + slot >> 3);
		if (states & (1 << (slot & 7)))
			return true;
		return false;
	}
	static void setBitmap(BufType p, int slot, bool used) {
		char* data = ((char*)p + (bitmapStart + slot >> 3));
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
			if (~bitmap[ptr]) {
				break;
			}
			if (ptr >= bitmapSize) {
				return -1;
			}
			ptr++;
		}
		for (int i = 0; i < 7; ++i) {
			if ((bitmap[ptr] & (1 << i)) == 0) {
				return i + 8 * ptr;
			}
		}
		return -2;
	}
	static int getUsedSlot(BufType p, int pslot, int bitmapStart, int bitmapSize) {
		/* if 8bit is 0, */
		char* bitmap = ((char*)p + bitmapStart);
		int ptr = pslot >> 3;
		while (1) {
			while (1) {
				if (bitmap[ptr]) {
					break;
				}
				if (ptr >= bitmapSize) {
					return -1;
				}
				ptr++;
			}
			for (int i = 0; i < 7; ++i) {
				if ((bitmap[ptr] & (1 << i))) {
					int slot = i + 8 * ptr;
					if (slot >= pslot)
						return slot;
				}
			}
			ptr++;
		}
	}
	static char* getRPtr(BufType p, int slot, int recordStart, int recordSize)
	static void writeRecord(BufType p, int recordStart, int recordSize, int slot, char* data) {
		char* dest = ((char*)p + (recordStart + recordSize * slot));
		memcpy(dest, data, recordSize);
	}
}
class Record {
public:
	Record() {}
	Record(RID rid, char* data, int recordSize) {
		this.rid.copy(rid);
		this.data = data;
		this.recordSize = recordSize;
	}
	void set(RID id, char* data, int recordSize) {
		this.rid.copy(rid);
		this.data = data;
		this.recordSize = recordSize;
	}
	RID& getRID() {
		return this->rid;
	}
	char* getData() {
		return this->data;
	}
private:
	RID rid;
	char* data;
	int recordSize;
};
class RecordManager {
public:
	RecordManager();
	~RecordManager();
	friend class RecordHandle;
	bool createFile(const char* filename, int recordSize);
	bool destroyFile(const char* filename);
	RecordHandle& openFile(const char* filename);
	bool closeFile(RecordHandle& recordHandle);
private:
	FileManager* fileManager;
	BufPageManager* bufPageManager;
}
class RecordHandle {
public:
	int getRec(const RID &rid, Record& record) const;
	RID &insertRec(const char *data);
	bool deleteRec(const RID &rid);
	bool updateRec(const Record &rec);
	bool init(const BufType firstPage, int fileID);
	RecordHandle();
	bool isValid();
	int getFileID();
	int setIndex(int index) {
		this->index = index
	}
	void setManager(const RecordManager* rm) {
		this->recordManager = rm;
		this->fileManager = rm->fileManager;
		this->bufPageManager = rm->bufPageManager;
	}
	int getPageNum() {
		return this->fileHeader.pageNum;
	}
	int getSlotNum() {
		return this->fileHeader.recordNum;
	}
	int getRecordSize() {
		return this->fileHeader.recordSize;
	}
	int getRecordNum() {
		return this->fileHeader.recordNum;
	}
private:
	BufType& getPageContent(int page, int& index) {
		BufType b = this->bufPageManager.getPage(this->fileID, page, index);
		return b;
	}
	RecordManager* recordManager;
	FileManager* fileManager;
	BufPageManager* bufPageManager;
	struct FileHeader fileHeader;
	int fileID, index;
	bool valid;
}
class RecordScan {
public:
	RecordScan openScan(const RecordHandle &recordHandle, AttrType attrType, int attrLength, int attrOffset, CompOp compOp, void *value);
	int getNextRec(Record& record);  
	bool closeScan();
private:
	RecordHandle& recordHandle;
	AttrType attrType;
	int attrLength, attrOffset, scanPage, scanSlot, pageNum, slotNum;
	CompOp compOp;
	void* value;
	void* comparator;
	bool isValid;
	int recordNum;
	void moveAhead();
}