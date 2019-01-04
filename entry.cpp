#include "globalHolder.h"
#include "parser/parser.h"
void initInstances();
FileManager* gl_fileManager;
BufPageManager* gl_bufPageManager;
RecordManager* gl_recordManager;
IX_Manager* gl_indexingManager;
SM_Manager* gl_systemManager;
QL_Manager *gl_qlManager;
int main() {
    initInstances();
    runParser();
}
void initInstances() {
    gl_fileManager = new FileManager();
    gl_bufPageManager = new BufPageManager(gl_fileManager);
    gl_recordManager = new RecordManager(gl_fileManager, gl_bufPageManager);
    gl_indexingManager = new IX_Manager(gl_fileManager, gl_bufPageManager);
    gl_systemManager = new SM_Manager(gl_indexingManager, gl_recordManager);
    gl_qlManager = new QL_Manager();
}