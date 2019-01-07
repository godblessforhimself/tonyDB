#include "indexing.h"
#include "../const.h"
#include <algorithm>
RID rid_global_test;
FileManager* fileManager = new FileManager();
BufPageManager* bufPageManger = new BufPageManager(fileManager);
IX_Manager manager(fileManager, bufPageManger);
char fileName[20] = "indexingtest.db";
void test1() {
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
}
void test2() {
	// 测试插入
	manager.DestroyIndex(fileName, 0);
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
	IX_IndexHandle handle;
	manager.OpenIndex(fileName, 0, handle);
	for (int i = 0; i < 50000; ++i) {
		rid_global_test.set(i, 2*i);
		handle.InsertEntry((void*)&i, rid_global_test);	
		Debug::debug("%d", i);
	}
	handle.traverse();
}
void test3() {
	// 测试插入、删除
	manager.DestroyIndex(fileName, 0);
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
	IX_IndexHandle handle;
	manager.OpenIndex(fileName, 0, handle);
	int total = 499;
	for (int i = 0; i < total; ++i) {
		rid_global_test.set(i, 2*i);
		handle.InsertEntry((void*)&i, rid_global_test);	
	}
	for (int i = 0; i < total; ++i) {
		rid_global_test.set(i, 2*i);
		handle.DeleteEntry((void*)&i, rid_global_test);	
	}
	for (int i = 0; i < total; ++i) {
		rid_global_test.set(i, 2*i);
		handle.InsertEntry((void*)&i, rid_global_test);	
	}
	for (int i = 0; i < total; ++i) {
		rid_global_test.set(i, 2*i);
		handle.DeleteEntry((void*)&i, rid_global_test);	
	}
	handle.traverse();
	handle.printFreeLink();
}
void test4() {
	// 插入删除 倒序
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
	for (int i = 0; i < total; ++i) {
		rid_global_test.set(i, 2*i);
		handle.InsertEntry((void*)&i, rid_global_test);	
	}
	for (int i = 0; i < total; ++i) {
		rid_global_test.set(i, 2*i);
		handle.DeleteEntry((void*)&i, rid_global_test);	
	}
	handle.traverse();
	handle.printFreeLink();
}
void test5() {
	manager.DestroyIndex(fileName, 0);
	manager.CreateIndex(fileName, 0, AttrType::INT, sizeof(int));
	IX_IndexHandle handle;
	manager.OpenIndex(fileName, 0, handle);
	int total = 40000;
	int arr[total];// = {2, 8, 1,0,3,4,5,9,6,7};
	 for (int i = 0; i < total; ++i) 
		arr[i] = i;
	std::random_shuffle(arr, arr + total);
	cout << arr[0] << endl;
	for (int i = 0; i < total; i++) {
		rid_global_test.set(arr[i], arr[i]);
		handle.InsertEntry((void*)&arr[i], rid_global_test);
		//cout << arr[i] << endl;	
	}
	IX_IndexScan scan;
	int x = 0;
	if (!scan.OpenScan(handle, CompOp::NO_OP, NULL)) {
		while (scan.GetNextEntry(rid_global_test) == 0) {
			//rid_global_test.show();
			assert(x == rid_global_test.getPage());
			x++;
			// int n = rid_global_test.getPage();
			// handle.DeleteEntry((void*)(&n), rid_global_test);	
		}
	}
	// handle.traverse();
}
int main()
{
	for (int i = 0; i < 100; ++i) {
		//test5();
	}
	tickTock.tick();
	int c = 1001, llop = 100000;
	for (int lp = 0; lp < llop; ++lp) {
		for (int i = 0; i < 1000; ++i) {
			if (c == i)
				break;
		}
	}
	double second = tickTock.tock();
	cout << "时间" << second * 1000 << "ms\n";
}