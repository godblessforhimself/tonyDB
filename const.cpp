#include "const.h"
using namespace constSpace;
bool (*constSpace::getComparator(CompOp op))(void*, void*, AttrType, int) {
	switch (op) {
		case EQ_OP:
			return &constSpace::equal;
		case LT_OP:
			return &constSpace::less_than;
		case LE_OP:
			return &constSpace::less_than_or_eq_to;
		case GT_OP:
			return &constSpace::greater_than;
		case GE_OP:
			return &constSpace::greater_than_or_eq_to;
		case NE_OP:
			return &constSpace::not_equal;
		default:
			return NULL;
	}
	return NULL;
}
bool constSpace::twoTypeMatch(AttrType a, ValueType b) {
	if (b == v_null)
		return true;
	if (a == INT && b == v_int)
		return true;
	if (a == STRING && b == v_string)
		return true;
	if (a == FLOAT && b == v_float)
		return true;
	if (a == DATE && b == v_string)
		return true;
	return false;
}
bool constSpace::equal(void* value1, void* value2, constSpace::AttrType attrtype, int attrLength) {
	assert(value1 != NULL && value2 != NULL);
	switch (attrtype) {
	    case constSpace::FLOAT: return (*(float*)value1 == *(float*)value2);
	    case constSpace::INT: return (*(int*)value1 == *(int*)value2) ;
	    default: return (strncmp((char*) value1, (char*) value2, attrLength) == 0); 
  	}
}
bool constSpace::less_than(void* value1, void* value2, constSpace::AttrType attrtype, int attrLength) {
	assert(value1 != NULL && value2 != NULL);
  	switch (attrtype) {
    	case constSpace::FLOAT: return (*(float*)value1 < *(float*)value2);
    	case constSpace::INT: return (*(int*)value1 < *(int*)value2);
    	default: return (strncmp((char*) value1, (char*) value2, attrLength) < 0);
  }
}
bool constSpace::greater_than(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength){
	assert(value1 != NULL && value2 != NULL);
 	switch (attrtype) {
   		case constSpace::FLOAT: return (*(float *)value1 > *(float*)value2);
    	case constSpace::INT: return (*(int *)value1 > *(int *)value2) ;
    	default: return (strncmp((char *) value1, (char *) value2, attrLength) > 0);
  }
}
bool constSpace::less_than_or_eq_to(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength){
	assert(value1 != NULL && value2 != NULL);
  	switch(attrtype){
    	case constSpace::FLOAT: return (*(float *)value1 <= *(float*)value2);
    	case constSpace::INT: return (*(int *)value1 <= *(int *)value2) ;
    	default: return (strncmp((char *) value1, (char *) value2, attrLength) <= 0);
  }
}
bool constSpace::greater_than_or_eq_to(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength){
	assert(value1 != NULL && value2 != NULL);
  	switch(attrtype){
    	case constSpace::FLOAT: return (*(float *)value1 >= *(float*)value2);
    	case constSpace::INT: return (*(int *)value1 >= *(int *)value2) ;
    	default: return (strncmp((char *) value1, (char *) value2, attrLength) >= 0);
  }
}
bool constSpace::not_equal(void * value1, void * value2, constSpace::AttrType attrtype, int attrLength){
  	assert(value1 != NULL && value2 != NULL);
	switch(attrtype){
    	case constSpace::FLOAT: return (*(float *)value1 != *(float*)value2);
    	case constSpace::INT: return (*(int *)value1 != *(int *)value2) ;
    	default: return (strncmp((char *) value1, (char *) value2, attrLength) != 0);
  }
}
CompOp constSpace::getopByfunc(bool (*f)(void*, void*, AttrType, int)) {
	if (f == NULL) {
		return NO_OP;
	}
	if (f == &constSpace::equal) 
		return EQ_OP;
	if (f == &constSpace::not_equal)
		return NE_OP;
	if (f == &constSpace::less_than)
		return LT_OP;
	if (f == &constSpace::less_than_or_eq_to)
		return LE_OP;
	if (f == &constSpace::greater_than)
		return GT_OP;
	if (f == &constSpace::greater_than_or_eq_to)
		return GE_OP;
	return NO_OP;
}
int constSpace::getattrlength(AttrType a) {
	switch (a) {
		case AttrType::INT:
			return 4;
		case FLOAT:
			return 4;
		case DATE:
		case STRING:
		case INVALID:
			return 0;
	}
	return 0;
}
Printer::Printer() {

}
Printer::~Printer() {

}
void Printer::printRelation(ostream &c, constSpace::RelationEntry* entry) {
	c << entry->relName << " tupleLength: " << entry->tupleLength << " attrCount: " << entry->attrCount << " indexCount: " << entry->indexCount << endl;
}
void Printer::printAttribute(ostream &c, constSpace::AttributeEntry* entry) {
	char type[16];
	constSpace::transAttrType(entry->attrType, type);
	c << entry->attrName << "," << type << "," << entry->attrLenth << "," << entry->notNull << "," << entry->useForeignkey << "," << entry->isPrimarykey << endl;;
}
void constSpace::transAttrType(AttrType type, char* dst) {
	switch (type) {
		case AttrType::INT:
			strcpy(dst, attr_int);
			return;
		case FLOAT:
			strcpy(dst, attr_float);
			return;
		case STRING:
			strcpy(dst, attr_string);
			return;
		case DATE:
			strcpy(dst, attr_date);
			return;
		case INVALID:
			strcpy(dst, attr_invalid);
			return;
	}
}
AttrType constSpace::getAttrType(char *src) {
	if (strcmp(src, attr_int) == 0) {
        return AttrType::INT;
    } else if (strcmp(src, attr_float) == 0) {
        return FLOAT;
    } else if (strcmp(src, attr_string) == 0) {
        return STRING;
    } else if (strcmp(src, attr_date) == 0) {
		return DATE;
	} else {
        return INVALID;
    }
}
void parser_node::set_selector(parser_node *a) {
	nType = N_SELECTOR;
	u.Selector.colList = a;
}
void parser_node::set_attr_type(AttrType type, int len) {
	nType = N_ATTRTYPE;
	u.ATTRTYPE.type = type;
	u.ATTRTYPE.len = len;
}
void parser_node::set_field_normal(char* a, parser_node* b) {
	nType = N_FIELD;
	u.Field.ftype = f_normal;
	u.Field.v.normal.colname = a;
	u.Field.v.normal.type = b;
}
void parser_node::set_field_notnull(char* a, parser_node* b) {
	nType = N_FIELD;
	u.Field.ftype = f_notnull;
	u.Field.v.normal.colname = a;
	u.Field.v.normal.type = b;
}
void parser_node::init_string_list(char *a) {
	nType = N_STRING_LIST;
	u.StringList.string = a;
	u.StringList.next = u.StringList.tail = NULL;
}
void parser_node::append_string_list(char *a, parser_node *b) {
	b->init_string_list(a);
	if (u.StringList.next == NULL) {
		u.StringList.next = u.StringList.tail = b;
	} else {
		parser_node *&tail = u.StringList.tail;
		tail->u.StringList.next = b;
		tail = b;
	}
}
void parser_node::set_field_foreign_key(char* loc, char* ftable, char *fname) {
	nType = N_FIELD;
	u.Field.ftype = f_foreign_key;
	u.Field.v.foreignkey.localname = loc;
	u.Field.v.foreignkey.foreigntable = ftable;
	u.Field.v.foreignkey.foreignname = fname;
}
void parser_node::printStringList(ostream &o) {
	parser_node *current = this;
	while (current != NULL) {
		o << current->u.StringList.string << endl;
		current = current->u.StringList.next;
	}
}
void parser_node::set_col(char *tb, char *col) {
	nType = N_COL;
	u.Col.tbName = tb;
	u.Col.colName = col;
}
void parser_node::col_or_value(parser_node *c, parser_node *v) {
	nType = N_COL_OR_VALUE;
	u.ColOrValue.col = c;
	u.ColOrValue.value = v;
}
void parser_node::set_normal_condition(parser_node *l, CompOp o, parser_node *r) {
	nType = N_CONDITION;
	u.Condition.lh = l;
	u.Condition.op = o;
	u.Condition.expr = r;
}
void parser_node::set_null_cond(parser_node *l) {
	nType = N_IS_NULL_COND;
	u.IsNullCond.lh = l;
}
void parser_node::set_notnull_cond(parser_node *l) {
	nType = N_NOT_NULL_COND;
	u.NotNullCond.lh = l;
}
void parser_node::set_value() {
	nType = N_VALUE;
	u.Value.vtype = v_null;
}
void parser_node::set_value(int a) {
	nType = N_VALUE;
	u.Value.vtype = v_int;
	u.Value.value.integer = a;
}
void parser_node::set_value(float a) {
	nType = N_VALUE;
	u.Value.vtype = v_float;
	u.Value.value.fnumber = a;
}
void parser_node::set_value(char* a) {
	nType = N_VALUE;
	u.Value.vtype = v_string;
	u.Value.value.string = a;
}
void parser_node::init_value_list(parser_node *value) {
	nType = N_VALUE_LIST;
	u.ValueList.value = value;
	u.ValueList.next = u.ValueList.tail = NULL;
}
void parser_node::append_value_list(parser_node *value, parser_node *newptr) {
	newptr->u.ValueList.value = value;
	newptr->u.ValueList.next = newptr->u.ValueList.tail = NULL;
	if (u.ValueList.next == NULL) {
		u.ValueList.next = u.ValueList.tail = newptr;
	} else {
		parser_node *&tail = u.ValueList.tail;
		tail->u.ValueList.next = newptr;
		tail = newptr;
	}
}
void parser_node::single_set(char *a, parser_node *b) {
	nType = N_SET;
	u.SingleSet.colName = a;
	u.SingleSet.value = b;
}
void parser_node::init_list(parser_node *value) {
	nType = N_LIST;
	u.List.value = value;
	u.List.next = u.List.tail = NULL;
}
void parser_node::append_list(parser_node *value, parser_node *newptr) {
	newptr->init_list(value);
	if (u.List.next == NULL) {
		u.List.next = u.List.tail = newptr;
	} else {
		parser_node *&tail = u.List.tail;
		tail->u.List.next = newptr;
		tail = newptr;
	}
}
void parser_node::printValue(ostream &o) {
	assert(nType == N_VALUE);
	switch (u.Value.vtype) {
		case v_null:
			o << "null";
			break;
		case v_string:
			o << u.Value.value.string;
			break;
		case v_int:
			o << u.Value.value.integer;
			break;
		case v_float:
			o << u.Value.value.fnumber;
			break;
	}
}
void parser_node::printValueList(ostream &o) {
	assert(nType == N_VALUE_LIST);
	parser_node *current = this, *value;
	while (current != NULL) {
		if (current != this)
			o << ", ";
		value = current->u.ValueList.value;
		value->printValue(o);
		current = current->u.ValueList.next;
	}
}
void setBitmap(int position, void* dst, int value) {
	assert(value == 0 || value == 1);
	int bigpos = position >> 3;
	int offset = position % 8;
	char *that = (char*)dst + bigpos;
	if (value == 1) {
		*that |= (1 << offset);
	} else {
		*that &= ~(1 << offset);
	}
}
int getBitmap(int position, void* src) {
	int bigpos = position >> 3;
	int offset = position % 8;
	char that = *((char*)src + bigpos);
	if ((that & (1 << offset))) 
		return 1;
	return 0;
}
void TickTock::tick() {
	// 记录当前时间
	begin = clock();
}
double TickTock::tock() {
	// 返回上次tick\tock的秒数
	return (double)(clock() - begin) / CLOCKS_PER_SEC;
}
void PrimaryKeyBuffer::resetPKBuffer() {
	stackpos = 0;
}
PrimaryKeyBuffer::PrimaryKeyBuffer() {
	stackpos = 0;
}
int PrimaryKeyBuffer::getPKBufferState() {
	// 当前状态: 0 正常; -1 未初始化; 1溢出
	if (stackpos == 0) {
		return -1;
	}
	if (stackpos == -1)
		return 1;
	return 0;
}
void PrimaryKeyBuffer::append(int x) {
	if (stackpos >= PKSize) {
		stackpos = -1;
		return;
	}
	keys[stackpos] = x;
	stackpos++;
}
int PrimaryKeyBuffer::checkPKDuplicate(int value) {
	// 0无重复 -1重复
	assert(stackpos != -1);
	if (stackpos == 0) {
		return 0;
	}
	for (int i = 0; i < stackpos; ++i) {
		if (value == keys[i])
			return -1;
	}
	return 0;
}
TickTock tickTock;
PrimaryKeyBuffer PKBuffer;
bool usePKOptimize;