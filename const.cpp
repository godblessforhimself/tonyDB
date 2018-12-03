#include "const.h"
bool constSpace::equal(void* value1, void* value2, constSpace::AttrType attrtype, int attrLength) {
	switch (attrtype) {
	    case constSpace::FLOAT: return (*(float*)value1 == *(float*)value2);
	    case constSpace::INT: return (*(int*)value1 == *(int*)value2) ;
	    default: return (strncmp((char*) value1, (char*) value2, attrLength) == 0); 
  	}
}
bool constSpace::less_than(void* value1, void* value2, constSpace::AttrType attrtype, int attrLength) {
  	switch (attrtype) {
    	case constSpace::FLOAT: return (*(float*)value1 < *(float*)value2);
    	case constSpace::INT: return (*(int*)value1 < *(int*)value2);
    	default: return (strncmp((char*) value1, (char*) value2, attrLength) < 0);
  }
}
bool constSpace::greater_than(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength){
 	switch (attrtype) {
   		case constSpace::FLOAT: return (*(float *)value1 > *(float*)value2);
    	case constSpace::INT: return (*(int *)value1 > *(int *)value2) ;
    	default: return (strncmp((char *) value1, (char *) value2, attrLength) > 0);
  }
}
bool constSpace::less_than_or_eq_to(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength){
  	switch(attrtype){
    	case constSpace::FLOAT: return (*(float *)value1 <= *(float*)value2);
    	case constSpace::INT: return (*(int *)value1 <= *(int *)value2) ;
    	default: return (strncmp((char *) value1, (char *) value2, attrLength) <= 0);
  }
}
bool constSpace::greater_than_or_eq_to(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength){
  	switch(attrtype){
    	case constSpace::FLOAT: return (*(float *)value1 >= *(float*)value2);
    	case constSpace::INT: return (*(int *)value1 >= *(int *)value2) ;
    	default: return (strncmp((char *) value1, (char *) value2, attrLength) >= 0);
  }
}
bool constSpace::not_equal(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength){
  	switch(attrtype){
    	case constSpace::FLOAT: return (*(float *)value1 != *(float*)value2);
    	case constSpace::INT: return (*(int *)value1 != *(int *)value2) ;
    	default: return (strncmp((char *) value1, (char *) value2, attrLength) != 0);
  }
}