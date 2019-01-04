#include "ql.h"
#include "../globalHolder.h"
int checkDuplicate(AttrType type, void *value, int offset, int len, RecordHandle &rh) {
    // 没有重复返回0 重复返回1
    RecordScan scan;
    scan.openScan(rh, type, len, offset, EQ_OP, value);
    Record record;
    if (scan.getNextRec(record) == 0) {
        return -1;
    }
    return 0;
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
}
void ColInfo::print(Record *r, ostream &o) {
    char temp[length + 1];
    temp[length] = 0;
    Record *vRecord = &r[tableIndex];
    char *data = vRecord->getData() + offset;
    switch (type) {
        case AttrType::INT:
            o << *(int*)(data);
            break;
        case STRING:
            strncpy(temp, data, length);
            o << temp;
            break;
        case FLOAT:
            o << *(float*)(data);
            break;
        case DATE:
            break;
        default:
            break;
    }
}
void ColInfo::init(int r, AttributeEntry *e) {
    type = e->attrType;
    colName = new char[MAX_ATTRNAME_LENGTH];
    strncpy(colName, e->attrName, MAX_ATTRNAME_LENGTH);
    offset = e->offset;
    length = e->attrLenth;
    tableIndex = r;
}
ColInfo::~ColInfo() {
    if (colName != NULL)
        delete[] colName;
}
IndexStore::IndexStore() {
    isNull = true;
    indexNo = -1;
    data = NULL;
}
void IndexStore::Set(int in) {
    isNull = true;
    indexNo = in;
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
int QL_Manager::Insert(const char *tableName, parser_node *value) {
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
    parser_node *current = value, *n_value;
    RecordHandle &attrInfo = gl_systemManager->attrHandle;
    RecordScan scan;
    if (scan.openScan(gl_systemManager->relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, (void*)tableName) != 0) {
        cout << "QL_Manager::Insert 打开表失败\n";
        return -1;
    }
    Record rc;
    if (scan.getNextRec(rc) != 0) {
        printf("no such table %s\n", tableName);
        return -1;
    }
    int attrcount, entrysize, indexcount, exactcount = 0; 
    RelationEntry *table = (RelationEntry*)rc.getData();
    attrcount = table->attrCount;
    entrysize = table->tupleLength;
    indexcount = table->indexCount;
    RecordScan attrscan;
    if (attrscan.openScan(attrInfo, STRING, MAX_RELNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, (void*)tableName) != 0) {
        cout << "QL_Manager::Insert 打开属性记录失败\n";
        return -1;
    }
    AttributeEntry *attr;
    // 约定1表示空
    int BMsize = (attrcount >> 3) + 1;
    char ValueData[entrysize + BMsize];
    memset(ValueData, 0, sizeof(ValueData));
    char *nullBM = ValueData + entrysize;
    RecordHandle fileHandle;
    if (gl_recordManager->openFile(tableName, fileHandle) != 0) {
        if (gl_recordManager->createFile(tableName, BMsize + entrysize) == false) {
            printf("create %s failed!\n", tableName);
            return -1;
        }
        if (gl_recordManager->openFile(tableName, fileHandle) != 0) {
            printf("open %s failed.\n", tableName);
            return -1;
        }
    }
    int writepos, indexpos = 0;
    IndexStore indexStore[indexcount]; 
    char *ipos;
    int var;
    char *varchar;
    while (current != NULL) {
        exactcount++;
        if (exactcount > attrcount) {
            printf("插入属性过多! %d > %d\n", exactcount, attrcount);
            return -1;
        }
        if (attrscan.getNextRec(rc) != 0) {
            printf("too much attri!\n");
            return -1;
        }
        attr = (AttributeEntry*)rc.getData();
        n_value = current->u.ValueList.value;
        current = current->u.ValueList.next;
        writepos = attr->offset;
        switch (n_value->u.Value.vtype) {
            case v_null:
                if (attr->isPrimarykey) {
                    printf("主键不能为空!\n");
                    return -1;
                }
                if (attr->notNull) {
                    printf("属性 %s 不能为空!\n", attr->attrName);
                    return -1;
                }
                if (attr->indexNo != 0) {
                    indexStore[indexpos++].Set(attr->indexNo);
                }
                setBitmap(exactcount - 1, nullBM, 1);
                break;
            case v_int:
                // 获取数值
                var = n_value->u.Value.value.integer;
                ipos = ValueData + writepos;
                if (attr->attrType != AttrType::INT) {
                    printf("int类型不符\n");
                    return -1;
                }
                if (attr->isPrimarykey) {
                    // 检查是否有重复
                    if (checkDuplicate(AttrType::INT, &var, writepos, sizeof(int), fileHandle) != 0) {
                        printf("主键重复 %s 重复值 %d\n", attr->attrName, var);
                        return -1;
                    }
                }
                if (attr->useForeignkey) {
                    // todo:
                } 
                if (attr->indexNo != 0) {
                    indexStore[indexpos++].Set(ipos, attr->indexNo);
                }
                memcpy(ipos, &var, sizeof(int));
                break;
            case v_string:
                // 获取字符串
                varchar = n_value->u.Value.value.string;
                ipos = ValueData + writepos;
                if (attr->attrType != STRING) {
                    printf("类型不符 string\n");
                    return -1;
                }
                if (attr->isPrimarykey) {
                    // 检查是否有重复
                    if (checkDuplicate(STRING, varchar, writepos, attr->attrLenth, fileHandle) != 0) {
                        printf("主键重复 %s 重复值 %s\n", attr->attrName, varchar);
                        return -1;
                    }
                }
                if (attr->useForeignkey) {
                    // todo:
                }
                if (attr->indexNo != 0) {
                    indexStore[indexpos++].Set(ipos, attr->indexNo);
                } 
                memcpy(ipos, varchar, attr->attrLenth);
                break;
        }
    }
    RID rid;
    if (fileHandle.insertRec(ValueData, rid) != 0) {
        printf("insertRec failed!\n");
        return -1;
    }
    for (int i = 0; i < indexcount; ++i) {
        IX_IndexHandle indexHandle;
        int indexNo = indexStore[i].indexNo;
        bool isNull = indexStore[i].isNull;
        void *pData = (void*)indexStore[i].data;
        if (gl_indexingManager->OpenIndex(tableName, indexNo, indexHandle) != 0) {
            printf("打开索引 %s.%d失败\n", tableName, indexNo);
            return -1;
        }
        if (isNull) {
            if (indexHandle.InsertNull(rid) != 0) {
                printf("插入空值索引失败\n");
                return -1;
            }
        } else {
            if (indexHandle.InsertEntry(pData, rid) != 0) {
                printf("插入非空值索引失败\n");
                return -1;
            }
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
void constructCondition(ConditionInfo *array, int count, parser_node *whereClause, char *tbNames[], int tablecount) {
    parser_node *current = whereClause;
    int pos = 0;
    while (current != NULL) {
        array[pos++].construct(current->u.List.value, tbNames, tablecount);
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
    int r = 0, temp;
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
        current_attr = col->u.Col.colName;
        if (prepareAttr(&tempAttr, current_tb, current_attr, temp) != 0) {
            cout << "读取属性记录失败\n";
            return -1;
        }
        temp = getTableIndex(current_tb, tbNames, tbcount);
        if (temp != 0) {
            cout << "获取属性的表的下标失败\n";
            return -1;
        }
        assert(r < n);
        infos[r].init(temp, &tempAttr);
        colList = colList->u.List.next;
        r++;
    }
    return 0;
}
int constructColInfo(ColInfo *infos, int n, char *tbNames[], int tbcount) {
    // selector is *
    int pos = 0;
    for (int i = 0; i < tbcount; ++i) {
        RecordScan scan;
        if (scan.openScan(gl_systemManager->attrHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, tbNames[i]) != 0) {
            cout << "打开属性记录失败\n" << endl;
            return -1;
        }
        Record attrRec;
        while (scan.getNextRec(attrRec) == 0) {
            AttributeEntry *attr = (AttributeEntry*)(attrRec.getData());
            assert(pos < n);
            infos[pos].init(i, attr);
            pos++;
        }
    }
    return 0;
}
void printColHead(ostream &o, ColInfo *cols, int colNum) {
    for (int i = 0; i < colNum; ++i) {
        o << cols[i].colName;
        if (i != colNum - 1) {
            o << ", ";
        }
    }
    o << endl;
}
void printColInfos(ostream &o, ColInfo *cols, int colNum, Record *r) {
    for (int i = 0; i < colNum; ++i) {
        cols[i].print(r, o);
    }
    o << endl;
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
    int tablecount = 0, conditioncount = 0;
    parser_node *current = tables;
    while (current != NULL) {
        current = current->u.StringList.next;
        tablecount++;
    }
    current = whereClause;
    while (current != NULL) {
        current = current->u.List.next;
        conditioncount++;
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
    constructCondition(conditions, conditioncount, whereClause, tbNames, tablecount);
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
    if (tablecount == 1) {
        // 单表查询 table不变，
        char *tbName = tables->u.StringList.string;
        AttributeEntry *best_attr;
        ConditionInfo *best_cond = guessBestCond(conditions, conditioncount, best_attr);
        if (best_cond) {
            // index
            cout << "最佳筛选条件:" << best_attr->attrName << endl;
            IX_IndexHandle indexhandle;
            if (gl_indexingManager->OpenIndex(tbName, best_attr->indexNo, indexhandle) != 0) {
                return -1;
            }
            indexhandle.printHeadPage();
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
            while (scan.GetNextEntry(rid) == 0) {
                cout << "索引找到一条记录\n";
                if (recordhandle.getRec(rid, records[0]) != 0) {
                    cout << "读取失败\n";
                    return -1;
                }
                if (checkSatisfiedCond(records, conditions, conditioncount)) {
                    // print it
                    printColInfos(cout, printCols, selectNum, records);
                }
            }
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
            while (scan.getNextRec(records[0]) == 0) {
                if (checkSatisfiedCond(records, conditions, conditioncount)) {
                    // print it
                    printColInfos(cout, printCols, selectNum, records);
                }
            }
        }
    } else {
        // 多表连接
        // 找一个具有index的属性
    }
    return 0;
}
int QL_Manager::Delete(const char *tbName, parser_node *whereClause) {
    return 0;
}
int QL_Manager::Update(const char *tbName, parser_node *setClause, parser_node *whereClause) {
    return 0;
}