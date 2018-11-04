/* const defined in this file */
#include <stdio.h>
using namespace std;
enum AttrType {
    INT,
    FLOAT,
    STRING
};
enum CompOp {
    NO_OP,                                      // no comparison
    EQ_OP, NE_OP, LT_OP, GT_OP, LE_OP, GE_OP    // binary atomic operators
};

