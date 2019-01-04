#include "sm.h"
SM_Manager::SM_Manager(IX_Manager* ix, RecordManager* rm): indexingManager(ix), recordManager(rm) {
    //打开系统管理模块
    strncpy(pwd, "default", sizeof(pwd));
    usingDb = false;
}
SM_Manager::~SM_Manager() {

}
int SM_Manager::createDb(const char *dbName) {
    char command[80] = "mkdir ";
    if (system(strcat(command, dbName)) < 0) {
        Debug::debug("SM_Manager::createDb create directory fail!");
        return -1;
    }
    if (chdir(dbName) < 0) {
        Debug::debug("SM_Manager::createDb change directory fail!");
        return -1;
    } 
    if (!recordManager->createFile(relationCatalogName, sizeof(struct RelationEntry))) {
        Debug::debug("SM_Manager::createDb create relationCatalog fail!");
        return -1;
    }
    if (!recordManager->createFile(attributeCatalogName, sizeof(struct AttributeEntry))) {
        Debug::debug("SM_Manager::createDb create attributeCatalog fail!");
        return -1;
    }
    chdir("../");
    return 0;
}
int SM_Manager::dropDb(const char *dbName) {
    char command[80] = "rm -r ";
    if (system(strcat(command, dbName)) == -1) {
        Debug::debug("SM_Manager::dropDb failed!");
        return -1;
    }
    return 0;
}
int SM_Manager::openDb(const char *dbName) {
    // 加载两个RecordHandle
    if (dbName == NULL) {
        Debug::debug("SM_Manager::openDb dbName is NULL!");
        return -1;
    }
    if (chdir(dbName) < 0) {
        Debug::debug("SM_Manager::openDb change directory fail!");
        return -1;
    }
    if (recordManager->openFile(relationCatalogName, relationHandle) != 0) {
        Debug::debug("SM_Manager::openDb load relation catalog fail!");
        return -1;
    }
    if (recordManager->openFile(attributeCatalogName, attrHandle) != 0) {
        Debug::debug("SM_Manager::openDb load attribute catalog fail!");
        return -1;
    }
    strncpy(pwd, dbName, sizeof(pwd));
    usingDb = true;
    return 0;
}
int SM_Manager::closeDb() {
    recordManager->closeFile(relationHandle);
    recordManager->closeFile(attrHandle);
    usingDb = false;
    return 0;
}
/*
struct AttrInfo {
    // 解析器传入的属性数组
    char     *attrName;           // Attribute name
    AttrType attrType;            // Type of attribute
    int      attrLength;          // Length of attribute
};
struct RelationEntry {
    // 一个数据表对应一项
    char relName[MAX_RELNAME_LENGTH];
    int tupleLength;
    int attrCount;
    int indexCount;
};
struct AttributeEntry {
    // 一个属性对应一项
    char relName[MAX_RELNAME_LENGTH];
    char attrName[MAX_ATTRNAME_LENGTH];
    int offset;
    AttrType attrType;
    int attrLenth;
    int indexNo;
};
*/
int SM_Manager::printTable(const char *tbName, ostream &o) {
    if (strncmp(pwd, "default", sizeof(pwd)) == 0) {
        return -1;
    } else {
        RecordScan rs;
        if (rs.openScan(relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, (void*)tbName) != 0) {
            Debug::debug("SM_Manager::printTable openScan failed!");
            return -1;
        }
        Record relation, attr;
        Printer p;
        if (rs.getNextRec(relation) == 0) {
            RelationEntry *table = (RelationEntry*)(relation.getData());
            int len = table->attrCount, count = 0;
            RecordScan as;
            if (as.openScan(attrHandle, STRING, MAX_ATTRNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, (void*)tbName) != 0) {
                printf("SM_Manager::printTable fail open attribute file!\n");
                return -1;
            }
            o << "Field, Type, Len, NotNull, hasForeignkey, isPrimarykey" << endl;
            while (as.getNextRec(attr) == 0) {
                count++;
                AttributeEntry* attribute = (AttributeEntry*)(attr.getData());
                p.printAttribute(o, attribute);
                if (count == len)
                    break;
            }
        } else {
            cout << "获取一条记录都失败了\n";
            return -1;
        }
        return 0;
    }
}
int SM_Manager::showTables() {
    if (strncmp(pwd, "default", sizeof(pwd)) == 0) {
        printf("当前无指定数据库\n");
        return -1;
    } else {
        RecordScan rs;
        if (rs.openScan(relationHandle, STRING, 0, 0, NO_OP, NULL) != 0) {
            Debug::debug("SM_Manager::showTables openScan failed!");
            return -1;
        }
        Record relation;
        Printer p;
        while (rs.getNextRec(relation) == 0) {
            p.printRelation(cout, (RelationEntry*)relation.getData());
        }
        return 0;
    }
}
void setNormal(AttributeEntry *dst, parser_node *src) {
    strncpy(dst->attrName, src->u.Field.v.normal.colname, MAX_ATTRNAME_LENGTH);
    parser_node *&type = src->u.Field.v.normal.type;
    dst->attrLenth = type->u.ATTRTYPE.len;
    dst->attrType = (type->u.ATTRTYPE.type);
    dst->indexNo = -1;
    dst->isPrimarykey = false;
    dst->notNull = false;
    dst->useForeignkey = false;
}
void setNotnull(AttributeEntry *dst, parser_node *src) {
    strncpy(dst->attrName, src->u.Field.v.notnull.colname, MAX_ATTRNAME_LENGTH);
    parser_node *&type = src->u.Field.v.notnull.type;
    dst->attrLenth = type->u.ATTRTYPE.len;
    dst->attrType = (type->u.ATTRTYPE.type);
    dst->indexNo = -1;
    dst->isPrimarykey = false;
    dst->notNull = true;
    dst->useForeignkey = false;
}
int setPrimarykey(AttributeEntry *array, int count, parser_node *p) {
    assert(p->nType == N_STRING_LIST);
    parser_node *current = p;
    while (current != NULL) {
        char *&keyname = current->u.StringList.string;
        for (int j = 0; j < count; ++j) {
            if (strncmp(keyname, array[j].attrName, MAX_ATTRNAME_LENGTH) == 0) {
                array[j].isPrimarykey = true;
                break;
            }
            if (j == count - 1) 
            // primarykey 有一项不存在
                return -1;
        }
        current = current->u.StringList.next;
    }
    return 0;
}
int setForeignkey(AttributeEntry *array, int count, parser_node *p) {
    char *&key = p->u.Field.v.foreignkey.localname;
    for (int i = 0; i < count; ++i) {
        if (strncmp(array[i].attrName, key, MAX_ATTRNAME_LENGTH) == 0) {
            array[i].useForeignkey = true;
            strncpy(array[i].foreignTable, p->u.Field.v.foreignkey.foreigntable, MAX_RELNAME_LENGTH);
            strncpy(array[i].foreignName, p->u.Field.v.foreignkey.foreignname, MAX_ATTRNAME_LENGTH);
            return 0;
        }
    }
    return -1;
}
int SM_Manager::createTable(const char *tbName, parser_node *fieldList) {
    assert(fieldList->nType == N_LIST);
    if (strncmp(pwd, "default", sizeof(pwd)) == 0) {
        printf("SM_Manager::createTable db not defined!\n");
        return -1;
    }
    int attrCount = 0, offset = 0, len = 0;
    parser_node *current = fieldList;
    while (current != NULL) {
        len++;
        current = current->u.List.next;
    }
    AttributeEntry entrys[len];
    int typeLen;
    current = fieldList;
    while (current != NULL) {
        parser_node *iter = current->u.List.value;
        parser_node *type;
        assert(iter->nType == N_STRING_LIST || iter->nType == N_FIELD);
        switch (iter->nType) {
            case N_STRING_LIST:
                if (setPrimarykey(entrys, attrCount, iter) != 0) {
                    printf("SM_Manager::createTable not find primary key\n");
                    return -1;
                }
                break;
            case N_FIELD:
                switch (iter->u.Field.ftype) {
                    case f_normal:
                        type = iter->u.Field.v.normal.type;
                        typeLen = (type->u.ATTRTYPE.type == STRING) ? type->u.ATTRTYPE.len : constSpace::getattrlength(type->u.ATTRTYPE.type);
                        entrys[attrCount].offset = offset;
                        offset += typeLen;
                        strncpy(entrys[attrCount].relName, tbName, MAX_RELNAME_LENGTH);
                        setNormal(&entrys[attrCount++], iter);
                        break;
                    case f_notnull:
                        type = iter->u.Field.v.normal.type;
                        typeLen = (type->u.ATTRTYPE.type == STRING) ? type->u.ATTRTYPE.len : constSpace::getattrlength(type->u.ATTRTYPE.type);
                        entrys[attrCount].offset = offset;
                        offset += typeLen;
                        strncpy(entrys[attrCount].relName, tbName, MAX_RELNAME_LENGTH);
                        setNotnull(&entrys[attrCount++], iter);
                        break;
                    case f_foreign_key:
                        if (setForeignkey(entrys, attrCount, iter) != 0) {
                            printf("SM_Manager::createTable not find foreign key\n");
                            return -1;
                        }
                        break;
                }
                break;
        }
        current = current->u.List.next;
    }
    RelationEntry relationEntry;
    strncpy(relationEntry.relName, tbName, MAX_RELNAME_LENGTH);
    relationEntry.indexCount = 0;
    relationEntry.attrCount = attrCount;
    relationEntry.tupleLength = offset;
    RID rid;
    for (int i = 0; i < attrCount; i++) {
        if (attrHandle.insertRec((char*)&entrys[i], rid)) {
            Debug::debug("SM_Manager::createTable attrHandle.insertRec fail!");
            return -1;
        }
    }
    if (relationHandle.insertRec((char*)&relationEntry, rid)) {
        Debug::debug("SM_Manager::createTable relationHandle.insertRec fail!");
        return -1;
    }
    relationHandle.forceDisk();
    attrHandle.forceDisk();
    return 0;
}
int SM_Manager::createIndex(const char *table, const char *attrName) {
    // 为属性创建索引
    // 只要维护 RelationEntry.indexCount AttributeEntry.indexNo 
    // 并创建索引即可
    // 隐藏问题，有可能已经存在索引
    RecordScan recordScan;
    if (recordScan.openScan(relationHandle, STRING, MAX_RELNAME_LENGTH, 0, EQ_OP, (void*)table)) {
        Debug::debug("SM_Manager::createIndex recordScan.openScan fail!");
        return -1;
    }
    // 查询关系表
    Record record;
    // 查找relation
    if (recordScan.getNextRec(record) != 0) {
        cout << "找不到表" << table << endl;
        return -1;
    }
    RelationEntry *tbinfo = (RelationEntry*)record.getData();
    int count = tbinfo->indexCount;
    int tupleLength = tbinfo->tupleLength;
    tbinfo->indexCount++;
    if (relationHandle.updateRec(record) == false) {
        cout << "更新表的indexcount失败\n";
        return -1;
    }
    // 查找attribute
    RecordScan scan;
    if (scan.openScan(attrHandle, STRING, MAX_ATTRNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, (void*)table) != 0) {
        Debug::debug("SM_Manager::createIndex 打不开", attrName);
        return -1;
    }
    int attrindex = 0; // 属性的下标
    while (scan.getNextRec(record) == 0) {
        AttributeEntry *attrinfo = (AttributeEntry*)record.getData();
        if (strncmp(attrinfo->attrName, attrName, MAX_ATTRNAME_LENGTH) == 0) {
            if (attrinfo->indexNo >= 0) {
                cout << "不能创建已有索引\n";
                return -1;
            }
            int realLen = (attrinfo->attrType == STRING) ? attrinfo->attrLenth : getattrlength(attrinfo->attrType);
            if (indexingManager->CreateIndex(table, count, attrinfo->attrType, realLen) != 0) {
                cout << "创建索引失败\n";
                return -1;
            }
            attrinfo->indexNo = count;
            attrHandle.updateRec(record);
            IX_IndexHandle handle;
            if (indexingManager->OpenIndex(table, count, handle) != 0) {
                cout << "插入索引失败\n";
                return -1;
            }
            RecordHandle fileHandle;
            if (recordManager->openFile(table, fileHandle) != 0) {
                cout << "打不开文件" << table << endl;
                return -1;
            }
            RecordScan fileScan;
            if (fileScan.openScan(fileHandle, STRING, 0, 0, NO_OP, NULL) != 0) {
                cout << "打不开scan\n";
                return -1;
            }
            Record rc;
            while (fileScan.getNextRec(rc) == 0) {
                char *data = rc.getData();
                char *bm = data + tupleLength;
                int isNull = getBitmap(attrindex, (void*)bm);
                void *pData;
                if (isNull == 1) 
                    pData = NULL;
                else
                    pData = data + attrinfo->offset;
                if (handle.InsertEntry(pData, rc.getRID()) != 0) {
                    cout << "插入索引值失败\n";
                    return -1;
                }
            }
            relationHandle.forceDisk();
            attrHandle.forceDisk();
            return 0;
        }
        attrindex ++;
    }
    return -1;
}
int SM_Manager::dropTable(const char *relname) {
    // 删除RelationEntry里的relName
    // 删除AttributeEntry里relName字段相同的
    // 删除有index的attribute的index
    RecordScan relationScan;
    relationScan.openScan(relationHandle, STRING, MAX_RELNAME_LENGTH, OFFSETOF(RelationEntry, relName), EQ_OP, (void*)relname);
    Record relationRec;
    while (relationScan.getNextRec(relationRec) == 0) {
        relationHandle.deleteRec(relationRec.getRID());
    }
    RecordScan attributeScan;
    attributeScan.openScan(attrHandle, STRING, MAX_ATTRNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, (void*)relname);
    Record attributeRec;
    while (attributeScan.getNextRec(attributeRec) == 0) {
        AttributeEntry* entry = (AttributeEntry*)attributeRec.getData();
        int index = entry->indexNo;
        if (index >= 0) {
            indexingManager->DestroyIndex(relname, index);
        }
        attrHandle.deleteRec(attributeRec.getRID());
    }
    return 0;
}
int SM_Manager::dropIndex(const char *relname, const char *attrname) {
    // 检查index是否存在
    // 维护 RelationEntry.indexCount AttributeEntry.indexNo 
    // 删除index文件
    RecordScan scan;
    if (scan.openScan(attrHandle, STRING, MAX_ATTRNAME_LENGTH, OFFSETOF(AttributeEntry, relName), EQ_OP, (void*)relname) != 0) {
        cout << "无法打开属性文件\n";
        return -1;
    }
    Record record;
    while (scan.getNextRec(record) == 0) {
        AttributeEntry *attrinfo = (AttributeEntry*)record.getData();
        if (strncmp(attrinfo->attrName, attrname, MAX_ATTRNAME_LENGTH) == 0) {
            int index = attrinfo->indexNo;
            if (index < 0) {
                cout << "索引不存在\n";
                return -1;
            }
            attrinfo->indexNo = -1;
            if (attrHandle.updateRec(record) == false) {
                cout << "更新索引失败\n";
                return -1;
            }
            if (indexingManager->DestroyIndex(relname, index) != 0) {
                cout << "删除索引失败\n";
                return -1;
            }
            return 0;
        }
    }
    return -1;
}