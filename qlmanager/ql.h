#ifndef QL_H
#define QL_H
#include "../const.h"
#include "../recordmanager/rm.h"
#include "../indexing/indexing.h"
using namespace constSpace;
struct IndexStore {
    int indexNo;
    char *data;
    bool isNull;
    IndexStore();
    void Set(int);
    void Set(char*, int);
};
struct ColInfo {
    // colName 
    char *tbName;
    char *colName; 
    int offset, bmoffset, attrIndex;
    int length, tableIndex;
    AttrType type;
    ColInfo();
    ~ColInfo();
    void print(Record *r, ostream &o);      // 输出属性值
    void init(int r, AttributeEntry *e, int bm, int attrindex);    // 初始化属性名,偏移,长度,第几项,类型
};
struct OneSetClause {
    AttributeEntry attribute; // 由属性名获取到属性的具体信息,offset
    int attrIndex;         // 第几个属性和位图偏移
    int actLength;          // 属性实际长度, 
    void *value;              // 指向内存中的值 NULL 表示空值
    int setValue(parser_node *v);
};
enum CondType {
    ql_NotnullCond,
    ql_NullCond,
    ql_NormalCond
};
struct ConditionInfo {
    CondType type;
    union {
        struct {
            int tb;
            int bmoffset;
            int index;  // attrindex 用来计算bitmap的位置
            AttributeEntry attr;
        } nullcond;
        struct {
            int tb1, tb2, alen; // tb1 非负 tb2为-1时说明它为常量，此时off2不需要释放, offset需要释放而value不需要释放
            void *off1, *off2;  // off1必定是整数，off2可能是整数(非常量)和指针(常数)       off为自己分配的offset或者pnode栈上的指针
            AttrType atype;
            AttributeEntry attr1, attr2;
            bool (*op)(void* v1, void* v2, AttrType attrtype, int attrLength);
        } normalcond;
    } cond;
    bool Verify(Record *r);
    int getTbIndex();
    ~ConditionInfo();   //off1 off2可能分配了
    void setNotNull(int tableIndex, int attrIndex, int bitmapOffset, AttributeEntry &e1);
    void setIsNull(int tableIndex, int attrIndex, int bitmapOffset, AttributeEntry &e1);
    void setNormal(int tb1, AttributeEntry &e1);
    void setNormal(int tb2, void *offorvalue);
    void setNormal(CompOp op);
    int setNormal(AttributeEntry &e2); 
    int construct(parser_node *cond, char *tb[], int tablecount);
};
class QL_Manager {
public:
    QL_Manager();
    ~QL_Manager();
    int Insert(const char *tbName, parser_node *, RelationEntry *, AttributeEntry *attr, int attrcount, RecordHandle &fileHandle, IX_IndexHandle *indexHandle, int indexcount);
    int Select(parser_node *selector, parser_node *tables, parser_node *whereClause);
    int Delete(char *tbName, parser_node *whereClause);
    int Update(char *tbName, parser_node *setClause, parser_node *whereClause);
private:
    
};
#endif
