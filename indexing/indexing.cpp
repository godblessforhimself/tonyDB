#include "indexing.h"
/* private function */
#define dataSize (*data_size)
#define childSize (*child_size)
#define _order (headPage.entryLimit+1)
#define _root (headPage.root)
char data_global[MAX_ATTRLENGTH];
RID rid_global;
/* ix manager implement */
AttrType Head_Page::getType() {
    if (strncmp(attrType, "INT", sizeof(attrType)) == 0) {
        return AttrType::INT;
    } else if (strncmp(attrType, "FLOAT", sizeof(attrType)) == 0) {
        return AttrType::FLOAT;
    } else if (strncmp(attrType, "STRING", sizeof(attrType)) == 0) {
        return AttrType::STRING;
    } else {
        return AttrType::INVALID;
    }
}
void Head_Page::setHeader(BufType b, AttrType attrType, int attrLength) {
    Head_Page* ptr = (Head_Page*)b;
    char typestr[8];
    switch (attrType) {
        case constSpace::INT:
            strncpy(typestr, "INT", sizeof(typestr));
            break;
        case FLOAT:
            strncpy(typestr, "FLOAT", sizeof(typestr));
            break;
        case STRING:
            strncpy(typestr, "STRING", sizeof(typestr));
            break;
        default:
            strncpy(typestr, "INVALID", sizeof(typestr));
    }
    memcpy((void*)b, typestr, sizeof(attrType));
    ptr->attrLength = attrLength;
    ptr->entryLimit = (PAGESIZE - sizeof(int) * 6) / (attrLength + sizeof(RID) + sizeof(int)) - 1; // 为溢出留空间
    ptr->pageNum = 2;
    ptr->root = 1;
    ptr->nextFree = 2;
    ptr->entryNum = 0;
}
void Head_Page::set(BufType b) {
    memcpy((void*)b, (void*)this, sizeof(Head_Page));
}
void Head_Page::parse(BufType b) {
    Head_Page* ptr = (Head_Page*)b;
    memcpy((void*)attrType, (void*)ptr, sizeof(attrType));
    attrLength = ptr->attrLength;
    entryLimit = ptr->entryLimit;
    pageNum = ptr->pageNum;
    root = ptr->root;
    nextFree = ptr->nextFree;
    entryNum = ptr->entryNum;
}
void printHead(Head_Page head) {
    Debug::debug("printHead: attrType %s, attrLength %d, entryLimit %d, pageNum %d, root %d, nextFree %d, entryNum %d", \
        head.attrType, head.attrLength, head.entryLimit, head.pageNum, head.root, head.nextFree, head.entryNum);
       
}
void forcePage(BufPageManager* bufPageManager, int index) {
    // 将修改过的页面写回并释放
    bufPageManager->markDirty(index);
    bufPageManager->writeBack(index);
}
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
    Head_Page::setHeader(b, attrType, attrLength);
    Head_Page headPage;
    headPage.parse(b);
    forcePage(bufPageManager, index);
    b = bufPageManager->allocPage(fileID, 1, index, false);
    BTNode root(bufPageManager);
    root.withEmptyPage(b, headPage, 1, index);
    *root.prev_free = 0; *root.next_free = 2;
    forcePage(bufPageManager, index);
    assert(fileManager->closeFile(index) == 0);
    return 0;
}
int IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    char indexFileName[MAX_FILENAME_LENGTH];
    setFileName(indexFileName, fileName, indexNo);
    if (remove(indexFileName)) 
        return -1;
    return 0;
}
int IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
	int fileID;
    char indexFileName[MAX_FILENAME_LENGTH];
    bool opened = fileManager->openFile(indexFileName, fileID);
    if (!opened) {
    	return -1;
	}
	int index;
	BufType b = bufPageManager->getPage(fileID, 0, index);
	indexHandle.init(fileID, b, this);
    bufPageManager->release(index);
    //释放首页，关闭时再写回
    return 0;
}
int IX_Manager::CloseIndex(IX_IndexHandle &indexHandle) {
    return 0;
}
/* IX_IndexHandle implement */
void IX_IndexHandle::init(int fileID, BufType b, IX_Manager* ix_manager) {
    headPage.parse(b);
    this->fileID = fileID;
    this->fileManager = ix_manager->fileManager;
    this->bufPageManager = ix_manager->bufPageManager;
    _hot = 0;
    attrLength = headPage.attrLength;
}
IX_IndexHandle::IX_IndexHandle() {

}
IX_IndexHandle::~IX_IndexHandle() {
    writeHeader();
}
int IX_IndexHandle::searchEntryRecur(int v, void *pData, RID& rid) {
    if (v == NULL_NODE)
        return -1;
    BTNode node_v = loadNode(v);
    if (node_v.getDataSize() == 0)
        return -1;
    int r = node_v.search((char*)pData, rid);
    if (r + 1 < node_v.getDataSize()) {
        if (searchEntryRecur(node_v.getChild(r + 1), pData, rid)) {
            node_v.getData(r + 1, (char*)pData, rid);
            return 0;
        } else {
            return 0;
        }
    } else {
        if (searchEntryRecur(node_v.getChild(r + 1), pData, rid)) {
            return -1;
        } else {
            return 0;
        }
    }
}
int IX_IndexHandle::SearchEntry(void *pData, RID& rid, bool compare) {
    //找到第一个大于(pData, rid)的项
    if (!compare) {
        // 找到node_v叶子
        int v = _root;
        BTNode node_v(bufPageManager);
        while (v != NULL_NODE) {
            node_v = loadNode(v);
            if (node_v.getChildSize() > 0 && node_v.getChild(0) != NULL_NODE) {
                v = node_v.getChild(0);
            } else {
                break;
            }
        }
        if (v == NULL_NODE) {
            return -1;
        } else {
            node_v.getData(0, (char*)pData, rid);
            return 0;
        }
    } else {
        int v = _root;
        if (searchEntryRecur(v, pData, rid)) {
            return -1;
        } else {
            return 0;
        }
    }
}
int IX_IndexHandle::InsertEntry(void *pData, const RID &rid) {
    if (!insert((char*)pData, rid)) {
        Debug::error("InsertEntry already exist!");
        return -1;
    }
    return 0;
}
int IX_IndexHandle::DeleteEntry(void *pData, const RID &rid) {
    if (!remove((char*)pData, rid)) {
        Debug::error("DeleteEntry not found!");
        return -1;
    }
    return 0;
}
int IX_IndexHandle::ForcePages() {
    bufPageManager->close();
    return 0;
}
void IX_IndexHandle::printHeadPage() {
    printHead(headPage);
}
void IX_IndexHandle::writeHeader() {
    int index;
    BufType b = bufPageManager->allocPage(fileID, 0, index);
    headPage.set(b);
    forcePage(bufPageManager, index);
}
BTNode IX_IndexHandle::loadNode(int pID) {
    int index;
    BufType b = bufPageManager->getPage(fileID, pID, index);
    BTNode node(bufPageManager);
    node.withContentPage(b, headPage, pID, index);
    return node;
}
BTNode IX_IndexHandle::createNode() {
    // 通过空闲链表找到pageID,初始化新结点,更新链表,修改页数
    // 注意,createNode的结点必须写回
    int index;
    int nextFree = headPage.nextFree;
    BufType b = bufPageManager->allocPage(fileID, nextFree, index);
    bufPageManager->markDirty(index);
    BTNode node(bufPageManager);
    node.withEmptyPage(b, headPage, nextFree, index);
    removeLink(node);
    if (nextFree == headPage.pageNum) 
        headPage.pageNum++;
    return node;
}
int IX_IndexHandle::search(char* e, const RID& rid) {
    // 不修改结点
    int v = _root; _hot = NULL_NODE;
    while (v != NULL_NODE) {
        BTNode node_v = loadNode(v);
        int r = node_v.search(e, rid);
        if (r >= 0 && node_v.equals(r, e, rid)) {
            //node_v.release(bufPageManager, false);
            return v;
        }
        _hot = v; v = node_v.getChild(r + 1);
        //node_v.release(bufPageManager, false);
    }
    return NULL_NODE;
}
bool IX_IndexHandle::remove(char* e, const RID& rid_in) {
    Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::remove is called");
    int v = search(e, rid_in);
    Debug::print(Debug::BTREE_REMOVE, "search result is %d", v);
    if (v == NULL_NODE) return false;
    BTNode node_v = loadNode(v);
    int r = node_v.search(e, rid_in);
    Debug::print(Debug::BTREE_REMOVE, "the result is pageID:%d, data index:%d", v, r);
    if (!node_v.isLeaf()) {
        int u = node_v.getChild(r + 1);
        BTNode node_u = loadNode(u);
        while (!node_u.isLeaf()) {
            u = node_u.getChild(0);
            //node_u.release(bufPageManager, false);
            node_u = loadNode(u);
        }
        node_u.removeData(0, data_global, rid_global);
        node_v.setData(r, data_global, rid_global);
        node_u.removeChild(1);
        //node_v.release(bufPageManager);
        solveUnderflow(node_u); 
        headPage.entryNum--;
        return true;
    }
    node_v.removeData(r);
    node_v.removeChild(r + 1);
    solveUnderflow(node_v);
    Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::remove return");
    headPage.entryNum--;
    return true;
}
void IX_IndexHandle::solveUnderflow(BTNode& node_v) {
    //被调用的结点在返回前会被写回
    int v = node_v.pageID, p = node_v.getParent();
    Debug::print(Debug::BTREE_REMOVE, "solveUnderflow %d %d", v, p);
    if (node_v.getChildSize() >= (_order + 1) / 2) {
        //node_v.release(bufPageManager);
        return;
    }
    if (p == NULL_NODE) {
        if (node_v.getDataSize() == 0 && node_v.getChild(0) != NULL_NODE) {
            // 根结点多余
            _root = node_v.getChild(0);
            BTNode newRoot = loadNode(_root);
            newRoot.setParent(NULL_NODE);
            //newRoot.release(bufPageManager);
            addLink(node_v);
        }
        //node_v.release(bufPageManager);
        return;
    }
    BTNode node_p = loadNode(p);
    int r = 0; 
    while (v != node_p.getChild(r)) r++;
    if (r > 0) {
        // 情况1：向左兄弟借关键码
        int ls = node_p.getChild(r - 1);
        BTNode node_ls = loadNode(ls);
        if (node_ls.getChildSize() > (_order + 1) / 2) {
            node_p.getData(r - 1, data_global, rid_global);
            node_v.insertData(0, data_global, rid_global);
            node_ls.removeData(node_ls.getDataSize() - 1, data_global, rid_global);
            node_p.setData(r - 1, data_global, rid_global);
            node_v.insertChild(0, node_ls.removeChild(node_ls.getChildSize() - 1));
            //node_p.release(bufPageManager);
            //node_ls.release(bufPageManager);
            if (!node_v.isLeaf()) {
                BTNode node_lc = loadNode(node_v.getChild(0));
                node_lc.setParent(v);
                //node_lc.release(bufPageManager);
            }
            //node_v.release(bufPageManager);
            return;
        }
        //node_ls.release(bufPageManager, false);
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
            //node_p.release(bufPageManager);
            //node_rs.release(bufPageManager);
            if (!node_v.isLeaf()) {
                BTNode node_rc = loadNode(node_v.getChild(0));
                node_rc.setParent(v);
                //node_rc.release(bufPageManager);
            }
            //node_v.release(bufPageManager);
            return;
        }
        //node_rs.release(bufPageManager, false);
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
            //node_lsc.release(bufPageManager);
        }
        while (node_v.getDataSize() > 0) {
            node_v.removeData(0, data_global, rid_global);
            node_ls.insertData(node_ls.getDataSize(), data_global, rid_global);
            node_ls.insertChild(node_ls.getChildSize(), node_v.removeChild(0));
            lsc = node_ls.getChild(node_ls.getChildSize() - 1);
            if (lsc != NULL_NODE) {
                BTNode node_lsc = loadNode(lsc);
                node_lsc.setParent(ls);
                //node_lsc.release(bufPageManager);
            }
            Debug::print(Debug::BTREE_REMOVE, "solveUnderflow 与左兄弟合并");
        }
        //node_ls.release(bufPageManager);
        addLink(node_v);
        //node_v.release(bufPageManager);
    } else {
        // 情况3 与右兄弟合并
        int rs = node_p.getChild(r + 1); 
        BTNode node_rs = loadNode(rs);
        node_p.removeData(r, data_global, rid_global);
        node_rs.insertData(0, data_global, rid_global);
        node_p.removeChild(r);
        node_rs.insertChild(0, node_v.removeChild(node_v.getChildSize() - 1));
        int rsc = node_rs.getChild(0);
        if (rsc != NULL_NODE) {
            BTNode node_rsc = loadNode(rsc);
            node_rsc.setParent(rs);
            //node_rsc.release(bufPageManager);
        }
        while (node_v.getDataSize() > 0) {
            //Debug::print(Debug::BTREE_REMOVE, "solveUnderflow 与右兄弟合并 node_v.size %d", node_v.getDataSize());
            node_v.removeData(node_v.getDataSize() - 1, data_global, rid_global);
            node_rs.insertData(0, data_global, rid_global);
            node_rs.insertChild(0, node_v.removeChild(node_v.getChildSize() - 1));
            rsc = node_rs.getChild(0);
            if (rsc != NULL_NODE) {
                BTNode node_rsc = loadNode(rsc);
                node_rsc.setParent(rs);
                //node_rsc.release(bufPageManager);
            }
            //Debug::print(Debug::BTREE_REMOVE, "solveUnderflow 与右兄弟合并");
        }
        //node_rs.release(bufPageManager);
        Debug::print(Debug::BTREE_REMOVE, "solveUnderflow load parent3 is %d", node_p.getParent());
        addLink(node_v);
        Debug::print(Debug::BTREE_REMOVE, "solveUnderflow load parent4 is %d", node_p.getParent());
        //node_v.release(bufPageManager);
    }
    Debug::print(Debug::BTREE_REMOVE, "solveUnderflow load parent2 is %d", node_p.getParent());
    solveUnderflow(node_p);
    return;
}
bool IX_IndexHandle::insert(char* e, const RID& rid_in) {
    int v = search(e, rid_in);
    if (v != NULL_NODE) return false;
    BTNode node_hot = loadNode(_hot);
    int r = node_hot.search(e, rid_in);
    node_hot.insertData(r + 1, e, rid_in);
    node_hot.insertChild(r + 2, NULL_NODE);
    solveOverflow(node_hot);
    headPage.entryNum++;
    return true;
}
void IX_IndexHandle::solveOverflow(BTNode& node_v) {
    // split node_v to node_u, add middle to parent, node_u, middle, node_v
    // 被调用结点node_v返回前写回
    if (node_v.getChildSize() <= _order) {
        //node_v.release(bufPageManager);
        return;
    }
    int s = _order / 2;
    BTNode node_u = createNode();
    //node_u
    for (int j = 0; j < _order - s - 1; j++) {
        node_u.insertChild(j, node_v.removeChild(s + 1));
        node_v.removeData(s + 1, data_global, rid_global);
        node_u.insertData(j, data_global, rid_global);
    }
    node_u.setChild(_order - s - 1, node_v.removeChild(s + 1));
    if (node_u.getChild(0) != NULL_NODE) {
        for (int j = 0; j < _order - s; j++) {
            BTNode child = loadNode(node_u.getChild(j));
            child.setParent(node_u.pageID);
            //child.release(bufPageManager);
        }
    }
    int p = node_v.getParent();
    BTNode node_parent(bufPageManager);
    if (p == NULL_NODE) {
        //node[u, parent]
        node_parent = createNode();
        p = _root = node_parent.pageID;
        node_parent.setChild(0, node_v.pageID);
        node_v.setParent(p);
    } else {
        node_parent = loadNode(p);
    }
    node_v.getData(0, data_global, rid_global);
    int r = 1 + node_parent.search(data_global, rid_global);
    node_v.removeData(s, data_global, rid_global);
    node_parent.insertData(r, data_global, rid_global);
    node_parent.insertChild(r + 1, node_u.pageID);
    node_u.setParent(p);
    //node_v.release(bufPageManager);
    //node_u.release(bufPageManager);
    solveOverflow(node_parent);
}
void IX_IndexHandle::addLink(const BTNode& node) {
    // 被调用的结点node不会被写回
    // case 1: node is before head.next
    if (node.pageID < headPage.nextFree) {
        if (headPage.nextFree != headPage.pageNum) {
            BTNode next_node = loadNode(headPage.nextFree);
            *(next_node.prev_free) = node.pageID;
            //next_node.release(bufPageManager);
        }
        headPage.nextFree = node.pageID;
        *(node.prev_free) = 0;
        return;
    } else { // case2: node is after head.next
        BTNode current = loadNode(headPage.nextFree);
        int next = *current.next_free;
        while (next < node.pageID) {
            //current.release(bufPageManager);
            current = loadNode(next);
            next = *current.next_free;
        }
        *current.next_free = node.pageID;
        //current.release(bufPageManager);
        *node.prev_free = next;
        if (next != headPage.pageNum) {
            BTNode next_node = loadNode(next);
            *next_node.prev_free = node.pageID;
        }
    }
}
void IX_IndexHandle::removeLink(const BTNode& node) {
    // 从空闲链表移除
    // 被调用的结点不会被写回
    if (headPage.nextFree == node.pageID) {
        headPage.nextFree++;
        *node.prev_free = 0;
        *node.next_free = headPage.nextFree;
        return;
    }
    int next = *node.next_free;
    headPage.nextFree = next;
    assert(*node.prev_free == 0);
    if (next != headPage.pageNum) {
        BTNode next_node = loadNode(next);
        *next_node.prev_free = node.pageID;
        //next_node.release(bufPageManager);
    }
}
void IX_IndexHandle::traverse() {
    traverse(_root);
}
void IX_IndexHandle::traverse(int pageID) {
    //按从小到大顺序打印
    printHeadPage();
    BTNode node = loadNode(pageID);
    node.print();
    for (int i = 0; i < node.getChildSize(); i++) {
        int child = node.getChild(i);
        if (child != NULL_NODE) {
            BTNode chi = loadNode(child);
            chi.print();
        }
    }
}
/* IX_IndexScan */
IX_IndexScan::IX_IndexScan() {

}
IX_IndexScan::~IX_IndexScan() {

}
int IX_IndexScan::OpenScan(IX_IndexHandle &indexHandle, CompOp compOp, void *value) {
    numScanned = 0;
    this->indexHandle = &indexHandle;
    attrLength = indexHandle.attrLength;
    attrType = indexHandle.headPage.getType();
    this->value = new char[attrLength];
    current_value = new char[attrLength];
    memcpy(this->value, value, attrLength);
    this->compOp = compOp;
    if (compOp == NO_OP || compOp == NE_OP) {
        Debug::error("IX_IndexScan use wrong op");
        return -1;
    }
    switch (compOp) {
        case CompOp::EQ_OP : comparator = &equal; break;
        case CompOp::LT_OP : comparator = &less_than; break;
        case CompOp::GT_OP : comparator = &greater_than; break;
        case CompOp::LE_OP : comparator = &less_than_or_eq_to; break;
        case CompOp::GE_OP : comparator = &greater_than_or_eq_to; break;
        default: Debug::error("IX_IndexScan use wrong op"); break;
    }
    return 0;
}
int IX_IndexScan::GetNextEntry(RID &rid) {
    bool comp = false;
    while (!comp) {
        if (indexHandle->SearchEntry(current_value, current_RID, (numScanned > 0))) {
            return -1;
        }
        comp = comparator(current_value, value, attrType, attrLength);
    }
    rid.copy(current_RID);
    numScanned++;
    return 0;
}
int IX_IndexScan::CloseScan() {
    return 0;
}
void BTNode::print() const {
    Debug::debug("node:[pageID:%d,parent:%d,dataSize:%d,childSize:%d,prev_free:%d,next_free:%d]",\
        pageID, *parent, *data_size, *child_size, *prev_free, *next_free);
    /* Debug::debug("\tdata:");
    for (int i = 0; i < dataSize; i++) {
        getData(i, data_global, rid_global);
        Debug::debug("\t[i:%d,data:%d,rid:(%d,%d)]", i, *(int*)data_global, rid_global.getPage(), rid_global.getSlot());
    } */
}
void BTNode::markDirty() const {
    bufPageManager->markDirty(bufIndex);
}
BTNode::BTNode(BufPageManager* bufPageManager) {
    this->bufPageManager = bufPageManager;
    parent = NULL; data_size = NULL; child_size = NULL;
    values = NULL; rids = NULL; childs = NULL; prev_free = next_free = NULL;
    entryLimit = 0; attrLength = 0; pageID = bufIndex = -1;
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
/* revise node */
void BTNode::setChild(int rank, int pageID) {
    if (rank >= 0 && rank < childSize) {
        if (childs[rank] != pageID) {
            childs[rank] = pageID;
            markDirty();    
        }
        return;
    }
    Debug::debug("setChild rank [%d, %d]", rank, childSize);
}
void BTNode::setData(int rank, char* data, RID& rid) {
    if (rank < 0 || rank >= dataSize) {
        Debug::error("setData range exceeds %d, %d", rank, dataSize);
        return;
    }
    char* value = values + rank * attrLength;
    memcpy(value, data, attrLength);
    rids[rank].copy(rid);
    markDirty();
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
    markDirty();
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
    markDirty();
}
int BTNode::removeChild(int rank) {
    if (rank < 0 || rank >= childSize) {
        Debug::error("removeChild rank [%d, %d]", rank, childSize);
        return NULL_NODE;
    }
    // (rank, childSize) 
    int ret = childs[rank];
    memmove((void*)&childs[rank], (void*)(&childs[rank + 1]), (childSize - 1 - rank) * sizeof(int));
    childSize--;
    markDirty();
    return ret;
}
void BTNode::insertData(int rank, char* data, const RID& rid) {
    // rank [0, dataSize]
    if (rank > dataSize || rank < 0) {
        Debug::error("insertData rank [%d, %d]", rank, dataSize);
        return;
    }
    markDirty();
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
    markDirty();
    if (rank == childSize) {
        childs[rank] = id;
        childSize++;
        return;
    }
    // [rank, childSize) -> (rank, childSize]
    memmove((void*)(&childs[rank + 1]), (void*)(&childs[rank]), (childSize - rank) * sizeof(int));
    childs[rank] = id;
    childSize++;
}
void BTNode::setParent(int p) {
    if (parent == NULL) {
        Debug::error("BTNode setParent without parent ptr init!");
        return;
    }
    markDirty();
    *parent = p;
}
/* without revising node */
int BTNode::getChild(int child) const{
    // child valid [0, size)
    if (child >= 0 && child < childSize)
        return childs[child];
    return NULL_NODE;
}
void BTNode::getData(int rank, char* data, RID& rid) const{
    if (rank < 0 || rank >= dataSize) {
        Debug::error("getData range exceeds %d, %d", rank, dataSize);
        return;
    }
    char* value = values + rank * attrLength;
    memcpy(data, value, attrLength);
    rid.copy(rids[rank]);
}
int BTNode::getChildSize() const {
    return childSize;
}
int BTNode::getDataSize() const {
    return dataSize;
}
int BTNode::getParent() const {
    if (parent == NULL)
        return NULL_NODE;
    return *parent;
}
bool BTNode::isLeaf() const {
    return childs[0] == NULL_NODE;
}
void BTNode::withEmptyPage(BufType b, Head_Page headPage, int pID, int index) {
    //node init here
    memset((void*)b, 0, PAGESIZE);
    withContentPage(b, headPage, pID, index);
    dataSize = 0;
    childSize = 0;
    *parent = NULL_NODE;
    insertChild(0, NULL_NODE);
}
void BTNode::withContentPage(BufType b, Head_Page headPage, int pID, int index) {
    parent = (int*)b;
    data_size = (int*)b + 1;
    child_size = (int*)b + 2;
    prev_free = (int*)b + 3;
    next_free = (int*)b + 4;
    entryLimit = headPage.entryLimit;
    attrLength = headPage.attrLength;
    char* atype = headPage.attrType;
    if (strncmp(atype, "INT", 8) == 0) {
        attrType = AttrType::INT;
    } else if (strncmp(atype, "FLOAT", 8) == 0) {
        attrType = AttrType::FLOAT;
    } else if (strncmp(atype, "STRING", 8) == 0) {
        attrType = AttrType::STRING;
    } else {
        attrType = AttrType::INVALID;
    }
    pageID = pID; bufIndex = index;
    values = (char*)((char*)b + sizeof(int) * 5);
    char* rid_pos = values + (entryLimit + 1) * attrLength;
    char* child_pos = rid_pos + (entryLimit + 1) * sizeof(RID);
    rids = (RID*)(rid_pos);
    childs = (int*)child_pos;
}