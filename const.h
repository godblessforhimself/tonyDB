/* const defined in this file */
#ifndef CONST_H
#define CONST_H
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#define CopyANDPrint(x) printf("%s: ", x);\
va_list args;\
va_start(args, format);\
vprintf(format, args);\
va_end(args);\
printf("\n")
namespace constSpace {
	enum AttrType {
	    INT,
	    FLOAT,
	    STRING
	};
	enum CompOp {
	    NO_OP,                                      // no comparison
	    EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP    // binary atomic operators
	};
}
class Debug {
	static const int DEBUG = 0;
	static const int INFO = 3;
	static const int PROD = 5;
	static const int level = DEBUG;
public:
	static void debug(const char* format, ...) {
		if (level >= DEBUG) {
			CopyANDPrint("DEBUG");
		}
	}
	static void info(const char* format, ...) {
		if (level >= INFO) {
			CopyANDPrint("INFO");
		}
	}
	static void produce(const char* format, ...) {
		if (level >= PROD) {
			CopyANDPrint("PRODUCE");
		}
	}
};
#endif
