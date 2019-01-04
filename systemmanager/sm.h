#ifndef SM_H
#define SM_H
#include "../const.h"
#include "../recordmanager/rm.h"
#include "../indexing/indexing.h"
#include <unistd.h>
using namespace std;
class SM_Manager {
public:
    SM_Manager(IX_Manager* ix, RecordManager* rm);
    ~SM_Manager();
    int createDb(const char *dbName);
    int dropDb(const char *dbName);
    int openDb(const char *dbName);
    int closeDb();
    int showTables();
    int printTable(const char *tbName, ostream &o);
    int createTable(const char *tbName, parser_node *fieldList);
    int createIndex(const char *relName, const char *attrName);
    int dropTable(const char *relName);
    int dropIndex(const char *relName, const char *attrName);
    RecordHandle relationHandle;
    RecordHandle attrHandle;
    bool usingDb;
private:
    RecordManager* recordManager;
    IX_Manager* indexingManager;
    char pwd[1024];
};
#endif // SM_H