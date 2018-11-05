/* const defined in this file */
#ifndef CONST_H
#define CONST_H
#include <stdio.h>
#include <assert.h>

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
#endif
