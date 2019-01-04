#include "indexing.h"
/* private function */
#define _order (headPage.entryLimit)
#define _root (headPage.root)
char data_global[MAX_ATTRLENGTH];
RID rid_global;
int BTNode::entryLimit;
int BTNode::attrLength;
AttrType BTNode::attrType;
BufPageManager* BTNode::bufPageManager;
/* ix manager implement */
// 约定 null位在attrlen后, 0表示null, 1表示非null
void appendNullbit(char *dst, char *src, int len) {
    if (src == NULL) {
        // 
        memset(dst, 0, len + 1);
    } else {
        memcpy(dst, src, len);
        dst[len] = 1;
    }
}
AttrType Head_Page::getType() {
    return getAttrType(attrType);
}
void Head_Page::setHeader(BufType b, AttrType attrType, int attrLength) {
    /* 将 attrType attrLength 写入内存 */
    Head_Page* ptr = (Head_Page*)b;
    char typestr[8];
    transAttrType(attrType, typestr);
    memcpy((void*)b, typestr, sizeof(attrType));
    ptr->attrLength = attrLength;
    ptr->entryLimit = (PAGESIZE - sizeof(int) * 7) / (attrLength + 1 + sizeof(RID) + sizeof(int)) - 1; // 为溢出留空间
    ptr->pageNum = 2;
    ptr->root = 1;
    ptr->nextFree = 2;
    ptr->entryNum = 0;
    ptr->right = 1;
}
void Head_Page::set(BufType b) {
    /* 将当前head里的值写入内存 */
    memcpy((void*)b, (void*)this, sizeof(Head_Page));
}
void Head_Page::parse(BufType b) {
    /* 从内存中读取到当前head中 */
    Head_Page* ptr = (Head_Page*)b;
    memcpy((void*)attrType, (void*)ptr, sizeof(attrType));
    attrLength = ptr->attrLength;
    entryLimit = ptr->entryLimit;
    pageNum = ptr->pageNum;
    root = ptr->root;
    nextFree = ptr->nextFree;
    entryNum = ptr->entryNum;
    right = ptr->right;
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
    // 为了处理空值，增加一字节，0表示空，1表示非空
    // 设置好文件名
	char indexFileName[MAX_FILENAME_LENGTH];
    setFileName(indexFileName, fileName, indexNo);
    // 创建，打开文件，申请headpage的缓存，写进缓存，然后更新
    if (!fileManager->createFile(indexFileName)) {
        cout << "IX_Manager::CreateInde 创建文件" << indexFileName << "失败\n";
        return -1;
    }
    int fileID;
    if (!fileManager->openFile(indexFileName, fileID)) {
        cout << "IX_Manager::CreateInde 打开文件" << indexFileName << "失败\n";
        return -1;
    }
    int index;
    BufType b = bufPageManager->allocPage(fileID, 0, index, false);
    Head_Page::setHeader(b, attrType, attrLength);
    Head_Page headPage;
    headPage.parse(b);
    forcePage(bufPageManager, index);
    b = bufPageManager->allocPage(fileID, 1, index, false);
    BTNode root(bufPageManager);
    root.withEmptyPage(b, headPage, 1, index);
    *(root.prev_free) = 0; *(root.next_free) = 2;
    root.setLeaf(true);
    forcePage(bufPageManager, index);
    if (fileManager->closeFile(index) != 0) {
        cout << "IX_Manager::CreateIndex 关闭文件失败\n";
        return -1;
    }
    Debug::debug("IX_Manager::CreateIndex %s", indexFileName);
    return 0;
}
int IX_Manager::DestroyIndex(const char *fileName, int indexNo) {
    char indexFileName[MAX_FILENAME_LENGTH];
    setFileName(indexFileName, fileName, indexNo);
    Debug::debug("IX_Manager::DestroyIndex %s", indexFileName);
    if (remove(indexFileName)) 
        return -1;
    return 0;
}
int IX_Manager::OpenIndex(const char *fileName, int indexNo, IX_IndexHandle &indexHandle) {
	int fileID;
    char indexFileName[MAX_FILENAME_LENGTH];
    if (!fileManager->openFile(indexFileName, fileID)) {
        cout << "IX_Manager::OpenIndex 打开文件" << fileName << "失败\n";
    	return -1;
	}
	int index;
	BufType b = bufPageManager->getPage(fileID, 0, index);
	indexHandle.init(fileID, b, this);
    //释放首页，关闭时再写回
    bufPageManager->release(index);
    Debug::debug("IX_Manager::OpenIndex %s", indexFileName);
    return 0;
}
/* IX_IndexHandle implement */
void IX_IndexHandle::printFreeLink() {
    Debug::debugL("[0]->");
    int next = headPage.nextFree;
    while (next != headPage.pageNum) {
        Debug::debugL("[%d]->", next);    
        next = *(loadNode(next).next_free);
    }
    Debug::debugL("[%d]\n", next);
}
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
    ForcePages();
    fileManager->closeFile(fileID);
}
/*int IX_IndexHandle::searchEntryRecur(int v, void *pData, RID& rid) {
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
}*/
int IX_IndexHandle::InsertNull(const RID &rid) {
    return 0;
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
int BTNode::getParentRank(const BTNode& son, const BTNode& parent) {
    int r = 0;
    while (parent.getChild(r) != son.pageID) r++;
    return r;
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
    // createNode的结点必须写回
    int index;
    int nextFree = headPage.nextFree;
    BufType b = bufPageManager->getPage(fileID, nextFree, index);
    bufPageManager->markDirty(index);
    BTNode node(bufPageManager);
    node.withContentPage(b, headPage, nextFree, index);
    removeLink(node);
    *node.parent = NULL_NODE;
    *node.left = NULL_NODE;
    *node.right = NULL_NODE;
    *node.prev_free = NULL_NODE;
    //Debug::debug("node id:%d", nextFree);
    *node.next_free = NULL_NODE;
    *node.entry_count = 0;
    *node.child_count = 0;
    *node.is_leaf = 0;
    if (nextFree == headPage.pageNum) 
        headPage.pageNum++;
    return node;
}
int IX_IndexHandle::search(char* e1, const RID& rid) {
    // 入口的e是原型，处理一下
    char e[MAX_ATTRLENGTH];
    appendNullbit(e, e1, attrLength);
    // 搜索到叶子，若确实相等，则返回pageID，没有相等，返回无
    // 1.待插入位置
    // 2.目标元素
    // 3._hot为pageID
    //Debug::debug("IX_IndexHandle::search _root is %d", _root);
    int v = _root; _hot = _root;
    while (v != NULL_NODE) {
        BTNode node_v = loadNode(v);
        int rank = node_v.search(e, rid);
        if (!node_v.isLeaf()) {
            // 非叶子结点
            v = node_v.getChild(rank);
            //Debug::debug("IX_IndexHandle::search 转向第%d个子结点 pageID:%d", rank, v);
        } else {
            _hot = v;
            if (rank < 0 || rank == node_v.getDataSize()) {
                //Debug::debug("IX_IndexHandle::search rank return %d", rank);
                return NULL_NODE;
            }
            node_v.getData(rank, data_global, rid_global);
            if (BTNode::comp(e, rid, data_global, rid_global) == 0) {
                //Debug::debug("IX_IndexHandle::search find return [v:%d,rank:%d]", v, rank);
                return v;
            } else {
                //Debug::debug("IX_IndexHandle::search not find return [v:%d,rank:%d]", v, rank);
                return NULL_NODE;
            }
        }
    }
    return NULL_NODE;
}
bool IX_IndexHandle::remove(char* e, const RID& rid_in) {
    // 原型
    Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::remove is called");
    int v = search(e, rid_in);
    Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::remove 删除的结点pageID:%d", v);
    if (v == NULL_NODE) return false;
    BTNode node_v = loadNode(v);
    char e1[MAX_ATTRLENGTH];
    appendNullbit(e1, e, attrLength);
    int r = node_v.search(e1, rid_in); // 拓展型
    Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::remove 删除的结点秩:%d", r);
    assert(node_v.isLeaf());
    node_v.removeData(r);
    solveUnderflow(node_v);
    Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::remove 成功");
    headPage.entryNum--;
    return true;
}
void IX_IndexHandle::solveUnderflow(BTNode& node_v) {
    //被调用的结点在返回前会被写回
    int v = node_v.pageID, p = node_v.getParent();
    Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::solveUnderflow 当前结点pageID:%d, 父结点pageID:%d", v, p);
    if (node_v.getDataSize() >= (_order + 1) / 2) {
        int rank;
        do {
            if (p == NULL_NODE)
                return;
            BTNode node_parent = loadNode(p);
            rank = BTNode::getParentRank(node_v, node_parent);
            rank = updateParent(node_v, rank);
            node_v = loadNode(p);
        } while (rank == 0);
        Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::solveUnderflow 没有下溢");
        return;
    }
    if (p == NULL_NODE) {
        if (node_v.getDataSize() == 1 && !node_v.isLeaf()) {
            // 当前结点为根，但是当前结点只有一个孩子
            // 不改变叶子指针
            _root = node_v.getChild(0);
            BTNode newRoot = loadNode(_root);
            newRoot.setParent(NULL_NODE);
            addLink(node_v);
            Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::solveUnderflow 当前结点为根，但是当前结点只有一个孩子");
        }
        Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::solveUnderflow 当前结点为根，结束");
        return;
    }
    BTNode node_p = loadNode(p);
    bool leaf = node_v.isLeaf();
    int r = BTNode::getParentRank(node_v, node_p);
    if (r > 0) {
        // 情况1：向左兄弟借关键码
        int ls = node_p.getChild(r - 1);
        BTNode node_ls = loadNode(ls);
        if (node_ls.getDataSize() > (_order + 1) / 2) {
            if (leaf) {
                // 只借出数据项
                int rightMost = node_ls.getDataSize() - 1;
                node_ls.removeData(rightMost, data_global, rid_global);
                node_v.insertData(0, data_global, rid_global);
                node_p.setData(r, data_global, rid_global);
                updateToRoot(v);
            } else {
                // 还需要孩子
                int rightMost = node_ls.getDataSize() - 1;
                node_ls.removeData(rightMost, data_global, rid_global);
                node_v.insertData(0, data_global, rid_global);
                node_p.setData(r, data_global, rid_global);
                node_v.insertChild(0, node_ls.removeChild(rightMost));
                BTNode node_child = loadNode(node_v.getChild(0));
                node_child.setParent(v);
                updateToRoot(v);
            }
            Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::solveUnderflow 情况1：向左兄弟借关键码");
            return;
        }
    }
    if (node_p.getDataSize() - 1 > r) { // 若v不是p的最后一个儿子
        // 情况2：向右兄弟借关键码
        int rs = node_p.getChild(r + 1);
        BTNode node_rs = loadNode(rs);
        if (node_rs.getDataSize() > (_order + 1) / 2) {
            int v_size = node_v.getDataSize();
            if (!leaf) {
                // 孩子
                node_v.insertChild(v_size, node_rs.removeChild(0));
                BTNode node_child = loadNode(node_v.getChild(v_size));
                node_child.setParent(v);
            }
            node_rs.removeData(0, data_global, rid_global);
            node_v.insertData(v_size, data_global, rid_global);
            if (v_size == 0)
                node_p.setData(r, data_global, rid_global);
            updateToRoot(v);
            Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::solveUnderflow 情况2：向右兄弟借关键码");
            return;
        }
    }
    if (r > 0) {
        // 情况3：与左兄弟合并
        // 删除父亲的第r项 更新父亲的r-1项
        int ls = node_p.getChild(r - 1);
        BTNode node_ls = loadNode(ls);
        node_p.removeData(r);
        node_p.removeChild(r);
        while (node_v.getDataSize() > 0) {
            node_v.removeData(0, data_global, rid_global);
            node_ls.insertData(node_ls.getDataSize(), data_global, rid_global);
            if (!leaf) {
                node_ls.insertChild(node_ls.getChildSize(), node_v.removeChild(0));
                int child = node_ls.getChild(node_ls.getChildSize() - 1);
                BTNode node_child = loadNode(child);
                node_child.setParent(node_ls.pageID);
            }
        }
        // 改变叶子指针 移除了node_v
        removeLeafLink(node_v);
        addLink(node_v);
        assert(updateParent(node_ls, r - 1) < 0);
        Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::solveUnderflow 情况3 与左兄弟合并");
    } else {
        // 情况4：与右兄弟合并
        // 删除父亲第r+1项，更新父亲第r项
        int rs = node_p.getChild(r + 1);
        BTNode node_rs = loadNode(rs);
        node_p.removeData(r + 1);
        node_p.removeChild(r + 1);
        while (node_rs.getDataSize() > 0) {
            node_rs.removeData(0, data_global, rid_global);
            node_v.insertData(node_v.getDataSize(), data_global, rid_global);
            if (!leaf) {
                node_v.insertChild(node_v.getChildSize(), node_rs.removeChild(0));
                int child = node_v.getChild(node_v.getChildSize() - 1);
                BTNode node_child = loadNode(child);
                node_child.setParent(node_v.pageID);
            }
        }
        // 改变叶子指针 移除了node_rs
        removeLeafLink(node_rs);
        addLink(node_rs);
        assert(updateParent(node_v, r) < 0);
        Debug::print(Debug::BTREE_REMOVE, "IX_IndexHandle::solveUnderflow 情况4 与右兄弟合并");
    }
    solveUnderflow(node_p);
    return;
}
bool IX_IndexHandle::insert(char* e, const RID& rid_in) {
    // e原型
    /* pseudocode 
        1.找到叶子结点v
        2.插入(e,rid_in)
        3.solveOverflow
    */
    char e1[MAX_ATTRLENGTH];
    appendNullbit(e1, e, attrLength);
    int v = search(e, rid_in); // 原型
    if (v != NULL_NODE) return false;
    BTNode node_hot = loadNode(_hot);
    int r = node_hot.search(e1, rid_in); // 拓展型
    node_hot.insertData(r, e1, rid_in); // 拓展型
    Debug::print(Debug::BTREE_INSERT, "IX_IndexHandle::insert to %d, rank %d", node_hot.pageID, r);
    //Debug::debug("IX_IndexHandle::insert to %d, rank %d", node_hot.pageID, r);
    //Debug::debug("IX_IndexHandle::insert to %d, rank %d", node_hot.pageID, r);
    //Debug::debug("Enter solveOverflow");
    solveOverflow(node_hot);
    //Debug::debug("Leave solveOverflow");
    headPage.entryNum++;
    //traverse(1);
    return true;
}
int IX_IndexHandle::updateParent(BTNode& node_v) {
    // 更新node_v父结点的数据项, 返回被更新的秩
    int parent = node_v.getParent();
    if (parent == NULL_NODE || node_v.getDataSize() == 0) {
        return -1;
    }
    BTNode node_parent = loadNode(parent);
    int v = node_v.pageID, r = 0;
    while (node_parent.getChild(r) != v) r++;
    char data_internal[MAX_ATTRLENGTH];
    RID rid_internal;
    node_parent.getData(r, data_internal, rid_internal);
    node_v.getData(0, data_global, rid_global);
    if (BTNode::comp(data_global, rid_global, data_internal, rid_internal) < 0) {
        node_parent.setData(r, data_global, rid_global);
        return r;
    }
    return -1;
}
int IX_IndexHandle::updateParent(BTNode& node_v, int r) {
    // 更新node_v父结点的数据项, 返回被更新的秩
    int parent = node_v.getParent();
    if (parent == NULL_NODE || node_v.getDataSize() == 0) {
        return -1;
    }
    BTNode node_parent = loadNode(parent);
    char data_internal[MAX_ATTRLENGTH];
    RID rid_internal;
    node_parent.getData(r, data_internal, rid_internal);
    node_v.getData(0, data_global, rid_global);
    if (BTNode::comp(data_global, rid_global, data_internal, rid_internal) < 0) {
        node_parent.setData(r, data_global, rid_global);
        return r;
    }
    return -1;
}
void IX_IndexHandle::updateToRoot(int v) {
    // 
    if (v == NULL_NODE)
        return;
    BTNode node_v = loadNode(v);
    int parent = node_v.getParent();
    while (parent != NULL_NODE && node_v.getDataSize() > 0) {
        BTNode node_parent = loadNode(parent);
        int rank = BTNode::getParentRank(node_v, node_parent);
        node_v.getData(0, data_global, rid_global);
        node_parent.setData(rank, data_global, rid_global);
        if (rank != 0)
            break;
        v = parent;
        parent = node_parent.getParent();
    }
}
void IX_IndexHandle::solveOverflow(BTNode& node_v) {
    // 
    // 
    if (node_v.getDataSize() <= _order) {
        updateToRoot(node_v.pageID);
        return;
    }
    int s = (_order + 1) / 2;
    BTNode node_u = createNode();
    bool leaf = node_v.isLeaf();
    node_u.setLeaf(leaf);
    if (leaf) {
        addLeafLinkAfter(node_v, node_u);
    }
    //Debug::debug("IX_IndexHandle::solveOverflow for node_v(pageID:%d), dataSize:%d, root %d", node_v.pageID, node_v.getDataSize(), _root);
    //node_v(s) node_u(_order+1-s)
    for (int j = 0; j < _order + 1 - s; j++) {
        if (!leaf) {
            // Debug::debug("remove child");
            node_u.insertChild(j, node_v.removeChild(s));
        }
        node_v.removeData(s, data_global, rid_global);
        node_u.insertData(j, data_global, rid_global);
    }
    if (!leaf) {
        for (int j = 0; j < _order + 1 - s; j++) {
            BTNode node_child = loadNode(node_u.getChild(j));
            node_child.setParent(node_u.pageID);
        }
    }
    int p = node_v.getParent();
    BTNode node_parent(bufPageManager);
    if (p == NULL_NODE) {
        // 全树高度增加
        node_parent = createNode();
        node_parent.setLeaf(false);
        p = _root = node_parent.pageID;
        node_v.getData(0, data_global, rid_global);
        node_parent.insertData(0, data_global, rid_global);
        node_parent.insertChild(0, node_v.pageID);
        node_v.setParent(p);
    } else {
        node_parent = loadNode(p);
    }
    int r = BTNode::getParentRank(node_v, node_parent);
    node_u.getData(0, data_global, rid_global);
    node_parent.insertData(r + 1, data_global, rid_global);
    node_parent.insertChild(r + 1, node_u.pageID);
    node_u.setParent(p);
    solveOverflow(node_parent);
    //node_v.print();
    // Debug::debug("node_v:pageID:%d,node_u.pageID:%d", node_v.pageID, node_u.pageID);
}
void IX_IndexHandle::addLink(const BTNode& node) {
    // 将node的pageID加入空闲链表
    Debug::print(Debug::BTREE_LINK_DETAIL, "IX_IndexHandle::addLink:%d", node.pageID);
    if (node.pageID < headPage.nextFree) {
        // 情况1:pageID在第一个空闲指针前，
        *(node.next_free) = headPage.nextFree;
        *(node.prev_free) = 0;
        if (headPage.nextFree != headPage.pageNum) {
            // 第一个空闲页[存在]
            BTNode next_node = loadNode(headPage.nextFree);
            *(next_node.prev_free) = node.pageID;
        }
        headPage.nextFree = node.pageID;
    } else { // 情况2:pageID在某个指针后
        BTNode current = loadNode(headPage.nextFree);
        int next = *current.next_free;
        while (next < node.pageID) {
            current = loadNode(next);
            next = *current.next_free;
        }
        // 现在current.pageID < pageID < next
        *current.next_free = node.pageID;
        *node.prev_free = current.pageID;
        *node.next_free = next;
        if (next != headPage.pageNum) {
            // 若next页面存在
            BTNode next_node = loadNode(next);
            *next_node.prev_free = node.pageID;
        }
    }
}
void IX_IndexHandle::removeLink(const BTNode& node) {
    // 将一个空闲页从空闲链表移除，一般都是移除第一项
    // 若node存在，则更新nextFree和node的nextFree
    // 若node不存在，则更新nextFree
    Debug::print(Debug::BTREE_LINK_DETAIL, "IX_IndexHandle::removeLink:%d", node.pageID);
    if (node.pageID == headPage.nextFree) {
        // 移除第一项
        if (headPage.nextFree == headPage.pageNum) {
            // node不存在
            headPage.nextFree++;
            return;
        } else if (headPage.nextFree < headPage.pageNum) {
            // node存在
            int next = *node.next_free;
            headPage.nextFree = next;
            if (next < headPage.pageNum) {
                BTNode next_node = loadNode(next);
                *next_node.prev_free = 0;
            }
            return;
        }   
    }
    assert(false);
    int next = *node.next_free;
    headPage.nextFree = next;
    assert(*node.prev_free == 0);
    if (next != headPage.pageNum) {
        BTNode next_node = loadNode(next);
        *next_node.prev_free = node.pageID;
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
    if (node.isLeaf()) return;
    for (int i = 0; i < node.getChildSize(); i++) {
        int child = node.getChild(i);
        if (child != NULL_NODE) {
            BTNode chi = loadNode(child);
            chi.print();
        }
    }
}
void IX_IndexHandle::findFirstValue(void* value0, int& pageID, int& rank) {
    // 原型
    char value[MAX_ATTRLENGTH];
    appendNullbit(value, (char*)value0, attrLength);
    // 若找到返回0
    int v = _root;
    while (v != NULL_NODE) {
        BTNode node_v = loadNode(v);
        int len = node_v.getDataSize();
        if (node_v.isLeaf()) {
            // 只有这里返回
            node_v.getData(0, data_global, rid_global);
            if (BTNode::comp((char*)value, data_global) <= 0) {
                pageID = node_v.pageID;
                rank = 0;
                return;
            }
            int l = 0, r = len - 1;
            while (l <= r) {
                int mid = (l + r) >> 1;
                node_v.getData(mid, data_global, rid_global);
                int comp = BTNode::comp((char*)value, data_global);
                // comp = e - x[m]
                if (comp <= 0) {
                    r = mid - 1;
                } else {
                    l = mid + 1;
                }
            }
            pageID = node_v.pageID;
            rank = r;
            return;
        } else {
            // 转向最后一个小于关键值的孩子
            node_v.getData(0, data_global, rid_global);
            if (BTNode::comp((char*)value, data_global) <= 0) {
                v = node_v.getChild(0);
                continue;
            }
            int l = 0, r = len - 1;
            while (l <= r) {
                int mid = (l + r) >> 1;
                node_v.getData(mid, data_global, rid_global);
                int comp = BTNode::comp((char*)value, data_global);
                // comp = e - x[m]
                if (comp <= 0) {
                    r = mid - 1;
                } else {
                    l = mid + 1;
                }
            }
            v = node_v.getChild(r);
        }
    }
}
int IX_IndexHandle::findLastValue(void* value, int& pageID, int& rank, RID& rid) {
    return 0;
}
BTNode IX_IndexHandle::loadFirstLeaf() {
    if (headPage.right != NULL_NODE) {
        return loadNode(headPage.right);
    }
    assert(false);
    return loadNode(_root);
}
int IX_IndexHandle::loadNextNode(BTNode& node) {
    if (*node.right != NULL_NODE) {
        return *node.right;
    }
    return NULL_NODE;
}
/* IX_IndexScan */
IX_IndexScan::IX_IndexScan() {

}
IX_IndexScan::~IX_IndexScan() {

}
int IX_IndexScan::OpenScan(IX_IndexHandle &indexHandle, CompOp compOp, void *value) {
    if (BTNode::bufPageManager == NULL)
        BTNode::bufPageManager = indexHandle.bufPageManager;
    numScanned = 0;
    this->indexHandle = &indexHandle;
    attrLength = indexHandle.attrLength;
    attrType = indexHandle.headPage.getType();
    current_value = new char[attrLength];
    if (value == NULL) {
        if (compOp == EQ_OP || compOp == NE_OP) {

        } else {
            cout << "IX_IndexScan::OpenScan 空值只能使用EQ或者NE\n";
            return -1;
        }
        this->value = NULL;
    } else {
        this->value = new char[attrLength];
        memcpy(this->value, value, attrLength);
    }
    this->compOp = compOp;
    comparator = getComparator(compOp);
    return 0;
}
int IX_IndexScan::tryLoadNext(BTNode& node, int& rank, char* data, RID& rid) {
    // rank有可能已经改变
    while (node.getNextData(rank, data, rid)) {
        int next = indexHandle->loadNextNode(node);
        if (next != NULL_NODE) {
            rank = -1;
            node = indexHandle->loadNode(next);
        } else {
            // 到了叶结点的末尾
            return -1;
        }
    }
    return 0;
}
int IX_IndexScan::findNext(RID& rid) {
    // 若非第一次获取，则需要判断是否大于之前获取的值
    // 返回0表示获取成功。需要记录此时的值
    while (tryLoadNext(node_current, rank, data_global, rid_global) == 0) {
        bool satisfied;
        if (value == NULL) {
            assert(compOp == EQ_OP || compOp == NE_OP);
            if (compOp == EQ_OP) {
                // 为null
                satisfied = (data_global[attrLength] == 0);
            } else {
                // 为非null
                satisfied = (data_global[attrLength] == 1);
            }
        } else {
            satisfied = comparator(data_global, value, attrType, attrLength);
        }
        if (satisfied) {
            // 满足过滤条件
            if (numScanned == 0) {
                rid.copy(rid_global);
                numScanned++;
                current_RID.copy(rid_global);
                memcpy(current_value, data_global, attrLength);
                return 0;
            } else {
                if (BTNode::comp(data_global, rid_global, (char*)current_value, current_RID) > 0) {
                    // 当前值大于记录的值，有效
                    rid.copy(rid_global);
                    numScanned++;
                    current_RID.copy(rid_global);
                    memcpy(current_value, data_global, attrLength);
                    return 0;
                }
            }
        }
    } 
    return -1;
}
int IX_IndexScan::GetNextEntry(RID &rid) {
    // 为了允许删除，插入操作，不能只记录rank，要记录上次查询的rid
    bool shouldReset = false;
    if (numScanned > 0) {
        node_current.getData(rank, data_global, rid_global);
        if (RID::comp(current_RID, rid_global) != 0) {
            // 执行了删除操作，重置指针
            shouldReset = true;
        }
    }
    if (numScanned > 0 && !shouldReset) {
        // 没有插入、删除操作
        return findNext(rid);
    } else if (numScanned == 0) {
        // 第一次扫描
        if (compOp == CompOp::EQ_OP || compOp == CompOp::GT_OP || compOp == CompOp::GE_OP) {
            // 找到最后一个小于value的位置
            int pageID;
            indexHandle->findFirstValue(value, pageID, rank);
            node_current = indexHandle->loadNode(pageID);
            return findNext(rid);
        } else if (compOp == CompOp::LT_OP || compOp == CompOp::LE_OP) {
            // 找到第一个结点
            rank = -1;
            node_current = indexHandle->loadFirstLeaf();
            return findNext(rid);
        } 
    } else if (shouldReset) {
        if (compOp == CompOp::EQ_OP || compOp == CompOp::GT_OP || compOp == CompOp::GE_OP) {
            // 找到最后一个小于value的位置
            int pageID;
            indexHandle->findFirstValue(value, pageID, rank);
            node_current = indexHandle->loadNode(pageID);
            return findNext(rid);
        } else if (compOp == CompOp::LT_OP || compOp == CompOp::LE_OP) {
            // 找到第一个结点
            rank = -1;
            node_current = indexHandle->loadFirstLeaf();
            return findNext(rid);
        } 
    }
    return 0;
}
int IX_IndexScan::CloseScan() {
    return 0;
}
void IX_IndexHandle::addLeafLinkAfter(BTNode& node_v, BTNode& node_after) {
    int rightBro = *node_v.right;
    *node_v.right = node_after.pageID;
    *node_after.left = node_v.pageID;
    *node_after.right = rightBro;
    if (rightBro != NULL_NODE) {
        BTNode right_node = loadNode(rightBro);
        *right_node.left = node_after.pageID;
    }
}
void IX_IndexHandle::removeLeafLink(BTNode& node) {
    // 移除当前结点的叶子链接
    if (!node.isLeaf()) {
        return;
    }
    int leftBro = *node.left, rightBro = *node.right;
    if (leftBro != NULL_NODE) {
        BTNode left_node = loadNode(leftBro);
        *left_node.right = rightBro;
    } else {
        headPage.right = rightBro;
    }
    if (rightBro != NULL_NODE) {
        BTNode right_node = loadNode(rightBro);
        *right_node.left = leftBro;
    }
}
void BTNode::print() const {
    Debug::debug("当前结点 [pageID:%d,parent:%d,entry_count:%d,prev_free:%d,next_free:%d] %s叶子结点",\
        pageID, *parent, *entry_count, *prev_free, *next_free, (isLeaf()) ? "是" : "不是");
    for (int i = 0; i < *entry_count; i++) {
        getData(i, data_global, rid_global);
        Debug::print(Debug::BTREE_PRINT_DATA_DETAIL, "第%d项{RID[%d,%d] childPageID:%d}", i, rid_global.getPage(), rid_global.getSlot(), getChild(i));
    }
    /* Debug::debug("\tdata:");
    for (int i = 0; i < dataSize; i++) {
        getData(i, data_global, rid_global);
        Debug::debug("\t[i:%d,data:%d,rid:(%d,%d)]", i, *(int*)data_global, rid_global.getPage(), rid_global.getSlot());
    } */
}
void BTNode::markDirty() const {
    bufPageManager->markDirty(bufIndex);
}
BTNode::BTNode() {
    parent = NULL; left = NULL; right = NULL; entry_count = NULL;
    values = NULL; rids = NULL; childs = NULL; prev_free = next_free = NULL;
    BTNode::entryLimit = 0; BTNode::attrLength = 0; BTNode::attrType = INVALID;
    pageID = bufIndex = -1;
}
BTNode::BTNode(BufPageManager* bufPageManager) {
    if (BTNode::bufPageManager == NULL)
        BTNode::bufPageManager = bufPageManager;
    parent = NULL; left = NULL; right = NULL; entry_count = NULL;
    values = NULL; rids = NULL; childs = NULL; prev_free = next_free = NULL;
    BTNode::entryLimit = 0; BTNode::attrLength = 0; BTNode::attrType = INVALID;
    pageID = bufIndex = -1;
}
int BTNode::comp(char* v1, char* v2) {
    // 拓展型
    // 返回 v1 - v2;
    // 比较时，空值之间相等，小于其他值 v1,v2可能从其他地方取得
    assert(v1 != NULL && v2 != NULL);
    bool isNull1, isNull2;
    isNull1 = (v1[attrLength] == 0);
    isNull2 = (v2[attrLength] == 0);
    if (isNull1 && isNull2) {
        return 0;
    } else if (isNull1) {
        return -1;
    } else if (isNull2) {
        return 1;
    }
    switch (BTNode::attrType) {
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
        case DATE:
            return 0;
        case AttrType::INVALID:
            Debug::produce("BTNode comp invalid type");
            return 0;
    }
    return 0;
}
int BTNode::comp(char* v1, const RID& r1, char* v2, const RID& r2) {
    // 拓展型
    int value = BTNode::comp(v1, v2);
    return (value == 0) ? RID::comp(r1, r2) : value;
}
int BTNode::search(char* data, const RID& rid) {
    // 拓展型
    assert(data != NULL);
    // 规定 索引保存子树的最小值
    // 若为叶子结点，返回待插入位置[0, count] 或者存在的秩 [0, count)
    // 若为索引结点，返回待插入位置或存在的秩所在的子树的秩 [0, count)
    bool isLeaf = this->isLeaf();
    if (entry_count == NULL || *entry_count == 0) {
        // 这是一个空结点，则必然是叶子
        assert(isLeaf);
        return 0;
    }
    if (isLeaf) {
        int l = 0, r = *entry_count - 1;
        getData(l, data_global, rid_global);
        if (BTNode::comp(data, rid, data_global, rid_global) <= 0)
            return 0;
        getData(r, data_global, rid_global);
        if (BTNode::comp(data, rid, data_global, rid_global) > 0)
            return r + 1;
        while (l < r - 1) {
            int m = (l + r) >> 1;
            getData(m, data_global, rid_global);
            if (BTNode::comp(data, rid, data_global, rid_global) < 0) {
                r = m - 1;
            } else {
                l = m;
            }
        }
        if (l == r) return l;
        getData(r, data_global, rid_global);
        if (BTNode::comp(data, rid, data_global, rid_global) < 0) {
            return l;
        }
        return r;
    } else {
        // 索引
        if (*entry_count <= 1)
            return 0;
        int l = 1, r = *entry_count - 1;
        getData(r, data_global, rid_global);
        if (BTNode::comp(data, rid, data_global, rid_global) >= 0) {
            return r;
        }
        while (l <= r) {
            int m = (l + r) >> 1;
            getData(m, data_global, rid_global);
            // comp = e - v[m]
            int comp = BTNode::comp(data, rid, data_global, rid_global);
            if (comp < 0) {
                // e < v[m]
                r = m - 1;
            } else if (comp >= 0) {
                // e >= v[m]
                l = m + 1;
            }
        }
        return --l;
    }
}
/* revise node */
void BTNode::setChild(int rank, int pageID) {
    if (rank >= 0 && rank < *child_count) {
        if (childs[rank] != pageID) {
            childs[rank] = pageID;
            markDirty();    
        }
        return;
    }
    Debug::debug("setChild rank [%d, %d]", rank, *child_count);
}
void BTNode::setData(int rank, char* data, RID& rid) {
    // 拓展型
    if (rank < 0 || rank >= *entry_count) {
        Debug::error("setData range exceeds %d, %d", rank, *entry_count);
        return;
    }
    assert(data != NULL);
    markDirty();
    // 增加了1位
    char* value = values + rank * (attrLength + 1);
    memcpy(value, data, attrLength + 1);
    rids[rank].copy(rid);
}
void BTNode::removeData(int rank) {
    // copy (rank, size) to [rank, size - 1)
    int len = (*entry_count - 1 - rank); // len是右边的数目
    if (len < 0 || rank < 0) {
        Debug::error("removeData rank [%d, %d]", rank, *entry_count);
        return;
    }
    int actualLen = attrLength + 1;
    int offset = rank * actualLen;
    void* dest = (void*)(values + offset);
    void* src = (void*)(values + (offset + actualLen));
    memmove(dest, src, len * actualLen);
    for (int i = rank; i < *entry_count - 1; i++) {
        rids[i].copy(rids[i + 1]);
    }
    *entry_count-=1;
    markDirty();
}
void BTNode::getData(int rank, char* data, RID& rid) const{
    // 拓展型
    if (rank < 0 || rank >= *entry_count) {
        Debug::error("getData range exceeds %d, %d", rank, *entry_count);
        return;
    }
    assert(data != NULL);
    int actualLen = attrLength + 1;
    char* value = values + rank * actualLen;
    memcpy(data, value, actualLen);
    rid.copy(rids[rank]);
}
void BTNode::insertData(int rank, char* data, const RID& rid) {
    // 拓展型
    // rank [0, dataSize]
    if (rank > *entry_count || rank < 0) {
        Debug::error("insertData rank [%d, %d]", rank, *entry_count);
        return;
    }
    assert(data != NULL);
    markDirty();
    int actualLen = attrLength + 1;
    // [rank, dataSize) -> (rank, dataSize]
    if (rank != *entry_count)
        memmove((void*)(values + (rank + 1) * actualLen), (void*)(values + rank * actualLen), (*entry_count - rank) * actualLen);
    for (int i = *entry_count; i > rank; i--) {
        rids[i].copy(rids[i - 1]);
    }
    char *value = values + rank * actualLen;
    memset(value, 0, actualLen);
    rids[rank].copy(rid);
    *entry_count+=1;
}
void BTNode::removeData(int rank, char* data, RID& rid) {
    // 拓展型
    int len = (*entry_count - 1 - rank);
    if (len < 0 || rank < 0) {
        Debug::error("removeData (3param) rank [%d, %d]", rank, *entry_count);
        return;
    }
    markDirty();
    int actualLen = attrLength + 1;
    int offset = rank * actualLen;
    void* dest = (void*)(values + offset);
    void* src = (void*)(values + (offset + actualLen));
    memcpy(data, dest, actualLen);
    rid.copy(rids[rank]);
    memmove(dest, src, len * actualLen);
    for (int i = rank; i < *entry_count - 1; i++) {
        rids[i].copy(rids[i + 1]);
    }
    *entry_count-=1;
}
int BTNode::removeChild(int rank) {
    if (rank < 0 || rank >= *child_count) {
        Debug::error("removeChild rank [%d, %d]", rank, *child_count);
        return NULL_NODE;
    }
    // (rank, childSize) 
    int ret = childs[rank];
    memmove((void*)&childs[rank], (void*)(&childs[rank + 1]), (*child_count - 1 - rank) * sizeof(int));
    *child_count-=1;
    markDirty();
    return ret;
}
void BTNode::insertChild(int rank, int id) {
    // rank [0, childSize]
    if (rank > *child_count || rank < 0) {
        Debug::error("insertChild rank [%d, %d]", rank, *child_count);
        return;
    }
    markDirty();
    if (rank == *child_count) {
        childs[rank] = id;
        *child_count+=1;
        return;
    }
    // [rank, childSize) -> (rank, childSize]
    memmove((void*)(&childs[rank + 1]), (void*)(&childs[rank]), (*child_count - rank) * sizeof(int));
    childs[rank] = id;
    *child_count+=1;
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
    if (child >= 0 && child < *child_count)
        return childs[child];
    return NULL_NODE;
}
int BTNode::getNextData(int& rank, char* data, RID& rid) const {
    // 获取rank+1的数据
    if (rank + 1 == *entry_count) {
        return -1;
    }
    getData(++rank, data, rid);
    return 0;
}
int BTNode::getChildSize() const {
    assert(child_count != NULL);
    return *child_count;
}
int BTNode::getDataSize() const {
    assert(entry_count != NULL);
    return *entry_count;
}
int BTNode::getParent() const {
    if (parent == NULL)
        return NULL_NODE;
    return *parent;
}
void BTNode::setLeaf(bool isLeaf) {
    if (isLeaf) {
        *is_leaf = 1;
    } else {
        *is_leaf = 0;
    }
}
bool BTNode::isLeaf() const {
    return (*is_leaf == 1);
}
void BTNode::withEmptyPage(BufType b, Head_Page headPage, int pID, int index) {
    //node init here
    memset((void*)b, 0, PAGESIZE);
    withContentPage(b, headPage, pID, index);
    *parent = NULL_NODE;
    *left = NULL_NODE;
    *right = NULL_NODE;
    *prev_free = NULL_NODE;
    *next_free = NULL_NODE;
    *entry_count = 0;
    *child_count = 0;
    *is_leaf = 0;
}
void BTNode::withContentPage(BufType b, Head_Page headPage, int pID, int index) {
    parent = (int*)b;
    left = (int*)b + 1;
    right = (int*)b + 2;
    prev_free = (int*)b + 3;
    next_free = (int*)b + 4;
    entry_count = (int*)b + 5;
    child_count = (int*)b + 6;
    is_leaf = (int*)b + 7;
    BTNode::entryLimit = headPage.entryLimit;
    BTNode::attrLength = headPage.attrLength;
    BTNode::attrType = headPage.getType();
    pageID = pID; bufIndex = index;
    values = (char*)((char*)b + sizeof(int) * 8);
    int actualLen = attrLength + 1;
    char* rid_pos = values + (BTNode::entryLimit + 1) * actualLen;
    char* child_pos = rid_pos + (BTNode::entryLimit + 1) * sizeof(RID);
    rids = (RID*)(rid_pos);
    childs = (int*)child_pos;
}