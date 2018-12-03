#include "indexing.h"
#include "../const.h"
RID rid_global_test;
FileManager* fileManager = new FileManager();
BufPageManager* bufPageManger = new BufPageManager(fileManager);
IX_Manager manager(fileManager, bufPageManger);
char fileName[20] = "indexingtest.db";
void test1() {
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
}
void test2() {
	manager.DestroyIndex(fileName, 0);
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
	IX_IndexHandle handle;
	manager.OpenIndex(fileName, 0, handle);
	int data = 5;
	rid_global_test.set(1, 1);
	for (int i = 0; i < 49800; ++i) {
		rid_global_test.set(i, 2*i);
		handle.InsertEntry((void*)&i, rid_global_test);	
	}
	handle.traverse();
}
void test3() {
	manager.DestroyIndex(fileName, 0);
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
	IX_IndexHandle handle;
	manager.OpenIndex(fileName, 0, handle);
	int total = 49900;
	for (int i = 0; i < total; ++i) {
		rid_global_test.set(i, 2*i);
		handle.InsertEntry((void*)&i, rid_global_test);	
	}
	for (int i = 0; i < total; ++i) {
		rid_global_test.set(i, 2*i);
		handle.DeleteEntry((void*)&i, rid_global_test);	
	}
	handle.traverse();
}
void test4() {
	manager.DestroyIndex(fileName, 0);
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
	IX_IndexHandle handle;
	manager.OpenIndex(fileName, 0, handle);
	int total = 49900;
	for (int i = total - 1; i >= 0; i--) {
		rid_global_test.set(i, 2*i);
		handle.InsertEntry((void*)&i, rid_global_test);	
	}
	for (int i = total - 1; i >= 0; i--) {
		rid_global_test.set(i, 2*i);
		handle.DeleteEntry((void*)&i, rid_global_test);	
	}
	handle.traverse();
}
void test5() {
	manager.DestroyIndex(fileName, 0);
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
	IX_IndexHandle handle;
	manager.OpenIndex(fileName, 0, handle);
	int total = 49900;
	for (int i = total - 1; i >= 0; i--) {
		rid_global_test.set(i, 2*i);
		handle.InsertEntry((void*)&i, rid_global_test);	
	}
	IX_IndexScan scan;
	int value = 20000;
	if (!scan.OpenScan(handle, CompOp::LT_OP, (void*)(&value))) {
		while (!scan.GetNextEntry(rid_global_test)) {
			rid_global_test.show();
			int n = rid_global_test.getPage();
			handle.DeleteEntry((void*)(&n), rid_global_test);	
		}
	}
	handle.traverse();
}
int main()
{
	test5();
}