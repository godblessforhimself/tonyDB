#ifndef GLOBALHOLDER
#define GLOBALHOLDER
#include "const.h"
#include "recordmanager/rm.h"
#include "indexing/indexing.h"
#include "systemmanager/sm.h"
#include "qlmanager/ql.h"
extern FileManager* gl_fileManager;
extern BufPageManager* gl_bufPageManager;
extern RecordManager* gl_recordManager;
extern IX_Manager* gl_indexingManager;
extern SM_Manager* gl_systemManager;
extern QL_Manager *gl_qlManager;
#endif
