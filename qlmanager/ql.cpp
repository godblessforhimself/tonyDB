#include "ql.h"
#include "../globalHolder.h"
#define NULL_BIT_VALUE (1)
#define NOT_NULL_BIT_VALUE (0)
int checkDuplicate(AttrType type, void *value, int offset, int len, RecordHandle &rh) {
    // 没有重复返回0 重复返回-1
    int ret = 0, v;
    RecordScan scan;
    Record record;
    if (usePKOptimize) { // 当且仅当主键唯一的int类型
        switch (PKBuffer.getPKBufferState()) {
            case 0:
                if (PKBuffer.checkPKDuplicate(*(int*)value) != 0)
                    return -1;
                else
                    return 0;
            case -1: // 未初始化
                scan.openScan(rh, STRING, 0, 0, NO_OP, NULL);
                while (scan.getNextRec(record) == 0) { // 进行初始化
                    v = *(int*)(record.getData() + offset);
                    PKBuffer.append(v);
                    if (v == *(int*)value)
                        ret = -1;
                }
                return ret;
            case 1:
                break;
        }
    }
    scan.openScan(rh, type, len, offset, EQ_OP, value);
    if (scan.getNextRec(record) == 0) {
        return -1;
    }
    return 0;
}
int getConditionCounts(parser_node *where) {
    assert(where->nType == N_LIST);
    parser_node *current = where;
    int cnt = 0;
    while (current != NULL) {
        cnt++;
        current = current->u.List.next;
    }
    return cnt;
}
QL_Manager::QL_Manager() {
}
QL_Manager::~QL_Manager() {
}
void cleanColInfoArr(ColInfo *head, int len) {
    for (int i = 0; i < len; ++i) {
        if (head[i].colName != NULL)
            delete[] head[i].colName;
    }
}
ColInfo::ColInfo() {
    colName = NULL;
    tbName = NULL;
}
void ColInfo::print(Record *r, ostream &o) {
    char temp[length + 1];
    temp[length] = 0;
    Record *vRecord = &r[tableIndex];
    char *bm = vRecord->getData() + bmoffset;
    bool isNull = (getBitmap(attrIndex, bm) == NULL_BIT_VALUE);
    if (isNull) {
        o << "null";
        // o << "第几项" << attrIndex << "偏移" << bmoffset << endl;
        return;
    }
    char *data = vRecord->getData() + offset;
    switch (type) {
        case AttrType::INT:
            o << *(int*)(data);
            break;
        case DATE:
        case STRING:
            strncpy(temp, data, length);
            o << temp;
            break;
        case FLOAT:
            o << *(float*)(data);
            break;
        default:
            break;
    }
}
void ColInfo::init(int r, AttributeEntry *e, int bm, int attrind) {
    type = e->attrType;
    colName = new char[MAX_ATTRNAME_LENGTH];
    tbName = new char[MAX_RELNAME_LENGTH];
    strncpy(colName, e->attrName, MAX_ATTRNAME_LENGTH);
    strncpy(tbName, e->relName, MAX_RELNAME_LENGTH);
    offset = e->offset;
    length = e->attrLenth;
    tableIndex = r;
    bmoffset = bm;
    attrIndex = attrind;
}
ColInfo::~ColInfo() {
    if (colName != NULL)
        delete[] colName;
    if (tbName != NULL)
        delete[] tbName;
}
IndexStore::IndexStore() {
    isNull = true;
    indexNo = -1;
    data = NULL;
}
void IndexStore::Set(int in) {
    isNull = true;
    indexNo = in;
    data = NULL;
}
void IndexStore::Set(char *d, int in) {
    isNull = false;
    data = d;
    indexNo = in;
}
bool ConditionInfo::Verify(Record *r) {
    // Record r[tablecount]
    // r[i]是table[i]的数据
    void *v1, *v2, *bm;
    int tb;
    switch (type) {
        case ql_NotnullCond:
            tb = cond.nullcond.tb;
            bm = (tb == -1) ? (void*)(r->getData() + cond.nullcond.bmoffset) : (void*)(r[tb].getData() + cond.nullcond.bmoffset);
            return (getBitmap(cond.nullcond.index, bm) == 0);
        case ql_NullCond:
            tb = cond.nullcond.tb;
            bm = (tb == -1) ? (void*)(r->getData() + cond.nullcond.bmoffset) : (void*)(r[tb].getData() + cond.nullcond.bmoffset);
            return (getBitmap(cond.nullcond.index, bm) == 1);
        case ql_NormalCond:
        //  问题 约定使用的值非空
            tb = cond.normalcond.tb1;
            if (cond.normalcond.op == NULL) {
                cout << "check NO OP \n";
                return true;
            }
            if (tb != -1) {
                v1 = (void*)(r[tb].getData() + *(int*)(cond.normalcond.off1));
            } else {
                // 与常量比较
                v1 = cond.normalcond.off1;
            }
            tb = cond.normalcond.tb2;
            if (tb != -1) {
                v2 = (void*)(r[tb].getData() + *(int*)(cond.normalcond.off2));
            } else {
                // 与常量比较
                v2 = cond.normalcond.off2;
            }
            return cond.normalcond.op(v1, v2, cond.normalcond.atype, cond.normalcond.alen);
    }
    return false;
}
void ConditionInfo::setNotNull(int tableIndex, int attrIndex, int bitmapOffset, AttributeEntry &e) {
    type = ql_NotnullCond;
    cond.nullcond.tb = tableIndex;
    cond.nullcond.index = attrIndex;
    cond.nullcond.bmoffset = bitmapOffset;
    cond.nullcond.attr = e;
}
void ConditionInfo::setIsNull(int tableIndex, int attrIndex, int bitmapOffset, AttributeEntry &e) {
    type = ql_NullCond;
    cond.nullcond.tb = tableIndex;
    cond.nullcond.index = attrIndex;
    cond.nullcond.bmoffset = bitmapOffset;
    cond.nullcond.attr = e;
}
void ConditionInfo::setNormal(CompOp op) {
    cond.normalcond.op = getComparator(op);
}
void ConditionInfo::setNormal(int tb1, AttributeEntry &e1) {
    type = ql_NormalCond;
    assert(tb1 >= 0);
    cond.normalcond.tb1 = tb1;
    cond.normalcond.attr1 = e1;
    cond.normalcond.alen = e1.attrLenth;
    cond.normalcond.atype = e1.attrType;
    cond.normalcond.off1 = new int[1]; //记得释放
    *((int*)cond.normalcond.off1) = e1.offset;
}
void ConditionInfo::setNormal(int tb2, void *offorvalue) {
    // 填充偏移量或者value 和tbindex2
    cond.normalcond.tb2 = tb2;
    if (tb2 < 0) {
        // 常量
        cond.normalcond.off2 = offorvalue;
    } else {
        // offorvalue = &offset;
        cond.normalcond.off2 = new int[1];
        *((int*)cond.normalcond.off2) = *(int*)offorvalue;
    }
}
int ConditionInfo::setNormal(AttributeEntry &e2) {
    // 检查两者是否冲突
    cond.normalcond.attr2 = e2;
    if (cond.normalcond.atype != e2.attrType) {
        cout << "类型冲突 ConditionInfo::setNormal(AttributeEntry &e2)\n";
        return -1;
    }
    return 0;
}
int prepareAttr(AttributeEntry *dst, char *tbName, char *aName, int &attrindex) {
    // 遍历attrcat将attribute写入dst 并且设置attrindex为属性下标
    RecordScan rs;
    if (rs.openScan(gl_systemManager->attrHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, tbName) != 0) {
        cout << "prepareAttr failed\n";
        return -1;
    }
    Record rc;
    attrindex = 0;
    while (rs.getNextRec(rc) == 0) {
        AttributeEntry *e = (AttributeEntry*)rc.getData();
        if (strncmp(e->attrName, aName, MAX_ATTRNAME_LENGTH) == 0) {
            *dst = *e;
            return 0;
        }
        attrindex++;
    }
    return -1;
}
int getTableIndex(char *tbName, char *tbNames[], int tablecount) {
    // 计算出这是第几个表
    assert(tbName != NULL);
    for (int i = 0; i < tablecount; ++i) {
        if (strncmp(tbName, tbNames[i], MAX_RELNAME_LENGTH) == 0) {
            return i;
        }
    }
    return -1;
}
int getBMOffset(char *tbName) {
    assert(gl_systemManager->usingDb);
    RecordScan rs;
    if (rs.openScan(gl_systemManager->relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, tbName) != 0) {
        cout << "getBMOffset 失败\n";
        return -1;
    }
    Record rc;
    if (rs.getNextRec(rc) != 0) {
        cout << "表" << tbName << " 不存在\n";
        return -1;
    }
    RelationEntry *tabInfo = (RelationEntry*)rc.getData();
    return tabInfo->tupleLength;
}
int ConditionInfo::construct(parser_node *n_cond, char *tbNames[], int tablecount) {
    // 构造比较体
    assert(n_cond->nType == N_CONDITION || n_cond->nType == N_IS_NULL_COND || n_cond->nType == N_NOT_NULL_COND);
    char *table, *colName;
    parser_node *temp;
    AttributeEntry entry;
    int attrIndex, tbIndex, bmOffset;
    switch (n_cond->nType) {
        case N_CONDITION:
            temp = n_cond->u.Condition.lh;
            assert(temp->nType == N_COL);
            table = (temp->u.Col.tbName == NULL) ? tbNames[0] : temp->u.Col.tbName;
            colName = temp->u.Col.colName;
            if (prepareAttr(&entry, table, colName, attrIndex) != 0) {
                cout << "construct failed\n";
                return -1;
            }
            tbIndex = getTableIndex(table, tbNames, tablecount);
            if (tbIndex == -1) {
                cout << "lh op expr lh can't find table\n";
                return -1;
            }
            setNormal(tbIndex, entry);
            // 设置比较符
            setNormal(n_cond->u.Condition.op);
            // 设置右边
            temp = n_cond->u.Condition.expr;
            assert(temp->nType == N_COL_OR_VALUE);
            if (temp->u.ColOrValue.value != NULL) {
                // 右边是常量
                temp = temp->u.ColOrValue.value;
                assert(temp->nType == N_VALUE);
                if (!twoTypeMatch(entry.attrType, temp->u.Value.vtype)) {
                    cout << "类型不符 ConditionInfo::construct\n";
                    return -1;
                }
                switch (temp->u.Value.vtype) {
                    case v_int:
                        setNormal(-1, &temp->u.Value.value.integer);
                        break;
                    case v_string:
                        setNormal(-1, temp->u.Value.value.string);
                        break;
                    case v_float:
                        setNormal(-1, &temp->u.Value.value.fnumber);
                        break;
                    default:
                        break;
                }
            } else {
                // 右边是tb2.attr2
                temp = temp->u.ColOrValue.col;
                assert(temp->nType == N_COL);
                table = temp->u.Col.tbName;
                colName = temp->u.Col.colName;
                if (table == NULL) {
                    table = tbNames[0];
                }
                if (prepareAttr(&entry, table, colName, attrIndex) != 0) {
                    cout << "查找" << table << "." << colName << "失败\n";
                    return -1;
                }
                tbIndex = getTableIndex(table, tbNames, tablecount);
                if (tbIndex == -1) {
                    cout << "右侧表达式找不到表\n";
                    return -1;
                }
                if (setNormal(entry) != 0) {
                    cout << "右侧表达式和左侧表达式有冲突\n";
                    return -1;
                }
                setNormal(tbIndex, &entry.offset);
            }
            break;
        case N_IS_NULL_COND:
            temp = n_cond->u.IsNullCond.lh; // 类型为Col
            assert(temp->nType == N_COL);
            // 准备表名和属性名
            table = (temp->u.Col.tbName == NULL) ? tbNames[0] : temp->u.Col.tbName;
            colName = temp->u.Col.colName;
            // 准备 tbIndex attrIndex bitmapoffset attributeentry
            if (prepareAttr(&entry, table, colName, attrIndex) != 0) {
                cout << "读取attributeentry失败\n";
                return -1;
            }
            tbIndex = getTableIndex(table, tbNames, tablecount);
            if (tbIndex == -1) {
                cout << "is null条件找不到表\n";
                return -1;
            }
            bmOffset = getBMOffset(table);
            if (bmOffset == -1) {
                return -1;
            }
            setIsNull(tbIndex, attrIndex, bmOffset, entry);
            break;
        case N_NOT_NULL_COND:
            temp = n_cond->u.NotNullCond.lh; // 类型为Col
            assert(temp->nType == N_COL);
            // 准备表名和属性名
            table = (temp->u.Col.tbName == NULL) ? tbNames[0] : temp->u.Col.tbName;
            colName = temp->u.Col.colName;
            // 准备 tbIndex attrIndex bitmapoffset attributeentry
            if (prepareAttr(&entry, table, colName, attrIndex) != 0) {
                cout << "读取attributeentry失败\n";
                return -1;
            }
            tbIndex = getTableIndex(table, tbNames, tablecount);
            if (tbIndex == -1) {
                cout << "not null条件找不到表\n";
                return -1;
            }
            bmOffset = getBMOffset(table);
            if (bmOffset == -1) {
                return -1;
            }
            setNotNull(tbIndex, attrIndex, bmOffset, entry);
            break;
        default:
            cout << "构造条件类型错误\n";
            return -1;
    }
    return 0;
}
ConditionInfo::~ConditionInfo() {
    // offset需要释放
    if (type == ql_NormalCond) {
        if (cond.normalcond.tb1 != -1) 
            delete[] (char*)cond.normalcond.off1;
        if (cond.normalcond.tb2 != -1) 
            delete[] (char*)cond.normalcond.off2;
    }
}
int checkForeignKeyDuplicate(AttributeEntry *attribute, void *value, int attrLen) {
    // 检查属性值是否存在外键表中
    // -1 失败 0 存在 1 不存在
    char *tbName = attribute->foreignTable;
    char *attrN = attribute->foreignName;
    int offSetForeign = -1;
    RecordScan offsetScan;
    Record record;
    if (offsetScan.openScan(gl_systemManager->attrHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, tbName) != 0) {
        return -1;
    }
    AttributeEntry *aentry;
    while (offsetScan.getNextRec(record) == 0) {
        aentry = (AttributeEntry*)record.getData();
        if (strncmp(aentry->attrName, attrN, MAX_ATTRNAME_LENGTH) == 0) {
            offSetForeign = aentry->offset;
            break;
        }
    }
    if (offSetForeign == -1)
        return -1;
    RecordHandle handle;
    if (gl_recordManager->openFile(tbName, handle) != 0) {
        cout << "检查外键约束打开文件失败\n";
        return -1;
    }
    RecordScan scan;
    if (scan.openScan(handle, attribute->attrType, attrLen, offSetForeign, EQ_OP, value) != 0) {
        cout << "检查外键约束打开scan失败\n";
        return -1;
    }
    if (scan.getNextRec(record) == 0) {
        return 0;
    }
    return 1;
}
int QL_Manager::Insert(const char *tableName, parser_node *value, RelationEntry *relationEntry, AttributeEntry *attr, int attrcount, RecordHandle &fileHandle, IX_IndexHandle *indexHandle, int indexcount) {
    // bitmap 
    // 0 正常
    // 1 null
    // 功能:在当前数据库的table里插入一条数据
    // 对于某条属性 
    // 若它非空，检查对应属性的类型，
    // 若是主键
    // 检查数据条目等于表库和属性库的数
    // 若数据库不存在，先创建数据库
    // 插入数据库，index
    // todo 多个主键
    if (!gl_systemManager->usingDb) {
        cout << "database is not defined!\n";
        return -1;
    }
    Record rc;
    int entrysize = relationEntry->tupleLength, exactcount = 0; 
    // 约定1表示空
    int BMsize = (attrcount >> 3) + 1;
    char ValueData[entrysize + BMsize];
    memset(ValueData, 0, sizeof(ValueData));
    char *nullBM = ValueData + entrysize;
    int writepos, indexpos = 0;
    IndexStore indexStore[indexcount]; 
    char *ipos;
    int var;
    char *varchar;
    float vfloat;
    parser_node *current, *n_value;
    current = value;
    int pkvalue;
    while (current != NULL) {
        if (exactcount >= attrcount) {
            printf("插入属性过多! %d > %d\n", exactcount, attrcount);
            return -1;
        }
        n_value = current->u.ValueList.value;
        current = current->u.ValueList.next;
        writepos = attr[exactcount].offset;
        ipos = ValueData + writepos;
        switch (n_value->u.Value.vtype) {
            case v_null:
                if (attr[exactcount].isPrimarykey) {
                    printf("主键不能为空!\n");
                    return -1;
                }
                if (attr[exactcount].notNull) {
                    printf("属性 %s 不能为空!\n", attr[exactcount].attrName);
                    return -1;
                }
                if (attr[exactcount].indexNo != -1) {
                    indexStore[indexpos++].Set(attr[exactcount].indexNo);
                }
                if (checkForeignKey && attr[exactcount].useForeignkey) {
                    // todo:
                } 
                setBitmap(exactcount, nullBM, NULL_BIT_VALUE);
                break;
            case v_int:
                // 获取数值
                var = n_value->u.Value.value.integer;
                if (attr[exactcount].attrType != AttrType::INT) {
                    goto floatentry;
                }
                if (attr[exactcount].isPrimarykey) {
                    // 检查是否有重复
                    if (checkPrimaryKey && checkDuplicate(AttrType::INT, &var, writepos, sizeof(int), fileHandle) != 0) {
                        printf("主键重复 %s 重复值 %d\n", attr[exactcount].attrName, var);
                        return -1;
                    }
                    pkvalue = var;
                }
                if (checkForeignKey && attr[exactcount].useForeignkey) {
                    int result = checkForeignKeyDuplicate(&attr[exactcount], &var, sizeof(int));
                    if (result != 0) {
                        // cout << "外键" << attr[exactcount].foreignName << endl;
                        return -1;
                    }
                } 
                if (attr[exactcount].indexNo != -1) {
                    indexStore[indexpos++].Set(ipos, attr[exactcount].indexNo);
                }
                memcpy(ipos, &var, sizeof(int));
                setBitmap(exactcount, nullBM, NOT_NULL_BIT_VALUE);
                break;
            case v_float:
                // 获取数值
            floatentry:
                vfloat = (n_value->u.Value.vtype == v_float) ? n_value->u.Value.value.fnumber : (float)(n_value->u.Value.value.integer);
                if (attr[exactcount].attrType != FLOAT) {
                    printf("float类型不符\n");
                    return -1;
                }
                if (attr[exactcount].isPrimarykey) {
                    // 检查是否有重复
                    if (checkPrimaryKey && checkDuplicate(FLOAT, &vfloat, writepos, sizeof(float), fileHandle) != 0) {
                        printf("主键重复 %s 重复值 %f\n", attr[exactcount].attrName, vfloat);
                        return -1;
                    }
                }
                if (checkForeignKey && attr[exactcount].useForeignkey) {
                    if (checkForeignKeyDuplicate(&attr[exactcount], &vfloat, sizeof(float)) != 0) {
                        // cout << "外键重复 " << attr[exactcount].attrName << endl;
                        return -1;
                    }
                }
                if (attr[exactcount].indexNo != -1) {
                    indexStore[indexpos++].Set(ipos, attr[exactcount].indexNo);
                }
                memcpy(ipos, &vfloat, sizeof(float));
                setBitmap(exactcount, nullBM, NOT_NULL_BIT_VALUE);
                break;
            case v_string:
                // 获取字符串
                varchar = n_value->u.Value.value.string;
                if (attr[exactcount].attrType != STRING && attr[exactcount].attrType != DATE) {
                    printf("类型不符 string(date)\n");
                    return -1;
                }
                if (attr[exactcount].isPrimarykey) {
                    // 检查是否有重复
                    if (checkPrimaryKey && checkDuplicate(STRING, varchar, writepos, attr[exactcount].attrLenth, fileHandle) != 0) {
                        printf("主键重复 %s 重复值 %s\n", attr[exactcount].attrName, varchar);
                        return -1;
                    }
                }
                if (checkForeignKey && attr[exactcount].useForeignkey) {
                    if (checkForeignKeyDuplicate(&attr[exactcount], varchar, attr[exactcount].attrLenth) != 0) {
                        // cout << "外键重复 " << attr[exactcount].attrName << endl;
                        return -1;
                    }
                }
                if (attr[exactcount].indexNo != -1) {
                    indexStore[indexpos++].Set(ipos, attr[exactcount].indexNo);
                } 
                memcpy(ipos, varchar, attr[exactcount].attrLenth);
                setBitmap(exactcount, nullBM, NOT_NULL_BIT_VALUE);
                break;
        }
        exactcount++;
    }
    RID rid;
    if (fileHandle.insertRec(ValueData, rid) != 0) {
        printf("insertRec failed!\n");
        return -1;
    }
    if (usePKOptimize) {
        PKBuffer.append(pkvalue);
    }
    for (int i = 0; i < indexcount; ++i) {
        int indexNo = indexStore[i].indexNo;
        void *pData = (void*)indexStore[i].data;
        if (indexHandle[i].InsertEntry(pData, rid) != 0) {
            cout << "插入索引" << tableName << "." << indexNo << "失败\n";
            return -1;
        }
    }
    return 0;
}
ConditionInfo *guessBestCond(ConditionInfo *conditions, int count, AttributeEntry *&dst) {
    // 寻找最好的比较条件
    // 排除同等关系和字段比较
    // 有index的属性是主键
    // 主键 与非空值比较 空值
    ConditionInfo *indexnotnull = NULL, *indexnull = NULL;
    bool hasIndex, isPrimary, notNull;
    AttributeEntry *attribute = NULL, *attrnotnull, *attrnull;
    for (int i = 0; i < count; ++i) {
        if (conditions[i].type == ql_NormalCond && conditions[i].cond.normalcond.tb1 >= 0 && conditions[i].cond.normalcond.tb2 >= 0) {
            // 排除同等关系和字段比较
            continue;
        }
        switch (conditions[i].type) {
            case ql_NormalCond:
                // 非空
                notNull = true;
                if (conditions[i].cond.normalcond.tb1 >= 0) {
                    // lh 为属性
                    attribute = &(conditions[i].cond.normalcond.attr1);
                } else {
                    // rh 为属性 不可能
                    attribute = &(conditions[i].cond.normalcond.attr2);
                }
                hasIndex = (attribute->indexNo != -1);
                isPrimary = attribute->isPrimarykey;
                break;
            case ql_NotnullCond:
            case ql_NullCond:
                notNull = false;
                attribute = &(conditions[i].cond.nullcond.attr);
                hasIndex = (attribute->indexNo != -1);
                isPrimary = attribute->isPrimarykey;
                break;
        }
        if (hasIndex && isPrimary) {
            dst = attribute;
            return &conditions[i];
        } else if (hasIndex && notNull) {
            indexnotnull = &conditions[i];
            attrnotnull = attribute;
        } else if (hasIndex) {
            indexnull = &conditions[i];
            attrnull = attribute;
        }
    }
    if (indexnotnull != NULL) {
        dst = attrnotnull;
        return indexnotnull;
    } else {
        dst = attrnull;
        return indexnull;
    }
}
ConditionInfo *findCondWithIndex(ConditionInfo *conditions, int count, AttributeEntry *&dst) {
    ConditionInfo *indexnotnull = NULL, *indexnull = NULL;
    bool hasIndex, isPrimary, notNull;
    AttributeEntry *attribute = NULL, *attrnotnull, *attrnull;
    for (int i = 0; i < count; ++i) {
        if (conditions[i].type == ql_NormalCond && conditions[i].cond.normalcond.tb1 >= 0 && conditions[i].cond.normalcond.tb2 >= 0) {
            // 排除同等关系和字段比较
            continue;
        }
        switch (conditions[i].type) {
            case ql_NormalCond:
                // 非空
                notNull = true;
                if (conditions[i].cond.normalcond.tb1 >= 0) {
                    // lh 为属性
                    attribute = &(conditions[i].cond.normalcond.attr1);
                } else {
                    // rh 为属性 不可能
                    attribute = &(conditions[i].cond.normalcond.attr2);
                }
                hasIndex = (attribute->indexNo != -1);
                isPrimary = attribute->isPrimarykey;
                break;
            case ql_NotnullCond:
            case ql_NullCond:
                notNull = false;
                attribute = &(conditions[i].cond.nullcond.attr);
                hasIndex = (attribute->indexNo != -1);
                isPrimary = attribute->isPrimarykey;
                break;
        }
        if (hasIndex && isPrimary) {
            dst = attribute;
            return &conditions[i];
        } else if (hasIndex && notNull) {
            indexnotnull = &conditions[i];
            attrnotnull = attribute;
        } else if (hasIndex) {
            indexnull = &conditions[i];
            attrnull = attribute;
        }
    }
    if (indexnotnull != NULL) {
        dst = attrnotnull;
        return indexnotnull;
    } else {
        dst = attrnull;
        return indexnull;
    }
}
int constructCondition(ConditionInfo *array, int count, parser_node *whereClause, char *tbNames[], int tablecount) {
    parser_node *current = whereClause;
    int pos = 0;
    while (current != NULL) {
        if (array[pos++].construct(current->u.List.value, tbNames, tablecount) != 0) {
            return -1;
        }
        current = current->u.List.next;
    }
}
bool checkSatisfiedCond(Record *r, ConditionInfo *array, int count) {
    for (int i = 0; i < count; ++i) {
        if (!array[i].Verify(r)) {
            return false;
        }
    }
    return true;
} 
int openScanByCondition(ConditionInfo *condition, IX_IndexHandle *handle, IX_IndexScan *scan) {
    // condition是tb.col op constvalue
    CompOp op;
    void *value;
    switch (condition->type) {
        case ql_NormalCond:
            op = getopByfunc(condition->cond.normalcond.op);
            value = condition->cond.normalcond.off2;
            break;
        case ql_NotnullCond:
            op = NE_OP;
            value = NULL;
            break;
        case ql_NullCond:
            op = EQ_OP;
            value = NULL;
            break;
    }
    if (scan->OpenScan(*handle, op, value) != 0) {
        cout << "openScanByCondition 失败\n";
        return -1;
    }
    return 0;
}
int getSelectorNum(parser_node *selector) {
    // 获取selector的属性数目 0 for *
    int r = 0;
    assert(selector->nType == N_SELECTOR);
    parser_node *colList = selector->u.Selector.colList;
    if (colList == NULL)
        return 0; // select *
    assert(colList->nType == N_LIST);
    while (colList != NULL) {
        r++;
        parser_node *col = colList->u.List.value;
        assert(col->nType == N_COL);
        colList = colList->u.List.next;
    }
    return r;
}
int getAttrNum(char *tbName) {
    // 统计表tbName的属性数目
    assert(gl_systemManager->usingDb);
    RecordScan scan;
    if (scan.openScan(gl_systemManager->attrHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, tbName) != 0) {
        return -1;
    }
    Record record;
    int r = 0;
    while (scan.getNextRec(record) == 0) {
        r++;
    }
    cout << "表" << tbName << "的属性数为" << r << endl;
    return r;
}
int constructColInfo(ColInfo *infos, int n, parser_node *selector, char *tbName, char *tbNames[], int tbcount) {
    // selector非* 若tbName!=NULL说明为单表
    parser_node *colList = selector->u.Selector.colList;
    assert(colList != NULL && colList->nType == N_LIST);
    int r = 0, temp, attrindex;
    char *current_tb, *current_attr;
    AttributeEntry tempAttr;
    while (colList != NULL) {
        parser_node *col = colList->u.List.value;
        assert(col->nType == N_COL);
        current_tb = (col->u.Col.tbName) ? col->u.Col.tbName : ((tbName) ? tbName : NULL);
        if (current_tb == NULL) {
            cout << "多表查询指定tbName\n";
            return -1;
        }
        RecordScan scan;
        if (scan.openScan(gl_systemManager->relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, current_tb) != 0)
            return -1;
        Record record;
        if (scan.getNextRec(record) != 0)
            return -1;
        RelationEntry *relation = (RelationEntry*)record.getData();
        current_attr = col->u.Col.colName;
        if (prepareAttr(&tempAttr, current_tb, current_attr, attrindex) != 0) {
            cout << "读取属性记录失败\n";
            return -1;
        }
        temp = getTableIndex(current_tb, tbNames, tbcount);
        if (temp < 0) {
            cout << "获取属性的表的下标失败\n";
            return -1;
        }
        assert(r < n);
        infos[r].init(temp, &tempAttr, relation->tupleLength, attrindex);
        colList = colList->u.List.next;
        r++;
    }
    return 0;
}
int constructColInfo(ColInfo *infos, int n, char *tbNames[], int tbcount) {
    // selector is *
    int pos = 0;
    for (int i = 0; i < tbcount; ++i) {
        RecordScan tablescan;
        Record record;
        if (tablescan.openScan(gl_systemManager->relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, tbNames[i]) != 0) {
            return -1;
        }
        if (tablescan.getNextRec(record) != 0) {
            return -1;
        }
        RelationEntry *tbinfo = (RelationEntry*)record.getData();
        int bmOffset = tbinfo->tupleLength;
        RecordScan scan;
        if (scan.openScan(gl_systemManager->attrHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, tbNames[i]) != 0) {
            cout << "打开属性记录失败\n" << endl;
            return -1;
        }
        Record attrRec;
        int attrIndex = 0;
        while (scan.getNextRec(attrRec) == 0) {
            AttributeEntry *attr = (AttributeEntry*)(attrRec.getData());
            assert(pos < n);
            infos[pos].init(i, attr, bmOffset, attrIndex);
            pos++;
            attrIndex++;
        }
    }
    return 0;
}
void printColHead(ostream &o, ColInfo *cols, int colNum) {
    for (int i = 0; i < colNum; ++i) {
        o << cols[i].tbName << "." << cols[i].colName;
        if (i != colNum - 1) {
            o << ", ";
        }
    }
    o << endl;
}
void printColInfos(ostream &o, ColInfo *cols, int colNum, Record *r) {
    for (int i = 0; i < colNum; ++i) {
        cols[i].print(r, o);
        if (i != colNum - 1) {
            o << ", ";
        }
    }
    o << endl;
}
ConditionInfo *findNotconcatCon(ConditionInfo *conditions, int count, AttributeEntry *attr) {
    // 从conditions里找出一个非同等关系的条件，并设置好attr
    // 若有多个，优先选择非空值比较
    // 当且仅当全部条件都是同等关系时返回空
    return NULL;
}
bool judgeThenPrint(ostream &o, ColInfo *cols, int colNum, Record *r, ConditionInfo *condition, int conditioncount) {
    if (checkSatisfiedCond(r, condition, conditioncount)) {
        printColInfos(o, cols, colNum, r);
        return true;
    }
    return false;
}
bool hasNextRecordList(Record *records, RecordScan *scan, int tablecount, int tbIndex) {
    // tbIndex 为 -1时
    // 从最右侧开始进1,若溢出则将其重置,并进位,进位成功后将起右侧的初始化
    for (int i = tablecount - 1; i >= 0; --i) {
        if (i == tbIndex)
            --i;
        if (scan[i].getNextRec(records[i]) == 0) {
            // 第i位进位成功,此时要将右侧的每一项初始化
            for (int j = i + 1; j < tablecount; ++j) {
                if (j != tbIndex && scan[j].getNextRec(records[j]) != 0) {
                    // 
                    cout << "要么reset不起作用要么表里一项都没有\n";
                    return false;
                }
            }
            return true;
        } else {
            // 第i位耗尽,重置
            scan[i].reset();
        }    
    }
    // 第0位耗尽 失败
    return false;
}
int calculateDiff(Record *records, AttributeEntry *attrs[]) {
    AttrType type = attrs[0]->attrType;
    assert(type == attrs[1]->attrType);
    char *v[2];
    for (int i = 0; i < 2; ++i) {
        v[i] = records[i].getData() + attrs[i]->offset;
    }
    switch (type) {
        case AttrType::INT:
            return *(int*)v[0] - *(int*)v[1];
        case FLOAT:
            return *(float*)v[0] - *(float*)v[1];
        case DATE:
        case STRING:
            return strncmp(v[0], v[1], attrs[0]->attrLenth);
    }
    return 0;
}
int select_optimize(AttributeEntry *attrs[], ColInfo *cols, int colNum, ConditionInfo *conditions, int conditionCount) {
    // 约定该两属性非空 必有一个主键
    IX_IndexHandle handles[2];
    IX_IndexScan scans[2];
    RecordHandle fileReaders[2];
    Record records[2];
    RID rids[2];
    int withprimarykey = -1;
    for (int i = 0; i < 2; ++i) {
        if (attrs[i]->isPrimarykey) {
            withprimarykey = i;
            break;
        }
    }
    if (withprimarykey == -1)
        return -1;
    for (int i = 0; i < 2; ++i) {
        if (gl_indexingManager->OpenIndex(attrs[i]->relName, attrs[i]->indexNo, handles[i]) != 0) {
            cout << "打开索引失败\n";
            return -1;
        }
        if (scans[i].OpenScan(handles[i], NO_OP, NULL) != 0) {
            cout << "打开索引扫描失败\n";
            return -1;
        }    
        if (gl_recordManager->openFile(attrs[i]->relName, fileReaders[i]) != 0) {
            cout << "打开文件失败" << attrs[i]->relName << endl;
            return -1;
        }
        if (scans[i].GetNextEntry(rids[i]) != 0) {
            cout << "读取不到索引项\n";
            return -1;
        }
        if (fileReaders[i].getRec(rids[i], records[i]) != 0) {
            cout << "读取不到记录\n";
            return -1;
        }
    }
    // 至此已经初始化
    int movingI;
    int validCount = 0;
    printColHead(cout, cols, colNum);
    while (1) {
        int firstMinusSecond = calculateDiff(records, attrs);
        if (firstMinusSecond == 0) {
            if (judgeThenPrint(cout, cols, colNum, records, conditions, conditionCount)) {
                validCount++;
            }
            movingI = 1 - withprimarykey;
        } else if (firstMinusSecond < 0) {
            movingI = 0;
        } else {
            movingI = 1;
        }
        if (scans[movingI].GetNextEntry(rids[movingI]) != 0) {
            cout << "无下一条数据\n";
            break;
        }
        if (fileReaders[movingI].getRec(rids[movingI], records[movingI]) != 0) {
            cout << "索引值和数据值不匹配,可能是增加删除时处理索引出错\n";
            return -1;
        }
    }
    return validCount;
}
int QL_Manager::Select(parser_node *selector, parser_node *tables, parser_node *whereClause) {
    // 分类:单表查询、多表join
    // 单表查询 tables = one
    // 语法检查  检查col里的tableName
    //          忽略col里的tableName
    // 选择一个属性，进行单表查询
    // 多表查询 连接
    // 语法检查 检查col里tableName不能为空
    //          SELECT子句列表中，每个目标列都要加上基表名称
    //          FROM子句应包括所有使用的基表, 每个基表都有一个同等关系
    //          WHERE子句应定义一个同等连接
    // 具有index的同等关系:a.b = c.d
    //          a.b 或 c.d具有索引 假设a.b具有索引
    // 遍历c 以c.d为key调用a.b的indexscan
    // 现在有了record[0] record[1]
    // 再根据其他同等关系使用rmscan
    // 直到recordlist包含所有的表，除非语法'每个基表都有同等关系'非真
    // 判断是否成功
    // 准备输出
    if (!gl_systemManager->usingDb) {
        cout << "database is not defined!\n";
        return -1;
    }
    int tablecount = 0, conditioncount = getConditionCounts(whereClause);
    parser_node *current = tables;
    while (current != NULL) {
        current = current->u.StringList.next;
        tablecount++;
    }
    char *tbNames[tablecount];
    current = tables;
    tablecount = 0;
    while (current != NULL) {
        tbNames[tablecount++] = current->u.StringList.string;
        current = current->u.StringList.next;
    }
    Record records[tablecount];
    ConditionInfo conditions[conditioncount];
    if (constructCondition(conditions, conditioncount, whereClause, tbNames, tablecount) != 0) {
        cout << "语法错误\n";
        return -1;
    }
    //
    bool selectAll = false;
    int selectNum = getSelectorNum(selector);
    if (selectNum < 0) {
        cout << "selectNum " << selectNum << endl;
        return -1;
    }
    if (selectNum == 0) {
        selectAll = true;
        for (int i = 0; i < tablecount; ++i) {
            selectNum += getAttrNum(tbNames[i]);
        }
    }
    ColInfo printCols[selectNum];
    if (selectAll) {
        cout << "选择列为'*',计算出实际列数:" << selectNum << endl;
        if (constructColInfo(printCols, selectNum, tbNames, tablecount) != 0) {
            cout << "构造selector列信息失败\n";
            return -1;
        }
    } else if (tablecount == 1) {
        // 单表查询
        if (constructColInfo(printCols, selectNum, selector, tbNames[0], tbNames, tablecount) != 0) {
            cout << "构造单表查询列信息失败\n";
            return -1;
        }
    } else {
        // 多表查询
        if (constructColInfo(printCols, selectNum, selector, NULL, tbNames, tablecount) != 0) {
            cout << "构造多表查询列信息失败\n";
            return -1;
        }
    }
#define useOptimize (true)
#define NO_OPTIMIZE (0)
#define ONE_INDEX_OPTIMIZE (1)
#define TWO_INDEX_OPTIMIZE (2)
    int optimizeLevel = NO_OPTIMIZE;
    AttributeEntry *optimizeA[2];
    ConditionInfo *currentCondition = NULL;
    if (tablecount == 2 && useOptimize) {
        cout << "表数为2\n";
        for (int i = 0; i < conditioncount; ++i) {
            int currentIndexcount = 0;
            currentCondition = &conditions[i];
            if (currentCondition->type != ql_NormalCond || currentCondition->cond.normalcond.tb1 < 0 || currentCondition->cond.normalcond.tb2 < 0) // 必须是同等关系
                continue;
            if (currentCondition->cond.normalcond.attr1.indexNo >= 0)
                currentIndexcount++;
            if (currentCondition->cond.normalcond.attr2.indexNo >= 0)
                currentIndexcount++;
            if (currentIndexcount == 2) {
                // 直接跳出
                optimizeA[0] = &currentCondition->cond.normalcond.attr1;
                optimizeA[1] = &currentCondition->cond.normalcond.attr2;
                if (optimizeA[0]->isPrimarykey || optimizeA[1]->isPrimarykey)
                    optimizeLevel = TWO_INDEX_OPTIMIZE;
                cout << "具有两个索引\n";
                break;
            }
            if (currentIndexcount == 1) {
                optimizeA[0] = &currentCondition->cond.normalcond.attr1;
                optimizeLevel = ONE_INDEX_OPTIMIZE;
                cout << "具有一个索引\n";
                continue;
            }
        }
    }
    if (tablecount == 1) {
        // 单表查询 table不变，
        char *tbName = tbNames[0];
        AttributeEntry *best_attr;
        ConditionInfo *best_cond = guessBestCond(conditions, conditioncount, best_attr);
        if (best_cond) {
            // index
            cout << "最佳筛选条件:" << best_attr->attrName << endl;
            IX_IndexHandle indexhandle;
            if (gl_indexingManager->OpenIndex(tbName, best_attr->indexNo, indexhandle) != 0) {
                return -1;
            }
            IX_IndexScan scan;
            if (openScanByCondition(best_cond, &indexhandle, &scan) != 0) {
                return -1;
            }
            RID rid;
            RecordHandle recordhandle;
            if (gl_recordManager->openFile(tbName, recordhandle) != 0) {
                return -1;
            }
            printColHead(cout, printCols, selectNum);
            int findresultcount = 0;
            while (scan.GetNextEntry(rid) == 0) {
                // cout << "索引找到一条记录\n";
                if (recordhandle.getRec(rid, records[0]) != 0) {
                    cout << "读取失败\n";
                    return -1;
                }
                if (checkSatisfiedCond(records, conditions, conditioncount)) {
                    // print it
                    printColInfos(cout, printCols, selectNum, records);
                    findresultcount++;
                }
            }
            cout << "共" << findresultcount << "条记录\n";
        } else {
            RecordHandle recordhandle;
            if (gl_recordManager->openFile(tbName, recordhandle) != 0) {
                return -1;
            }
            RecordScan scan;
            if (scan.openScan(recordhandle, STRING, 0, 0, NO_OP, NULL) != 0) {
                return -1;
            }
            printColHead(cout, printCols, selectNum);
            int findresultcount = 0;
            while (scan.getNextRec(records[0]) == 0) {
                if (checkSatisfiedCond(records, conditions, conditioncount)) {
                    // print it
                    printColInfos(cout, printCols, selectNum, records);
                    findresultcount++;
                }
            }
            cout << "共" << findresultcount << "条记录\n";
        }
    } else if (optimizeLevel == TWO_INDEX_OPTIMIZE) {
        int validCount = select_optimize(optimizeA, printCols, selectNum, conditions, conditioncount);
        if (validCount < 0) {
            cout << "检查两个索引的错误\n";
            return -1;
        }
        cout << "当前两表具有两个索引，查询o(m+n)\n";
        cout << "共" << validCount << "条记录\n";
        return 0;
    } else {
        // 多表连接
        // 找一个大条件进行循环：找一个非同等关系
        // 获取一条数据
        // 循环找同等关系
        // 根据该同等关系和当前已有的值，计算出查询值
        // 对每一个查询结果，加入到当前数据中
        // 若属性满了，进行筛选
        ConditionInfo *notConcatCon = NULL;
        AttributeEntry *notConcatAttr = NULL;
        notConcatCon = findNotconcatCon(conditions, conditioncount, notConcatAttr);
        if (notConcatCon) {
            // 存在非同等关系，以之为大循环
            int tbIndex = notConcatCon->getTbIndex();
            if (true /*notConcatAttr->indexNo < 0*/) {
                // 无索引
                RecordHandle fileHandle[tablecount];
                RecordScan scan[tablecount];
                for (int i = 0; i < tablecount; ++i) {
                    if (gl_recordManager->openFile(tbNames[i], fileHandle[i]) != 0) { 
                        cout << "连接多个表有非同等关系无索引打开文件" << tbNames[i] << "失败\n";
                        return -1;
                    }
                    if (scan[i].openScan(fileHandle[i], STRING, 0, 0, NO_OP, NULL) != 0) {
                        cout << "多表连接打开scan失败\n";
                        return -1;
                    }
                }
                printColHead(cout, printCols, selectNum);
                int satisfiedCount = 0;
                while (scan[tbIndex].getNextRec(records[tbIndex]) == 0) {
                    if (notConcatCon->Verify(records)) {
                        // 初始化所有扫描
                        for (int i = 0; i < tablecount; ++i) {
                            if (i != tbIndex) {
                                if (scan[i].getNextRec(records[i]) != 0) {
                                    cout << "表为空，无法初始化一条数据\n";
                                    return -1;
                                }
                            }
                        }
                        // 判断并输出当前记录
                        if (judgeThenPrint(cout, printCols, selectNum, records, conditions, conditioncount))
                            satisfiedCount++;
                        // 生成下一条
                        while (hasNextRecordList(records, scan, tablecount, tbIndex)) {
                            if (judgeThenPrint(cout, printCols, selectNum, records, conditions, conditioncount))
                                satisfiedCount++;
                        }
                    }
                }
                cout << "共" << satisfiedCount << "条记录\n";
            }
        } else {
            // 全是同等关系
            RecordHandle fileHandle[tablecount];
            RecordScan scan[tablecount];
            for (int i = 0; i < tablecount; ++i) {
                if (gl_recordManager->openFile(tbNames[i], fileHandle[i]) != 0) { 
                    cout << "连接多个表无非同等关系打开文件" << tbNames[i] << "失败\n";
                    return -1;
                }
                if (scan[i].openScan(fileHandle[i], STRING, 0, 0, NO_OP, NULL) != 0) {
                    cout << "多表连接打开scan失败\n";
                    return -1;
                }
            }
            printColHead(cout, printCols, selectNum);
            // 初始化所有扫描
            for (int i = 0; i < tablecount; ++i) {
                if (scan[i].getNextRec(records[i]) != 0) {
                    cout << "表为空，无法初始化一条数据\n";
                    return -1;
                }
            }
            // 判断并输出当前记录
            int satisfiedCount = 0;
            if (judgeThenPrint(cout, printCols, selectNum, records, conditions, conditioncount))
                satisfiedCount++;
            while (hasNextRecordList(records, scan, tablecount, -1)) {
                if (judgeThenPrint(cout, printCols, selectNum, records, conditions, conditioncount))
                    satisfiedCount++;
            }
            cout << "共" << satisfiedCount << "条记录\n";
        }
        cout << "无优化\n";
    }
    return 0;
}
int ConditionInfo::getTbIndex() {
    switch (type) {
        case ql_NormalCond:
            return cond.normalcond.tb1;
        case ql_NotnullCond:
        case ql_NullCond:
            return cond.nullcond.tb;
    }
    return -1;
}
int QL_Manager::Delete(char *tbName, parser_node *whereClause) {
    // 暴力删除
    // 处理索引
    if (!gl_systemManager->usingDb) {
        cout << "未指定数据库\n";
        return -1;
    }
    RecordScan tbscan, attrscan;
    Record record;
    if (tbscan.openScan(gl_systemManager->relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, tbName) != 0) {
        cout << "打不开属性表" << tbName << endl;
        return -1;
    }
    int indexcount = 0, bmoffset;
    if (tbscan.getNextRec(record) == 0) {
        RelationEntry *tbinfo = (RelationEntry*)record.getData();
        indexcount = tbinfo->indexCount; // 实际可能比它少
        bmoffset = tbinfo->tupleLength;
    } else {
        cout << "不存在表" << tbName << endl;
        return -1;
    }
    int indexNoList[indexcount]; //打开时候使用
    int attroffset[indexcount]; //删除时的值
    int attrindex[indexcount];  //第几个用于读bitmap
    if (indexcount != 0 && attrscan.openScan(gl_systemManager->attrHandle, STRING, MAX_ATTRNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, tbName) != 0) {
        indexcount = 0;
        AttributeEntry *attrinfo;
        int ic = 0;
        while (attrscan.getNextRec(record) == 0) {
            attrinfo = (AttributeEntry*)record.getData();
            if (attrinfo->indexNo >= 0) {
                indexNoList[indexcount] = attrinfo->indexNo;
                attroffset[indexcount] = attrinfo->offset;
                attrindex[indexcount] = ic;
                indexcount++;
            }
            ic++;
        }
    }
    IX_IndexHandle indexhandle[indexcount];
    for (int i = 0; i < indexcount; ++i) {
        if (gl_indexingManager->OpenIndex(tbName, indexNoList[i], indexhandle[i]) != 0) {
            cout << "打开索引失败" << tbName << "." << indexNoList[i] << endl;
            return -1;
        }
    }
    int conditioncount = getConditionCounts(whereClause);
    ConditionInfo conditions[conditioncount];
    char *tbnames[1];
    tbnames[0] = tbName; 
    constructCondition(conditions, conditioncount, whereClause, tbnames, 1);
    RecordHandle filehandle;
    if (gl_recordManager->openFile(tbName, filehandle) != 0) {
        cout << "打开文件失败" << tbName << endl;
        return -1;
    }
    RecordScan scan;
    if (scan.openScan(filehandle, STRING, 0, 0, NO_OP, NULL) != 0) {
        cout << "打开scan失败\n";
        return -1;
    }
    int deletecount = 0;
    while (scan.getNextRec(record) == 0) {
        if (checkSatisfiedCond(&record, conditions, conditioncount)) {
            if (!filehandle.deleteRec(record.getRID())) {
                cout << "删除记录失败\n";
                return -1;
            } else {
                deletecount++;
                char *value = record.getData(), *bm = value + bmoffset;
                for (int i = 0; i < indexcount; ++i) { // 删除对应每个属性的索引
                    int isNull = getBitmap(attrindex[i], bm);
                    void *delValue;
                    if (isNull) {
                        delValue = NULL;
                    } else {
                        delValue = value + attroffset[i];
                    }
                    if (indexhandle[i].DeleteEntry(delValue, record.getRID()) != 0) {
                        cout << "索引" << indexNoList[i] << "删除记录失败\n";
                        return -1; 
                    }
                }
            }
        }
    }
    cout << tbName << "共删除" << deletecount << "条数据\n";
    return 0;
}
int getSetclauseCount(parser_node *setClause) {
    assert(setClause->nType == N_LIST);
    parser_node *current = setClause, *set;
    int n = 0;
    while (current != NULL) {
        set = current->u.List.value;
        assert(set->nType == N_SET);
        n++;
        current = current->u.List.next;
    }
    return n;
}
int OneSetClause::setValue(parser_node *v) {
    assert(v->nType == N_VALUE);
    switch (v->u.Value.vtype) {
        case v_float:
        substitude_float:
            if (attribute.attrType != FLOAT) {
                cout << "类型不匹配\n";
                return -1;
            }
            value = (v->u.Value.vtype == v_float) ? (void*)&(v->u.Value.value.fnumber) : (void*)&(v->u.Value.value.integer);
            actLength = 4;
            break;
        case v_int:
            if (attribute.attrType != AttrType::INT) { // 也有可能是float
                goto substitude_float;
            }
            value = &(v->u.Value.value.integer);
            actLength = sizeof(int);
            break;
        case v_string://可能是varchar date
            if (attribute.attrType != STRING && attribute.attrType != DATE) {
                cout << "类型不匹配\n";
                return -1;
            }
            value = v->u.Value.value.string;
            actLength = attribute.attrLenth;
            break;
        case v_null:
            if (attribute.isPrimarykey || attribute.notNull) {
                cout << "主键\\非空属性不能为空\n";
                return -1;
            }
            value = NULL;
            actLength = 0;
            break;
    }
    return 0;
}
int constructSingleClause(OneSetClause *dst, parser_node *single, char *tbName) {
    assert(single->nType == N_SET);
    char *colName = single->u.SingleSet.colName;
    RecordScan scan;
    if (scan.openScan(gl_systemManager->attrHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, tbName) != 0) {
        cout << "打开关系表失败" << tbName << endl;
        return -1;
    }
    int n = 0, success = 0;
    Record record;
    AttributeEntry *attr;
    while (scan.getNextRec(record) == 0) {
        attr = (AttributeEntry*)record.getData();
        if (strncmp(colName, attr->attrName, MAX_ATTRNAME_LENGTH) == 0) {
            dst->attribute = *attr;
            dst->attrIndex = n;
            success = 1;
            break;
        }
        n++;
    }
    if (success) {
        if (dst->setValue(single->u.SingleSet.value) != 0) {
            cout << "构造失败\n";
            return -1;
        }
        return 0;
    }
    return -1;
}
int constructSetClauses(OneSetClause* array, int alen, parser_node *setClause, char *tbName) {
    int i = 0;
    parser_node *current = setClause, *single;
    while (current != NULL) {
        assert(current->nType == N_LIST);
        single = current->u.List.value;
        if (constructSingleClause(&array[i], single, tbName) != 0) {
            cout << "构造第" << i << "个更新部分失败\n";
            return -1;
        }
        current = current->u.List.next;
        i++;
    }
    assert(i == alen);
    return 0;
}
int countAttrwithindex(OneSetClause *array, int count) {
    // 统计有索引的属性数目
    int r = 0;
    for (int i = 0; i < count; ++i) {
        if (array[i].attribute.indexNo >= 0)
            r++;
    }
    return r;
}
void setattrpoints(int *attrpoint, int indexcount, OneSetClause *clauseArray, int setclausecount) {
    // 设置指针
    int cp = 0;
    for (int i = 0; i < setclausecount; ++i) {
        if (clauseArray[i].attribute.indexNo >= 0) {
            attrpoint[cp++] = i;
        }
    }
    assert(cp == indexcount);
}
void applyClause(Record &record, OneSetClause &clauseArray, int bmoffset) {
    char *data = record.getData(), *bm = data + bmoffset;
    if (clauseArray.value == NULL) {
        // 改为空值
        setBitmap(clauseArray.attrIndex, bm, 1);
        memset(data + clauseArray.attribute.offset, 0, clauseArray.actLength);
    } else {
        setBitmap(clauseArray.attrIndex, bm, 0);
        memcpy(data + clauseArray.attribute.offset, clauseArray.value, clauseArray.actLength);
    }
}
void applyClauses(Record &record, OneSetClause *clauseArray, int setclausecount, int bmoffset) {
    // 根据更新规则更新record的数据字段
    for (int i = 0; i < setclausecount; ++i) {
        applyClause(record, clauseArray[i], bmoffset);
    }
}
int QL_Manager::Update(char *tbName, parser_node *setClause, parser_node *whereClause) {
    // 暴力更新
    // 更新一条数据 属性a = v
    // 位置，长度，值
    // 处理索引
    if (!gl_systemManager->usingDb) {
        cout << "未指定数据库\n";
        return -1;
    }
    int setclausecount = getSetclauseCount(setClause);
    int indexcount = 0, bmoffset;
    OneSetClause clauseArray[setclausecount];
    RecordScan tbscan, attrscan;
    Record record;
    if (constructSetClauses(clauseArray, setclausecount, setClause, tbName) != 0) {
        cout << "构造set结构失败\n";
        return -1;
    }
    indexcount = countAttrwithindex(clauseArray, setclausecount);   // 统计含有索引的属性数
    int attrpoint[indexcount];
    setattrpoints(attrpoint, indexcount, clauseArray, setclausecount);  // 设置指向具有索引属性的下标
    if (tbscan.openScan(gl_systemManager->relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, tbName) != 0) {
        cout << "打不开属性表" << tbName << endl;
        return -1;
    }
    if (tbscan.getNextRec(record) == 0) {
        RelationEntry *tbinfo = (RelationEntry*)record.getData();
        bmoffset = tbinfo->tupleLength;
    } else {
        cout << "不存在表" << tbName << endl;
        return -1;
    }
    IX_IndexHandle indexhandle[indexcount];
    int indexNo;
    for (int i = 0; i < indexcount; ++i) {
        indexNo = clauseArray[attrpoint[i]].attribute.indexNo;
        if (gl_indexingManager->OpenIndex(tbName, indexNo, indexhandle[i]) != 0) {
            cout << "打开索引失败" << tbName << "." << indexNo << endl;
            return -1;
        }
    }
    // 根据条件进行筛选
    int conditioncount = getConditionCounts(whereClause);
    ConditionInfo conditions[conditioncount];
    char *tbnames[1];
    tbnames[0] = tbName; 
    constructCondition(conditions, conditioncount, whereClause, tbnames, 1);
    RecordHandle filehandle;
    if (gl_recordManager->openFile(tbName, filehandle) != 0) {
        cout << "打开文件失败" << tbName << endl;
        return -1;
    }
    RecordScan scan;
    if (scan.openScan(filehandle, STRING, 0, 0, NO_OP, NULL) != 0) {
        cout << "打开scan失败\n";
        return -1;
    }
    int updatecount = 0;
    OneSetClause *attrwithindex;
    while (scan.getNextRec(record) == 0) {
        if (checkSatisfiedCond(&record, conditions, conditioncount)) {
            char *data = record.getData(), *bm = data + bmoffset;
            for (int i = 0; i < indexcount; ++i) { // 删除对应每个属性的索引 读出原来的value 
                attrwithindex = &clauseArray[attrpoint[i]]; // 具有索引的属性
                bool isNull = (getBitmap(attrwithindex->attrIndex, bm) == 1) ? true : false;
                void *value = (isNull) ? NULL : data + attrwithindex->attribute.offset;
                if (indexhandle[i].DeleteEntry(value, record.getRID()) != 0) {
                    cout << "索引" << attrwithindex->attribute.indexNo << "删除记录失败\n";
                    return -1; 
                }
            }
            // 根据setClause更新数据
            applyClauses(record, clauseArray, setclausecount, bmoffset);
            if (!filehandle.updateRec(record)) {
                cout << "更新记录失败\n";
                return -1;
            } else {
                updatecount++;
                for (int i = 0; i < indexcount; ++i) { // 插入对应每个属性的索引
                    attrwithindex = &clauseArray[attrpoint[i]];
                    if (indexhandle[i].InsertEntry(attrwithindex->value, record.getRID()) != 0) {
                        cout << "插入索引" << attrwithindex->attribute.attrName << "失败\n";
                        return -1;
                    }
                }
            }
        }
    }
    cout << tbName << "共更新" << updatecount << "条数据\n";
    return 0;
}