/* const defined in this file */
#ifndef CONST_H
#define CONST_H
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/time.h>
#include "filesystem/utils/pagedef.h"
#include "filesystem/fileio/FileManager.h"
#include "filesystem/bufmanager/BufPageManager.h"
#define PKSize ((int)(1e7))
#define PAGESIZE (8000)
#define NULL_NODE (0)
#define MAX_RELNAME_LENGTH (32)
#define MAX_ATTRNAME_LENGTH (32)
#define MAX_FILENAME_LENGTH (256)
#define MAX_ATTRLENGTH (8096)
#define OFFSETOF(s,m) (size_t) &(((s*)0)->m)
#define checkPrimaryKey (true)
#define checkForeignKey (false)
#define usePKBufferOptimize (true)
#define CopyANDPrint(x) printf("%s: ", x);\
va_list args;\
va_start(args, format);\
vprintf(format, args);\
va_end(args);\
printf("\n")
unsigned long timediff(timeval start, timeval end);
namespace constSpace {
	const char attributeCatalogName[] = "attrcat";
	const char relationCatalogName[] = "relcat";
	const char attr_int[] = "int";
	const char attr_float[] = "float";
	const char attr_string[] = "string";
	const char attr_date[] = "date";
	const char attr_invalid[] = "invalid";
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
	bool (*getComparator(CompOp op))(void*, void*, AttrType, int);
	bool equal(void* value1, void* value2, constSpace::AttrType attrtype, int attrLength);
	bool less_than(void* value1, void* value2, constSpace::AttrType attrtype, int attrLength);
	bool greater_than(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength);
	bool less_than_or_eq_to(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength);
	bool greater_than_or_eq_to(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength);
	bool not_equal(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength);
	CompOp getopByfunc(bool (*)(void*, void*, AttrType, int));
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
		int offset; // 真实的地址
		AttrType attrType;
		int attrLenth;	//指传入的括号中的数，在string时有效
		int indexNo;
		bool isPrimarykey;
		bool notNull;
		bool useForeignkey;
		char foreignTable[MAX_RELNAME_LENGTH];
		char foreignName[MAX_ATTRNAME_LENGTH];
	};
	int getattrlength(AttrType);
	void transAttrType(AttrType type, char* dst);
	AttrType getAttrType(char *src);
	typedef enum {
		N_ATTRTYPE,
		N_FIELD,
		N_FIELD_LIST,
		N_STRING_LIST,
		N_VALUE,
		N_VALUE_LIST,
		N_COL_OR_VALUE,
		N_COL,
		N_CONDITION,
		N_IS_NULL_COND,
		N_NOT_NULL_COND,
		N_WHERE,
		N_LIST,
		N_SET,
		N_SELECTOR
	} NodeType;
	enum FieldType {
		f_normal,
		f_notnull,
		f_foreign_key
	};
	enum ValueType {
		v_null,
		v_int,
		v_string, // 对应varchar date
		v_float
	};
	struct parser_node {
		NodeType nType;
		union{
			struct {
				parser_node *colList;
			} Selector;
			struct {
				char *colName;
				parser_node *value;
			} SingleSet;
			struct{
				AttrType type;
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
			/* struct {
				// 指针: normal, notnull, foreignkey, primarylist
				parser_node *fList[MAX_LIST_LENGTH];
				int len;
			} FieldList; */
			struct {
				char *string;
				parser_node *next, *tail;
			} StringList;
			struct {
				ValueType vtype;
				union {
					int integer;
					char *string;
					char *date;
					float fnumber;
				} value;
			} Value;
			struct {
				parser_node *value, *next, *tail;
			} ValueList;
			struct {
				char *tbName;
				char *colName;
			} Col;
			struct {
				parser_node *col;
				parser_node *value;
			} ColOrValue;
			struct {
				parser_node *lh;
				CompOp op;
				parser_node *expr;
			} Condition;
			struct {
				parser_node *lh;
			} IsNullCond;
			struct {
				parser_node *lh;
			} NotNullCond;
			struct {
				parser_node *value, *next, *tail;
			} List;
		} u;
		void set_selector(parser_node *);
		void single_set(char *, parser_node *);
		void set_value();
		void set_value(int);
		void set_value(float);
		void set_value(char*);
		void set_col(char *tb, char *col);
		void col_or_value(parser_node *c, parser_node *v);
		void set_normal_condition(parser_node *l, CompOp o, parser_node *r);
		void set_null_cond(parser_node *l);
		void set_notnull_cond(parser_node *l);
		void init_list(parser_node*);
		void append_list(parser_node *value, parser_node *newpoint);
		void init_value_list(parser_node*);
		void append_value_list(parser_node*, parser_node*);
		void set_attr_type(AttrType, int);
		void set_field_normal(char*, parser_node*);
		void set_field_notnull(char*, parser_node*);
		void set_field_foreign_key(char* loc, char* ftable, char *fname);
		void init_string_list(char *);
		void append_string_list(char *, parser_node *);
		void printValue(ostream &o);
		void printStringList(ostream &o);
		void printValueList(ostream &o);
	};
	bool twoTypeMatch(AttrType, ValueType);
}
struct PrimaryKeyBuffer { // 用于主键重复检查的加速
	int keys[PKSize];
	int stackpos;		// 初始指向0; 随时指向下次插入位置
	void resetPKBuffer();	// 将stackpos置为0
	PrimaryKeyBuffer();		// 将stackpos置为0
	int getPKBufferState();	// 当前状态: 0 正常; -1 未初始化; 1溢出
	void append(int x);		// 添加一项
	int checkPKDuplicate(int value); // 0无重复 -1重复 assert(stackpos != -1)
};
extern bool usePKOptimize;
extern PrimaryKeyBuffer PKBuffer;
struct TickTock{ // 滴答
	clock_t begin;
	void tick();
	double tock();
};
extern TickTock tickTock;
void setBitmap(int, void*, int);
int getBitmap(int, void*);
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
class Record {
public:
	Record() {
		data = NULL;
	}
	~Record() {
		if (data != NULL) {
			delete[] data;
		}
	}
	Record(RID rid, char* data, int recordSize) {
		data = NULL;
		set(rid, data, recordSize);
	}
	void set(RID id, char* data, int recordSize) {
		this->rid.copy(id);
		if (this->data != NULL) {
			delete[] this->data;
		}
		this->data = new char[recordSize];
		memcpy(this->data, data, recordSize);
		this->recordSize = recordSize;
	}
	RID getRID() const{
		return rid;
	}
	char* getData() const{
		return data;
	}
private:
	RID rid;
	char* data;
	int recordSize;
};
class Debug {
	static const int DEBUG = 5;
	static const int INFO = 3;
	static const int PROD = 1;
	static const int ERROR = 7;
	static const int level = PROD;
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
