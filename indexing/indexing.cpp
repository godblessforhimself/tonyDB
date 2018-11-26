#include "indexing.h"
/* private function */
#define dataSize (*data_size)
#define childSize (*child_size)
char data_global[MAX_ATTRLENGTH];
RID rid_global;
void setFileName(char* dest, const char* fileName, int indexNo) {
    memset(dest, 0, MAX_FILENAME_LENGTH);
    int len = strlen(fileName);
    memcpy(dest, fileName, len);
    int i = 0;
    while (indexNo >= 0) {
        dest[i + len] = '0' + indexNo % 10;
        indexNo = indexNo / 10;
        i++;
        if (indexNo == 0)
            break;
    }
}
BTNode::BTNode() {
    parent = NULL; data_size = NULL; child_size = NULL;
    values = NULL; rids = NULL; childs = NULL;
    entryLimit = 0; attrLength = 0;
    attrType = INVALID;
}
int BTNode::comp(char* v1, char* v2) {
    switch (attrType) {
        case AttrType::INT:
            return *((int*)v1) - *((int*)v2);
        case AttrType::FLOAT:
            if (*((float*)v1) < *((float*)v1)) 
                return -1;
            if (*((float*)v1) == *((float*)v1))
                return 0;
            return 1;
        case AttrType::STRING:
            return strncmp(v1, v2, attrLength);
        case AttrType::INVALID:
            Debug::produce("BTNode comp invalid type");
            return 0;
    }
}
int BTNode::search(char* data, const RID& rid) {
    // [0, size] -1
    if (dataSize == 0) return -1;
    int l = 0, r = dataSize;
    while (l < r) {
        int m = (l + r) >> 1;
        int diff_data = comp(data, values + attrLength * m);
        int diff_rid = RID::comp(rid, rids[m]);
        int diff;
        if (diff_data < 0 || (diff_data == 0 && diff_rid < 0)) {
            diff = -1;
        } else if (diff_data == 0 && diff_rid == 0) {
            diff = 0;
        } else if (diff_data > 0 || (diff_data == 0 && diff_rid > 0)) {
            diff = 1;
        }
        if (diff < 0) {
            r = m;
        } else {
            l = m + 1;
        }
    }
    return --l;
}
bool BTNode::equals(int dataindex, char* data, const RID& rid) {
    // dataindex [0, dataSize)
    if (dataindex < 0 || dataindex >= dataSize) {
        Debug::error("BTNode::equals [%d, %d]", dataindex, dataSize);
        return false;
    }
    char* value = values + dataindex * attrLength;
    if (comp(data, value) == 0 && RID::comp(rids[dataindex], rid) == 0)
        return true;
    return false;
}
void BTNode::setChild(int rank, int pageID) {
    if (rank >= 0 && rank < childSize) {
        childs[rank] = pageID;
        return;
    }
    Debug::debug("setChild rank [%d, %d]", rank, childSize);
}
int BTNode::getChild(int child) {
    // child valid [0, size)
    if (child >= 0 && child < childSize)
        return childs[child];
    return NULL_NODE;
}
void BTNode::getData(int rank, char* data, RID& rid) {
    if (rank < 0 || rank >= dataSize) {
        Debug::error("getData range exceeds %d, %d", rank, dataSize);
        return;
    }
    char* value = values + rank * attrLength;
    memcpy(data, value, attrLength);
    rid.copy(rids[rank]);
}
void BTNode::setData(int rank, char* data, RID& rid) {
    if (rank < 0 || rank >= dataSize) {
        Debug::error("setData range exceeds %d, %d", rank, dataSize);
        return;
    }
    char* value = values + rank * attrLength;
    memcpy(value, data, attrLength);
    rids[rank].copy(rid);
}
void BTNode::removeData(int rank) {
    // copy (rank, size) to [rank, size - 1)
    int len = (dataSize - 1 - rank);
    if (len < 0 || rank < 0) {
        Debug::error("removeData rank [%d, %d]", rank, dataSize);
        return;
    }
    int offset = rank * attrLength;
    void* dest = (void*)(values + offset);
    void* src = (void*)(values + (offset + attrLength));
    memmove(dest, src, len * attrLength);
    for (int i = rank; i < dataSize - 1; i++) {
        rids[i].copy(rids[i + 1]);
    }
    dataSize--;
}
void BTNode::removeData(int rank, char* data, RID& rid) {
    int len = (dataSize - 1 - rank);
    if (len < 0 || rank < 0) {
        Debug::error("removeData (3param) rank [%d, %d]", rank, dataSize);
        return;
    }
    int offset = rank * attrLength;
    void* dest = (void*)(values + offset);
    void* src = (void*)(values + (offset + attrLength));
    memcpy(data, dest, attrLength);
    rid.copy(rids[rank]);
    memmove(dest, src, len * attrLength);
    for (int i = rank; i < dataSize - 1; i++) {
        rids[i].copy(rids[i + 1]);
    }
    dataSize--;
}
int BTNode::removeChild(int rank) {
    if (rank < 0 || rank >= childSize) {
        Debug::error("removeChild rank [%d, %d]", rank, childSize);
        return NULL_NODE;
    }
    // (rank, childSize) 
    int ret = childs[rank];
    memmove((void*)&childs[rank], (void*)childs[rank + 1], (childSize - 1 - rank) * sizeof(int));
    childSize--;
    return ret;
}
void BTNode::insertData(int rank, char* data, const RID& rid) {
    // rank [0, dataSize]
    if (rank > dataSize || rank < 0) {
        Debug::error("insertData rank [%d, %d]", rank, dataSize);
        return;
    }
    if (rank == dataSize) {
        memcpy(values + rank * attrLength, data, attrLength);
        rids[rank].copy(rid);
        dataSize++;
        return;
    }
    // [rank, dataSize) -> (rank, dataSize]
    memmove((void*)(values + (rank + 1) * attrLength), (void*)(values + rank * attrLength), (dataSize - rank) * attrLength);
    for (int i = dataSize; i > rank; i--) {
        rids[i].copy(rids[i - 1]);
    }
    memcpy((void*)(values + rank * attrLength), data, attrLength);
    rids[rank].copy(rid);
    dataSize++;
}
void BTNode::insertChild(int rank, int id) {
    // rank [0, childSize]
    if (rank > childSize || rank < 0) {
        Debug::error("insertChild rank [%d, %d]", rank, childSize);
        return;
    }
    if (rank == childSize) {
        childs[rank] = id;
        childSize++;
        return;
    }
    // [rank, childSize) -> (rank, childSize]
    memmove((void*)(childs[rank + 1]), (void*)(childs[rank]), (childSize - rank) * sizeof(int));
    childs[rank] = id;
    childSize++;
}
int BTNode::getChildSize() {
    return childSize;
}
int BTNode::getDataSize() {
    return dataSize;
}
int BTNode::getID() {
    return pageID;
}
int BTNode::getParent() {
    if (parent == NULL)
        return NULL_NODE;
    return *parent;
}
void BTNode::setParent(int p) {
    if (parent == NULL) {
        Debug::error("BTNode setParent without parent ptr init!");
        return;
    }
    *parent = p;
}
bool BTNode::isLeaf() {

}
void insertFrom(int r1, BTNode& node, int r2) {

}
void BTNode::withEmptyPage(BufType b, BTree btree, int pID) {
    withContentPage(b, btree, pID);
    dataSize = 0;
    childSize = 0;
    insertChild(0, NULL_NODE);
}
void BTNode::withContentPage(BufType b, BTree btree, int pID) {
    parent = (int*)b;
    data_size = (int*)((char*)b + sizeof(int));
    child_size = (int*)((char*)b + sizeof(int) * 2);
    entryLimit = btree.headPage.entryLimit;
    attrLength = btree.headPage.attrLength;
    char* atype = btree.headPage.attrType;
    if (strncmp(atype, "INT", 8) == 0) {
        attrType = AttrType::INT;
    } else if (strncmp(atype, "FLOAT", 8) == 0) {
        attrType = AttrType::FLOAT;
    } else if (strncmp(atype, "STRING", 8) == 0) {
        attrType = AttrType::STRING;
    } else {
        attrType = AttrType::INVALID;
    }
    pageID = pID;
    values = (char*)((char*)b + sizeof(int) * 2);
    char* rid_pos = values + entryLimit * attrLength;
    char* child_pos = rid_pos + sizeof(RID) * attrLength;
    rids = (RID*)(rid_pos);
    childs = (int*)child_pos;
}
BTNode BTree::loadNode(int pID) {
    int index;
    BufType b = bufPageManager->getPage(fileID, pID, index);
    bufPageManager->markDirty(index);
    BTNode node;
    node.withContentPage(b, *this, pID);
}
BTNode BTree::createNode() {
    int index;
    BufType b = bufPageManager->allocPage(fileID, _size + 1, index);
    bufPageManager->markDirty(index);
    BTNode node;
    node.withEmptyPage(b, *this, _size + 1);
}
int BTree::search(char* e, const RID& rid) {
    //check root
    if (_root == NULL_NODE) {

    }
    int v = _root; _hot = 0;
    while (v != NULL_NODE) {
        BTNode node_v = loadNode(v);
        int r = node_v.search(e, rid);
        if (r >= 0 && node_v.equals(r, e, rid))
            return v;
        _hot = v; v = node_v.getChild(r + 1);
    }
    return NULL_NODE;
}
bool BTree::insert(char* e, const RID& rid_in) {
    int v = search(e, rid_in);
    if (v != NULL_NODE) return false;
    BTNode node_hot = loadNode(_hot);
    int r = node_hot.search(e, rid_in);
    node_hot.insertData(r + 1, e, rid_in);
    node_hot.insertChild(r + 2, NULL_NODE);
    _size++; solveOverflow(node_hot);
    return true;
}
bool BTree::remove(char* e, const RID& rid_in) {
    int v = search(e, rid_in);
    if (v == NULL_NODE) return false;
    BTNode node_v = loadNode(v);
    int r = node_v.search(e, rid_in);
    if (!node_v.isLeaf()) {
        int u = node_v.getChild(r + 1);
        BTNode node_u = loadNode(u);
        while (!node_u.isLeaf()) {
            u = node_u.getChild(0);
            node_u = loadNode(u);
        }
        //char data[MAX_ATTRLENGTH];
        //RID rid;
        node_u.getData(0, data_global, rid_global);
        node_v.setData(r, data_global, rid_global);
        node_u.removeData(0);
        node_u.removeChild(1);
        _size--;
        solveUnderflow(node_u); return true;
    }
    node_v.removeData(r);
    node_v.removeChild(r + 1); _size--;
    solveUnderflow(node_v); return true;
}
void BTree::solveUnderflow(BTNode& node_v) {
    if ((_order + 1) / 2 <= node_v.getChildSize()) return;
    int p = node_v.getParent();
    if (p == NULL_NODE) return;
    BTNode node_p = loadNode(p);
    int r = 0, v = node_v.getID(); 
    while (v != node_p.getChild(r)) r++;
    if (r > 0) {
        // 情况1：向左兄弟借关键码
        int ls = node_p.getChild(r - 1);
        BTNode node_ls = loadNode(ls);
        if ((_order + 1) / 2 < node_ls.getChildSize()) {
            node_p.getData(r - 1, data_global, rid_global);
            node_v.insertData(0, data_global, rid_global);
            node_ls.removeData(node_ls.getDataSize() - 1, data_global, rid_global);
            node_p.setData(r - 1, data_global, rid_global);
            node_v.insertChild(0, node_ls.removeChild(node_ls.getChildSize() - 1));
            if (!node_v.isLeaf()) {
                BTNode node_lc = loadNode(node_v.getChild(0));
                node_lc.setParent(v);
            }
            return;
        }
    }
    if (node_p.getChildSize() - 1 > r) { // 若v不是p的最后一个儿子
        // 情况2：向右兄弟借关键码
        int rs = node_p.getChild(r + 1); // 右兄弟
        BTNode node_rs = loadNode(rs);
        if ((_order + 1) / 2 < node_rs.getChildSize()) { // 可以借
            node_p.getData(r, data_global, rid_global); // 借出兄弟最小元素
            node_v.insertData(node_v.getDataSize(), data_global, rid_global);
            node_rs.removeData(0, data_global, rid_global);
            node_p.setData(r, data_global, rid_global);
            node_v.insertChild(node_v.getChildSize(), node_rs.removeChild(0));
            if (!node_v.isLeaf()) {
                BTNode node_lc = loadNode(node_v.getChild(0));
                node_lc.setParent(v);
            }
            return;
        }
    }
    if (r > 0) {
        // 情况3 与左兄弟合并
        int ls = node_p.getChild(r - 1);
        BTNode node_ls = loadNode(ls);
        node_p.removeData(r - 1, data_global, rid_global);
        node_ls.insertData(node_ls.getDataSize(), data_global, rid_global);
        node_p.removeChild(r);
        node_ls.insertChild(node_ls.getChildSize(), node_v.removeChild(0));
        int lsc = node_ls.getChild(node_ls.getChildSize() - 1);
        if (lsc != NULL_NODE) {
            BTNode node_lsc = loadNode(lsc);
            node_lsc.setParent(ls);
        }
        while (node_v.getDataSize() > 0) {
            node_v.removeData(0, data_global, rid_global);
            node_ls.insertData(node_ls.getDataSize(), data_global, rid_global);
            node_ls.insertChild(node_ls.getChildSize(), node_v.removeChild(0));
            lsc = node_ls.getChild(node_ls.getChildSize() - 1);
            if (lsc != NULL_NODE) {
                BTNode node_lsc = loadNode(lsc);
                node_lsc.setParent(ls);
            }
        }
        //release(node_v);
    } else {
        // 情况3 与右兄弟合并
        int rs = node_p.getChild(r); 
        BTNode node_rs = loadNode(rs);
        node_p.removeData(r, data_global, rid_global);
        node_rs.insertData(0, data_global, rid_global);
        node_p.removeChild(r);
        node_rs.insertChild(0, node_v.removeChild(node_v.getChildSize() - 1));
        int rsc = node_rs.getChild(0);
        if (rsc != NULL_NODE) {
            BTNode node_rsc = loadNode(rsc);
            node_rsc.setParent(rs);
        }
        while (node_v.getDataSize() > 0) {
            node_v.removeData(node_v.getDataSize() - 1, data_global, rid_global);
            node_rs.insertData(0, data_global, rid_global);
            node_rs.insertChild(0, node_v.removeChild(node_v.getChildSize() - 1));
            rsc = node_rs.getChild(0);
            if (rsc != NULL_NODE) {
                BTNode node_rsc = loadNode(rsc);
                node_rsc.setParent(rs);
            }
        }
        //release(node_v);
    }
    solveUnderflow(node_p);
    return;
}
void BTree::solveOverflow(BTNode& node_v) {
    if (_order >= node_v.getChildSize()) return;
    int s = _order / 2;
    BTNode node_u = createNode();
    for (int j = 0; j < _order - s - 1; j++) {
        node_u.insertChild(j, node_v.removeChild(s + 1));
        node_v.removeData(s + 1, data_global, rid_global);
        node_u.insertData(j, data_global, rid_global);
    }
    node_u.setChild(_order - s - 1, node_v.removeChild(s + 1));
    if (node_u.getChild(0) != NULL_NODE) {
        for (int j = 0; j < _order - s; j++) {
            BTNode child = loadNode(node_u.getChild(j));
            child.setParent(node_u.getID());
        }
    }
    int p = node_v.getParent();
    BTNode node_parent;
    if (p == NULL_NODE) {
        node_parent = createNode();
        p = _root = node_parent.getID();
        node_parent.setChild(0, node_v.getID());
        node_v.setParent(p);
    } else {
        node_parent = loadNode(p);
    }
    node_v.getData(0, data_global, rid_global);
    int r = 1 + node_parent.search(data_global, rid_global);
    node_v.removeData(s, data_global, rid_global);
    node_parent.insertData(r, data_global, rid_global);
    node_parent.insertChild(r + 1, node_u.getID());
    node_u.setParent(p);
    solveOverflow(node_parent);
}
/* ix manager implement */
IX_Manager::IX_Manager(FileManager* fileManager, BufPageManager* bufPageManager) {
	this->fileManager = fileManager;
	this->bufPageManager = bufPageManager;
}
IX_Manager::~IX_Manager() {

}
int IX_Manager::CreateIndex(const char *fileName, int indexNo, AttrType attrType, int attrLength) {
    // 设置好文件名
	char indexFileName[MAX_FILENAME_LENGTH];
    setFileName(indexFileName, fileName, indexNo);
    // 创建，打开文件，申请headpage的缓存，写进缓存，然后更新
    bool created = fileManager->createFile(indexFileName);
    assert(created);
    int fileID;
    bool opened = fileManager->openFile(indexFileName, fileID);
    assert(opened);
    int index;
    BufType b = bufPageManager->allocPage(fileID, 0, index, false);
    Head_Page headPage;
    headPage.setHeader(b, attrType, attrLength);
    bufPageManager->markDirty(index);
    bufPageManager->writeBack(index);
    assert(fileManager->closeFile(index) == 0);
    return 0;
}
int IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    char indexFileName[256];
    setFileName(indexFileName, fileName, indexNo);
    if (remove(indexFileName)) 
        return -1;
    return 0;
}
int IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
	int fileID;
    char indexFileName[256];
    bool opened = fileManager->openFile(indexFileName, fileID);
    if (!opened) {
    	return -1;
	}
	int index;
	BufType b = bufPageManager->getPage(fileID, 0, index);
	indexHandle.init(fileID, b, this);
    return 0;
}
int IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
    return 0;
}
/* IX_IndexHandle implement */
void IX_IndexHandle::init(int fileID, BufType b, IX_Manager* ix_manager) {
	btree.init(b, fileID, ix_manager->fileManager, ix_manager->bufPageManager);
}
IX_IndexHandle::IX_IndexHandle() {

}
IX_IndexHandle::~IX_IndexHandle() {

}
int IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
    if (!btree.insert((char*)pData, rid)) {
        Debug::error("InsertEntry already exist!");
        return -1;
    }
    return 0;
}
int IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
    if (!btree.remove((char*)pData, rid)) {
        Debug::error("DeleteEntry not found!");
        return -1;
    }
    return 0;
}
int IX_IndexHandle::ForcePages() {

}
