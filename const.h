/* const defined in this file */
#ifndef CONST_H
#define CONST_H
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "filesystem/utils/pagedef.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/bufmanager/BufPageManager.h"
#define PAGESIZE (8000)
#define MAX_LIST_LENGTH (16)
#define NULL_NODE (0)
#define MAX_RELNAME_LENGTH (32)
#define MAX_ATTRNAME_LENGTH (32)
#define MAX_FILENAME_LENGTH (256)
#define MAX_ATTRLENGTH (8096)
#define CopyANDPrint(x) printf("%s: ", x);\
va_list args;\
va_start(args, format);\
vprintf(format, args);\
va_end(args);\
printf("\n")
namespace constSpace {
	const char attributeCatalogName[] = "attrcat";
	const char relationCatalogName[] = "relcat";
	enum AttrType {
	    INT,
	    FLOAT,
	    STRING,
		DATE,
	    INVALID
	};
	enum CompOp {
	    NO_OP,                                      // no comparison
	    EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP    // binary atomic operators
	};
	bool equal(void* value1, void* value2, constSpace::AttrType attrtype, int attrLength);
	bool less_than(void* value1, void* value2, constSpace::AttrType attrtype, int attrLength);
	bool greater_than(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength);
	bool less_than_or_eq_to(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength);
	bool greater_than_or_eq_to(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength);
	bool not_equal(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength);
	struct AttrInfo {
		// 解析器传入的属性数组
		char     *attrName;           // Attribute name
		AttrType attrType;            // Type of attribute
		int      attrLength;          // Length of attribute
	};
	struct RelationEntry {
		// 一个数据表对应一项
		char relName[MAX_RELNAME_LENGTH];
		int tupleLength;	// 占用的字节
		int attrCount;		// 属性数
		int indexCount;		// 索引数
	};
	struct AttributeEntry {
		// 一个属性对应一项
		char relName[MAX_RELNAME_LENGTH];
		char attrName[MAX_ATTRNAME_LENGTH];
		int offset;
		AttrType attrType;
		int attrLenth;
		int indexNo;
		bool isPrimarykey;
		bool notNull;
		bool useForeignkey;
		char foreignTable[MAX_RELNAME_LENGTH];
		char foreignName[MAX_ATTRNAME_LENGTH];
	};
	int getattrlength(AttrType);
	void transAttrType(AttrType type, char* dst);
	typedef enum {
		N_ATTRTYPE,
		N_FIELD,
		N_FIELD_LIST,
		N_FIELD_PRIMARY_KEY_LIST,
		N_STRING_LIST
	} NodeType;
	enum FieldType {
		f_normal,
		f_notnull,
		f_foreign_key
	};
	struct parser_node {
		NodeType nType;
		union{
			struct{
				AttrType* type;
				int len;
			} ATTRTYPE;
			struct {
				FieldType ftype;
				union {
					struct {
						char* colname;
						parser_node* type;
					} normal;
					struct {
						// 非空
						char* colname;
						parser_node* type;
					} notnull;
					struct {
						// 对应于外键
						char* localname;
						char* foreigntable;
						char* foreignname;
					} foreignkey;
				} v;
			} Field;
			struct {
				// 指针: normal, notnull, foreignkey, primarylist
				parser_node *fList[MAX_LIST_LENGTH];
				int len;
			} FieldList;
			struct {
				char* stringlist[MAX_LIST_LENGTH];
				int len;
			} StringList;
		} u;
		void set_attr_type(AttrType*, int);
		void set_field_normal(char*, parser_node*);
		void set_field_notnull(char*, parser_node*);
		void set_field_foreign_key(char* loc, char* ftable, char *fname);
		void init_field_list();
		void field_list_append(parser_node*);
		void init_string_list();
		void append_string_list(char *);
		void set_field_primary_list();
		void print(ostream& o);
		void printStringList(ostream &o);
	};
}
class Printer {
public: 
 	Printer();
    ~Printer();         
    void printRelation(ostream &c, constSpace::RelationEntry* entry);
	void printAttribute(ostream &c, constSpace::AttributeEntry* entry);
};
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
	void show() const {
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
	static const int BTREE_REMOVE = 0;
	static const int BTREE_INSERT = BTREE_REMOVE + 1;
	static const int BTREE_PRINT_DATA_DETAIL = BTREE_INSERT + 1;
	static const int BTREE_LINK_DETAIL = BTREE_PRINT_DATA_DETAIL + 1;
	static const int key = -1;
	static void print(int keyword, const char* format, ...) {
		if (key == keyword) {
			va_list args;
			va_start(args, format);
			vprintf(format, args);
			va_end(args);
			printf("\n");
		}
	}
	static void debugL(const char* format, ...) {
		if (level >= DEBUG) {
			va_list args;
			va_start(args, format);
			vprintf(format, args);
			va_end(args);
		}
	}
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
