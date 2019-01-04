drop database db1;
create database db1;
use db1;
create table tb1(col1 int(5), col2 int(5) not null, primary key(col1, col2), foreign key(col2) references tb2(colx));
