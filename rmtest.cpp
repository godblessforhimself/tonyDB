#include "rm.h"
#define STRLEN 12
#define filename ("testfile")
#define recordSum (405)
#define p(x) printf(x)
struct TestRec {
    char  str[STRLEN];
    int   num;
    float r;
    TestRec() {
    	strncpy(str, "test record", 12);
    	num = 1;
    	r = 1.2f;
    }
    void set(int x, float y) {
    	num = x;
    	r = y;
    }
    void show() {
    	printf("[%d, %f, %s]\n", num, r, str);
    }
};
int testCount = 4;
void testCase1();
void testCase2();
void testCase3();
void testCase4();
typedef void (*testFunc[])();
int main() {
	//新建一个文件，写n条记录，保存退出，打开文件，读取n条记录；按条件筛选n条记录。
	//不停的删除插入记录
	testFunc tests = {testCase1, testCase2, testCase3, testCase4};
	for (int i = 0; i < testCount; ++i) {
		printf("____TestCase[%d]____\n", i + 1);
		tests[i]();
		printf("----TestCase[%d]----\n", i + 1);
	}
}
void testCase1() {
	RecordManager rm;
	rm.destroyFile(filename);
	rm.createFile(filename, sizeof(TestRec));
	RecordHandle recordHandle;
	rm.openFile(filename, recordHandle);
	RID rid;
	Record record;
	TestRec rec;
	char* data = (char*)&rec;
	for (int i = 0; i < recordSum; i ++) {
		recordHandle.insertRec((char*)&rec, rid);
		rid.show();
		recordHandle.getRec(rid, record);
		rec = *(TestRec*)record.getData();
		rec.show();
	}
	rm.closeFile(recordHandle);
	rm.openFile(filename, recordHandle);
	for (int i = 0; i < 100; i ++) {
		rid.set(1, 50);
		printf("delete %d\n", i);
		if (!recordHandle.deleteRec(rid)) continue;
		recordHandle.insertRec(data, rid);
		recordHandle.insertRec(data, rid);
	}
	for (int i = 0; i < recordSum; i ++) {
		rid.set(1, i);
		if (recordHandle.getRec(rid, record)) continue;
		TestRec rec = *(TestRec*)record.getData();
		printf("%d\n", i);
		rec.show();
	}
	rm.closeFile(recordHandle);
}
void testCase2() {
	RecordManager rm;
	rm.destroyFile(filename);
	rm.createFile(filename, sizeof(TestRec));
	RecordHandle recordHandle;
	rm.openFile(filename, recordHandle);
	RID rid;
	Record record;
	TestRec rec;
	char* data = (char*)&rec;
	for (int i = 0; i < 3960; i ++) {
		recordHandle.insertRec((char*)&rec, rid);
		rid.show();
		printf("%d\n", i);
	}

	rm.closeFile(recordHandle);
	rm.openFile(filename, recordHandle);
	for (int i = 0; i < 10; i ++) {
		rid.set(i + 1, 50);
		printf("delete [%d, %d]\n", i + 1, 50);
		if (!recordHandle.deleteRec(rid)) continue;
		if (recordHandle.getRec(rid, record)) continue;
		TestRec rec = *(TestRec*)record.getData();
		rec.show();
	}
	for (int i = 0; i < 10; i ++) {
		recordHandle.insertRec(data, rid);
		rid.show();
	}
	rm.closeFile(recordHandle);
}
void testCase3() {
	RecordManager rm;
	rm.destroyFile(filename);
	rm.createFile(filename, sizeof(TestRec));
	RecordHandle recordHandle;
	rm.openFile(filename, recordHandle);
	RID rid;
	Record record;
	TestRec rec;
	char* data = (char*)&rec;
	for (int i = 0; i < recordSum; i ++) {
		recordHandle.insertRec((char*)&rec, rid);
		rid.show();
		recordHandle.getRec(rid, record);
		rec = *(TestRec*)record.getData();
		rec.show();
	}
	rm.closeFile(recordHandle);
	rm.openFile(filename, recordHandle);
	for (int i = 0; i < 100; i ++) {
		rid.set(1, 50);
		printf("delete %d\n", i);
		if (!recordHandle.deleteRec(rid)) continue;
		recordHandle.insertRec(data, rid);
		recordHandle.insertRec(data, rid);
	}
	for (int i = 0; i < recordSum; i ++) {
		rid.set(1, i);
		if (recordHandle.getRec(rid, record)) continue;
		TestRec rec = *(TestRec*)record.getData();
		printf("%d\n", i);
		rec.show();
	}
	rm.closeFile(recordHandle);
}
void testCase4() {
	RecordManager rm;
	rm.destroyFile(filename);
	rm.createFile(filename, sizeof(TestRec));
	RecordHandle recordHandle;
	rm.openFile(filename, recordHandle);
	RID rid;
	Record record;
	TestRec rec;
	char* data = (char*)&rec;
	for (int i = 0; i < 1025; i ++) {
		rec.set(i, (float)i);
		recordHandle.insertRec(data, rid);
		rid.show();
	}
	rm.closeFile(recordHandle);
	rm.openFile(filename, recordHandle);
	RecordScan recordScan;
	float value = 501.0f;
	recordScan.openScan(recordHandle, constSpace::FLOAT, sizeof(float), 16, constSpace::LE_OP, (void*)&value);
	while (!recordScan.getNextRec(record)) {
		rec = *(TestRec*)record.getData();
		rec.show();
	}
	rm.closeFile(recordHandle);
}