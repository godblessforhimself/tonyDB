#include "const.h"
using namespace constSpace;
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
int constSpace::getattrlength(AttrType a) {
	switch (a) {
		case AttrType::INT:
			return 4;
		case FLOAT:
			return 4;
		case DATE:
			return 4;
		case STRING:
			return 0;
		return 0;
	}
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
	c << "[" << entry->attrName << "," << entry->offset << "," << type << "," << entry->attrLenth << "," << entry->indexNo << endl;
}
void constSpace::transAttrType(AttrType type, char* dst)
{
	switch (type) {
		case INT:
			strcpy(dst, "int");
			return;
		case FLOAT:
			strcpy(dst, "float");
			return;
		case STRING:
			strcpy(dst, "string");
			return;
		case INVALID:
			strcpy(dst, "invalid");
			return;
	}
}
void parser_node::set_attr_type(AttrType* type, int len) {
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
void parser_node::init_field_list() {
	nType = N_FIELD_LIST;
	u.FieldList.len = 0;
}
void parser_node::field_list_append(parser_node* a) {
	if (u.FieldList.len >= MAX_LIST_LENGTH) {
		printf("error!parser_node::field_list_append\n");
	}
	u.FieldList.fList[u.FieldList.len++] = a;
}
void parser_node::init_string_list() {
	nType = N_STRING_LIST;
	u.StringList.len = 0;
}
void parser_node::append_string_list(char *a) {
	if (u.StringList.len >= MAX_LIST_LENGTH) {
		printf("error!parser_node::append_string_list\n");
	}
	u.StringList.stringlist[u.StringList.len++] = a;
}
void parser_node::set_field_foreign_key(char* loc, char* ftable, char *fname) {
	nType = N_FIELD;
	u.Field.ftype = f_foreign_key;
	u.Field.v.foreignkey.localname = loc;
	u.Field.v.foreignkey.foreigntable = ftable;
	u.Field.v.foreignkey.foreignname = fname;
}
void parser_node::set_field_primary_list() {
	nType = N_FIELD_PRIMARY_KEY_LIST;
}
void parser_node::printStringList(ostream &o) {
	for (int i = 0; i < u.StringList.len; ++i) {
		o << i << ": " << u.StringList.stringlist[i] << endl;
	}
}
void parser_node::print(ostream& o) {
	switch (nType) {
		case N_ATTRTYPE:
			switch (*u.ATTRTYPE.type) {
				case INT:
					o << "int " << u.ATTRTYPE.len << endl;
					break;
				case FLOAT:
					o << "float " << u.ATTRTYPE.len << endl;
					break;
				case STRING:
					o << "string " << u.ATTRTYPE.len << endl;
					break;
				case DATE:
					o << "date " << u.ATTRTYPE.len << endl;
					break;
				default:
					o << "default " << u.ATTRTYPE.len << endl;
					break;
			}
			break;
		case N_FIELD:
			switch (u.Field.ftype) {
				case f_normal:
					o << "普通域: " << u.Field.v.normal.colname << ",";
					u.Field.v.normal.type->print(o);
					break;
				case f_notnull:
					o << "非空域: " << u.Field.v.notnull.colname << ",";
					u.Field.v.notnull.type->print(o);
					break;
				case f_foreign_key:
					o << "外键约束: " << u.Field.v.foreignkey.localname << "," << u.Field.v.foreignkey.foreigntable << "," << u.Field.v.foreignkey.foreignname << endl;
					break;
			}
			break;
		case N_FIELD_PRIMARY_KEY_LIST:
			o << "主键约束:" << endl;
			printStringList(o);
			break;
		case N_FIELD_LIST:
			o << "fieldList 长度" << u.FieldList.len << "\n";
			for (int i = 0; i < u.FieldList.len; ++i) {
				u.FieldList.fList[i]->print(o);
			}
			break;
		default:
			o << "this should assert\n";
	}
}
