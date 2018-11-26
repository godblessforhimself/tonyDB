/* const defined in this file */
#ifndef CONST_H
#define CONST_H
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#define PAGESIZE (8000)
#define NULL_NODE (0)
#define MAX_FILENAME_LENGTH (256)
#define MAX_ATTRLENGTH (8096)
#define CopyANDPrint(x) printf("%s: ", x);\
va_list args;\
va_start(args, format);\
vprintf(format, args);\
va_end(args);\
printf("\n")
namespace constSpace {
	enum AttrType {
	    INT,
	    FLOAT,
	    STRING,
	    INVALID
	};
	enum CompOp {
	    NO_OP,                                      // no comparison
	    EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP    // binary atomic operators
	};
}
class RID {
public:
	RID(int p, int s) {
		this->pageIndex = p;
		this->slotIndex = s;
	}
	RID() {
		this->pageIndex = -1;
		this->slotIndex = -1;
	}
	int getPage() const{
		return this->pageIndex;
	}
	int getSlot() const {
		return this->slotIndex;
	}
	void copy(RID rid) {
		this->pageIndex = rid.pageIndex;
		this->slotIndex = rid.slotIndex;
	}
	void set(int p, int s) {
		this->pageIndex = p;
		this->slotIndex = s;
	}
	void show() {
		printf("RID[%d,%d]\n", pageIndex, slotIndex);
	}
	static int comp(const RID& r1, const RID& r2) {
		if (r1.pageIndex < r2.pageIndex) return -1;
		if (r1.pageIndex == r2.pageIndex) {
			if (r1.slotIndex < r2.slotIndex) return -1;
			if (r1.slotIndex == r2.slotIndex) return 0;
			return 1;
		}
		return 1;
	}
private:
	int pageIndex;
	int slotIndex;
};
class Debug {
	static const int DEBUG = 0;
	static const int INFO = 3;
	static const int PROD = 5;
	static const int ERROR = -1;
	static const int level = DEBUG;
public:
	static void debug(const char* format, ...) {
		if (level >= DEBUG) {
			CopyANDPrint("DEBUG");
		}
	}
	static void info(const char* format, ...) {
		if (level >= INFO) {
			CopyANDPrint("INFO");
		}
	}
	static void produce(const char* format, ...) {
		if (level >= PROD) {
			CopyANDPrint("PRODUCE");
		}
	}
	static void error(const char* format, ...) {
		if (level >= ERROR) {
			CopyANDPrint("ERROR");
		}
	}
};
#endif
