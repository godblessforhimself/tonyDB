// 删除数据库
#include <iostream>
#include <unistd.h>
using namespace std;
int main(int argc, char **argv) {
    char *dbname;
    char command[80] = "rm -r ";
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " dbname \n";
        exit(1);
    }
    // The database name is the second argument
    dbname = argv[1];
    system(strcat(command, dbname));
}